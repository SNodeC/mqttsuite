#include "AsyncPostgresConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <log/Logger.h>
#include <sstream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mqtt::mqtt::lib {

    AsyncPostgresConnection::AsyncPostgresConnection(const PostgresConfig& config)
        : ReadEventReceiver("AsyncPostgresConnectionRead", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , WriteEventReceiver("AsyncPostgresConnectionWrite", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , config_(config) {
    }

    AsyncPostgresConnection::~AsyncPostgresConnection() {
        cleanup();
    }

    void AsyncPostgresConnection::connectAsync(ConnectCallback callback) {
        if (state_ != State::DISCONNECTED) {
            if (callback) {
                callback(false, "Connection already in proggress or established");
            }
            return;
        }

        connectCallback_ = callback;
        state_ = State::CONNECTING;

        // Build connection string
        std::ostringstream connStr;

        if (config_.hostaddr.empty() && config_.hostname.empty()) {
            VLOG(0) << "PostgreSQL: Connection failed: no hostname or hostaddr specified";
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, "No hostname or hostaddr specified");
            }
            return;
        }

        // We need to avoid DNS because it would block the event loop
        // see: https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTSTARTPARAMS
        // "The hostaddr parameter must be used appropriately to prevent DNS queries from being made."

        // If hostaddr is specified, use it (avoids DNS)
        if (!config_.hostaddr.empty()) {
            connStr << "hostaddr=" << config_.hostaddr << " ";
        }

        // If hostname is specified, include it too (for SSL etc.)
        if (!config_.hostname.empty()) {
            connStr << "host=" << config_.hostname << " ";
        }

        connStr << "port=" << config_.port << " ";
        connStr << "dbname=" << config_.database << " ";
        if (!config_.username.empty()) {
            connStr << "user=" << config_.username << " ";
        }
        if (!config_.password.empty()) {
            connStr << "password=" << config_.password << " ";
        }

        VLOG(1) << "PostgreSQL: Starting non-blocking connection to " << (config_.hostaddr.empty() ? config_.hostname : config_.hostaddr);

        // Start non-blocking connection
        // PQconnectStart initiates a non-blocking connection to the database
        // see https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTSTARTPARAMS
        conn_ = PQconnectStart(connStr.str().c_str());

        if (conn_ == nullptr) {
            VLOG(0) << "PostgreSQL: Failed to allocate connection structure";
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, "Failed to allocate connection structure");
            }
            return;
        }

        // see https://www.postgresql.org/docs/current/libpq-status.html#LIBPQ-PQSTATUS
        if (PQstatus(conn_) == CONNECTION_BAD) {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Connection failed: " << error;
            cleanup();
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, error);
            }
            return;
        }

        // Set non-blocking mode
        // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQSETNONBLOCKING
        if (PQsetnonblocking(conn_, 1) != 0) {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Failed to set non-blocking mode: " << error;
            cleanup();
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, "Failed to set non-blocking mode: " + error);
            }
            return;
        }

        // Get socket file descriptor
        // see https://www.postgresql.org/docs/current/libpq-status.html#LIBPQ-PQSOCKET
        fd_ = PQsocket(conn_);
        if (fd_ < 0) {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Invalid socket descriptor: " << error;
            cleanup();
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, "Invalid socket descriptor: " + error);
            }
            return;
        }

        VLOG(2) << "PostgreSQL: Got socket descriptor: " << fd_;

        if (!ReadEventReceiver::enable(fd_) || !WriteEventReceiver::enable(fd_)) {
            VLOG(0) << "PostgreSQL: Failed to register socket with event system";
            cleanup();
            state_ = State::ERROR;
            if (connectCallback_) {
                connectCallback_(false, "Failed to register socket with event system");
            }
            return;
        }

        ReadEventReceiver::suspend();
        WriteEventReceiver::suspend();

        VLOG(2) << "PostgreSQL: Socket registered with snode.c event loop";

        // Do initial connect poll to determine what events to wait for
        handleConnectPoll();
    }

    void AsyncPostgresConnection::handleConnectPoll() {
        if (state_ != State::CONNECTING) {
            return;
        }

        // PQconnectPoll is called from readEvent/writeEvent

        // Copied from https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTSTARTPARAMS
        // - PGRES_POLLING_READING: wait until the socket is ready to read
        // - PGRES_POLLING_WRITING: wait until the socket is ready to write
        // - PGRES_POLLING_OK: indicating the connection has been successfully made
        // - PGRES_POLLING_FAILED: indicating the connection procedure has failed

        PostgresPollingStatusType pollStatus = PQconnectPoll(conn_);

        switch (pollStatus) {
            case PGRES_POLLING_READING:

                VLOG(2) << "PostgreSQL: Connection polling - waiting for read";
                if (ReadEventReceiver::isSuspended()) {
                    ReadEventReceiver::resume();
                }
                if (!WriteEventReceiver::isSuspended()) {
                    WriteEventReceiver::suspend();
                }
                break;

            case PGRES_POLLING_WRITING:
                VLOG(2) << "PostgreSQL: Connection polling - waiting for write";
                if (!ReadEventReceiver::isSuspended()) {
                    ReadEventReceiver::suspend();
                }
                if (WriteEventReceiver::isSuspended()) {
                    WriteEventReceiver::resume();
                }
                break;

            case PGRES_POLLING_OK:
                VLOG(1) << "PostgreSQL: Connection established successfully";
                state_ = State::CONNECTED;
                connected_ = true;

                // Suspend both events until there is work (a query to execute)
                if (!ReadEventReceiver::isSuspended()) {
                    ReadEventReceiver::suspend();
                }
                if (!WriteEventReceiver::isSuspended()) {
                    WriteEventReceiver::suspend();
                }

                if (connectCallback_) {
                    connectCallback_(true, "");
                    connectCallback_ = nullptr;
                }
                break;

            case PGRES_POLLING_FAILED: {
                std::string error = PQerrorMessage(conn_);
                VLOG(0) << "PostgreSQL: Connection failed: " << error;
                cleanup();
                state_ = State::ERROR;
                if (connectCallback_) {
                    connectCallback_(false, error);
                    connectCallback_ = nullptr;
                }
            } break;

            default:
                VLOG(0) << "PostgreSQL: Unknown polling status: " << pollStatus;
                break;
        }
    }

    void AsyncPostgresConnection::executeQuery(const std::string& query,
                                               QueryResultCallback onSuccess,
                                               ErrorCallback onError,
                                               const std::vector<std::string>& params) {
        if (!connected_) {
            if (onError) {
                onError("Not connected to database", -1);
            }
            return;
        }

        QueryContext* context = new QueryContext{query, params, onSuccess, onError, true};

        if (currentQuery_ != nullptr) {
            // Queue the query if busy
            queryQueue_.push(context);
            VLOG(2) << "PostgreSQL: Query queued (busy)";
            return;
        }

        currentQuery_ = context;
        state_ = State::SENDING_QUERY;

        VLOG(2) << "PostgreSQL: Sending query: " << query;

        int sendResult;
        if (params.empty()) {
            // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQSENDQUERY
            // and https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQFLUSH
            // we need to flush after sending command
            sendResult = PQsendQuery(conn_, query.c_str());
        } else {
            std::vector<const char*> paramValues;
            for (const auto& p : params) {
                paramValues.push_back(p.c_str());
            }
            // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQSENDQUERYPARAMS
            // and https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQFLUSH
            // we need to flush after sending command
            sendResult =
                PQsendQueryParams(conn_, query.c_str(), static_cast<int>(params.size()), nullptr, paramValues.data(), nullptr, nullptr, 0);
        }

        if (sendResult == 0) {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Failed to send query: " << error;
            reportError(error, -1);
            delete currentQuery_;
            currentQuery_ = nullptr;
            state_ = State::CONNECTED;
            return;
        }

        // After PQsendQuery, we need to flush the output buffer
        // Start flushing and register for write events
        state_ = State::FLUSHING;
        handleFlush();
    }

    void AsyncPostgresConnection::handleFlush() {
        if (state_ != State::FLUSHING) {
            return;
        }

        // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQFLUSH
        // PQflush attempts to flush outgoing data
        // Returns: 0 if successful, 1 if unable to send all data yet, -1 on error
        int flushResult = PQflush(conn_);

        if (flushResult == 1) {
            VLOG(2) << "PostgreSQL: Flush in progress, waiting for write ready";
            if (WriteEventReceiver::isSuspended()) {
                WriteEventReceiver::resume();
            }
            if (!ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::suspend();
            }
        } else if (flushResult == 0) {
            VLOG(2) << "PostgreSQL: Flush complete, waiting for results";
            state_ = State::READING_RESULT;

            if (!WriteEventReceiver::isSuspended()) {
                WriteEventReceiver::suspend();
            }
            if (ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::resume();
            }
        } else {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Flush error: " << error;
            reportError(error, -1);
            delete currentQuery_;
            currentQuery_ = nullptr;
            state_ = State::CONNECTED;
        }
    }

    void AsyncPostgresConnection::handleReadResult() {
        if (state_ != State::READING_RESULT) {
            return;
        }

        // Consume input from the connection
        // PQconsumeInput reads available data from the socket
        // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQCONSUMEINPUT
        if (PQconsumeInput(conn_) == 0) {
            std::string error = PQerrorMessage(conn_);
            VLOG(0) << "PostgreSQL: Failed to consume input: " << error;
            reportError(error, -1);
            delete currentQuery_;
            currentQuery_ = nullptr;
            state_ = State::CONNECTED;
            return;
        }

        // check if the connection is busy (results not ready yet)
        // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQISBUSY
        // "A 0 return indicates that PQgetResult can be called with assurance of not blocking."
        if (PQisBusy(conn_) == 1) {
            VLOG(3) << "PostgreSQL: Still busy, waiting for more data";
            return;
        }

        VLOG(2) << "PostgreSQL: Results ready, retrieving";

        // Get all results (there may be multiple for some queries)
        // see https://www.postgresql.org/docs/current/libpq-async.html#LIBPQ-PQGETRESULT
        PGresult* result = PQgetResult(conn_);
        while (result != nullptr) {
            processResult(result);
            // see https://www.postgresql.org/docs/current/libpq-exec.html#LIBPQ-PQCLEAR
            // free the PGresult object
            PQclear(result);
            result = PQgetResult(conn_);
        }

        VLOG(2) << "PostgreSQL: Query complete";
        delete currentQuery_;
        currentQuery_ = nullptr;
        state_ = State::CONNECTED;

        if (!ReadEventReceiver::isSuspended()) {
            ReadEventReceiver::suspend();
        }

        // Process next queued query
        if (!queryQueue_.empty()) {
            QueryContext* nextQuery = queryQueue_.front();
            queryQueue_.pop();
            executeQuery(nextQuery->query, nextQuery->onSuccess, nextQuery->onError, nextQuery->params);
            delete nextQuery;
        }
    }

    void AsyncPostgresConnection::processResult(PGresult* result) {
        if (currentQuery_ == nullptr) {
            return;
        }

        ExecStatusType status = PQresultStatus(result);

        switch (status) {
            case PGRES_COMMAND_OK:
            case PGRES_TUPLES_OK:
                VLOG(2) << "PostgreSQL: Query succeeded";
                if (currentQuery_->expectResults && currentQuery_->onSuccess) {
                    nlohmann::json jsonResult = convertResultToJson(result);
                    currentQuery_->onSuccess(jsonResult);
                }
                break;

            case PGRES_BAD_RESPONSE:
            case PGRES_NONFATAL_ERROR:
            case PGRES_FATAL_ERROR:
                {
                    std::string error = PQresultErrorMessage(result);
                    if (error.empty()) {
                        error = PQerrorMessage(conn_);
                    }
                    VLOG(0) << "PostgreSQL: Query error: " << error;

                    // Try to extract error code
                    int errorCode = -1;
                    const char* sqlstate = PQresultErrorField(result, PG_DIAG_SQLSTATE);
                    if (sqlstate != nullptr) {
                        errorCode = std::atoi(sqlstate);
                    }

                    if (currentQuery_->onError) {
                        currentQuery_->onError(error, errorCode);
                    }
                }
                break;

            default:
                VLOG(0) << "PostgreSQL: Unexpected result status: " << status;
                if (currentQuery_->onError) {
                    currentQuery_->onError("Unexpected query result status", -2);
                }
                break;
        }
    }

    nlohmann::json AsyncPostgresConnection::convertResultToJson(PGresult* res) {
        nlohmann::json result = nlohmann::json::array();

        // see https://www.postgresql.org/docs/current/libpq-exec.html#LIBPQ-PQNTUPLES
        // "Note that PGresult objects are limited to no more than INT_MAX rows, so an int result is sufficient."
        int nRows = PQntuples(res);
        int nCols = PQnfields(res);

        for (int row = 0; row < nRows; ++row) {
            nlohmann::json rowObj = nlohmann::json::object();

            for (int col = 0; col < nCols; ++col) {
                const char* fieldName = PQfname(res, col);

                // see https://www.postgresql.org/docs/current/libpq-exec.html#LIBPQ-PQGETISNULL
                // "This function returns 1 if the field is null and 0 if it contains a non-null value."
                // "Note that PQgetvalue will return an empty string, not a null pointer, for a null field."
                if (PQgetisnull(res, row, col)) {
                    rowObj[fieldName] = nullptr;
                } else {
                    // see https://www.postgresql.org/docs/current/libpq-exec.html#LIBPQ-PQGETVALUE
                    // "An empty string is returned if the field value is null. See PQgetisnull to distinguish null values from empty-string values."
                    // we fix this by checking PQgetisnull above
                    const char* value = PQgetvalue(res, row, col);

                    // see https://www.postgresql.org/docs/current/libpq-exec.html#LIBPQ-PQFTYPE
                    Oid type = PQftype(res, col);

                    // Handle common PostgreSQL types
                    /* 
                        SELECT *
                        FROM pg_type
                        WHERE typname NOT LIKE '\_%' ESCAPE '\';
                    */
                    switch (type) {
                        case 16: // BOOL
                            rowObj[fieldName] = (value[0] == 't');
                            break;
                        case 20: // INT8
                        case 21: // INT2
                        case 23: // INT4
                            try {
                                rowObj[fieldName] = std::stoll(value);
                            } catch (...) {
                                rowObj[fieldName] = value;
                            }
                            break;
                        case 700:  // FLOAT4
                        case 701:  // FLOAT8
                        case 1700: // NUMERIC
                            try {
                                rowObj[fieldName] = std::stod(value);
                            } catch (...) {
                                rowObj[fieldName] = value;
                            }
                            break;
                        default:
                            rowObj[fieldName] = value;
                            break;
                    }
                }
            }

            result.push_back(rowObj);
        }

        return result;
    }

    void AsyncPostgresConnection::cleanup() {
        while (!queryQueue_.empty()) {
            QueryContext* ctx = queryQueue_.front();
            queryQueue_.pop();
            delete ctx;
        }

        if (currentQuery_ != nullptr) {
            delete currentQuery_;
            currentQuery_ = nullptr;
        }

        if (ReadEventReceiver::isEnabled()) {
            ReadEventReceiver::disable();
        }
        if (WriteEventReceiver::isEnabled()) {
            WriteEventReceiver::disable();
        }

        if (conn_ != nullptr) {
            PQfinish(conn_);
            conn_ = nullptr;
        }

        fd_ = -1;
        connected_ = false;
        state_ = State::DISCONNECTED;
    }

    void AsyncPostgresConnection::reportError(const std::string& error, int code) {
        if (currentQuery_ != nullptr && currentQuery_->onError) {
            currentQuery_->onError(error, code);
        }
    }

    /*
     * Event handlerss called by snode.c event loop
     */

    // Called by snode.c when the PostgreSQL socket is readable
    void AsyncPostgresConnection::readEvent() {
        VLOG(3) << "PostgreSQL: Read event triggered by snode.c event loop";

        if (state_ == State::CONNECTING) {
            // Poll to continue the connection process
            handleConnectPoll();
        } else if (state_ == State::READING_RESULT) {
            handleReadResult();
        }
    }

    // Called by snode.c when the PostgreSQL socket is writable
    void AsyncPostgresConnection::writeEvent() {
        VLOG(3) << "PostgreSQL: Write event triggered by snode.c event loop";

        if (state_ == State::CONNECTING) {
            // Poll to continue the connection process
            handleConnectPoll();
        } else if (state_ == State::FLUSHING) {
            handleFlush();
        }
    }

    // Called by snode.c if read operation times out
    // (only if a timeout was set via setTimeout(), which I don't currently do)
    void AsyncPostgresConnection::readTimeout() {
        VLOG(1) << "PostgreSQL: Read timeout";
        ReadEventReceiver::disable();
    }

    // Called by snode.c if write operation times out
    // (only if a timeout was set via setTimeout(), which I don't currently do)
    void AsyncPostgresConnection::writeTimeout() {
        VLOG(1) << "PostgreSQL: Write timeout";
        WriteEventReceiver::disable();
    }

    // Called by snode.c when the connection is no longer being observed
    // This happens when all references to this connection are gone
    void AsyncPostgresConnection::unobservedEvent() {
        VLOG(1) << "PostgreSQL: Connection no longer observed by event system, self-destructing";
        delete this;
    }

} // namespace mqtt::mqtt::lib

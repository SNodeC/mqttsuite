#include "AsyncPostgresClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>
#include <sstream>
#include <cstring>
#include <vector>

#endif

namespace mqtt::mqtt::lib {

    AsyncPostgresClient::AsyncPostgresClient(const PostgresConfig& config, size_t poolSize)
        : config_(config)
        , lastAffectedRows_(0)
        , lastErrorCode_(0) {
        initializePool(poolSize);
    }

    AsyncPostgresClient::~AsyncPostgresClient() {
        // Clean up any pending queries
        while (!queryQueue_.empty()) {
            QueryContext* ctx = queryQueue_.front();
            queryQueue_.pop();
            delete ctx;
        }

        // Close all connections
        for (auto& conn : connectionPool_) {
            if (conn.activeQuery != nullptr) {
                delete conn.activeQuery;
            }
            closeConnection(conn.conn);
        }
    }

    std::string AsyncPostgresClient::buildConnectionString() const {
        std::ostringstream connStr;
        connStr << "host=" << config_.hostname << " ";
        connStr << "port=" << config_.port << " ";
        connStr << "dbname=" << config_.database << " ";

        if (!config_.username.empty()) {
            connStr << "user=" << config_.username << " ";
        }

        if (!config_.password.empty()) {
            connStr << "password=" << config_.password << " ";
        }

        return connStr.str();
    }

    PGconn* AsyncPostgresClient::createConnection() {
        std::string connStr = buildConnectionString();
        PGconn* conn = PQconnectdb(connStr.c_str());

        if (PQstatus(conn) != CONNECTION_OK) {
            std::string error = PQerrorMessage(conn);
            VLOG(0) << "PostgreSQL connection failed: " << error;
            PQfinish(conn);
            throw std::runtime_error("Failed to connect to PostgreSQL: " + error);
        }

        // Set connection to non-blocking mode for async operations
        if (PQsetnonblocking(conn, 1) != 0) {
            std::string error = PQerrorMessage(conn);
            VLOG(0) << "Failed to set non-blocking mode: " << error;
            PQfinish(conn);
            throw std::runtime_error("Failed to set non-blocking mode: " + error);
        }

        return conn;
    }

    void AsyncPostgresClient::closeConnection(PGconn* conn) {
        if (conn != nullptr) {
            PQfinish(conn);
        }
    }

    void AsyncPostgresClient::initializePool(size_t poolSize) {
        VLOG(1) << "Initializing PostgreSQL connection pool with " << poolSize << " connections";

        for (size_t i = 0; i < poolSize; ++i) {
            try {
                PGconn* conn = createConnection();
                connectionPool_.push_back({conn, nullptr, true});
                VLOG(2) << "Connection " << i << " created successfully";
            } catch (const std::exception& e) {
                VLOG(0) << "Failed to create connection " << i << ": " << e.what();
                // Clean up any connections we've already created
                for (auto& c : connectionPool_) {
                    closeConnection(c.conn);
                }
                connectionPool_.clear();
                throw;
            }
        }

        VLOG(1) << "PostgreSQL connection pool initialized with " << connectionPool_.size() << " connections";
    }

    AsyncPostgresClient::Connection* AsyncPostgresClient::getAvailableConnection() {
        for (auto& conn : connectionPool_) {
            if (conn.isAvailable) {
                return &conn;
            }
        }
        return nullptr;
    }

    std::vector<std::string> AsyncPostgresClient::convertParamsToStrings(const std::vector<nlohmann::json>& params) {
        std::vector<std::string> result;
        result.reserve(params.size());

        for (const auto& param : params) {
            if (param.is_null()) {
                result.push_back("");
            } else if (param.is_string()) {
                result.push_back(param.get<std::string>());
            } else if (param.is_number_integer()) {
                result.push_back(std::to_string(param.get<int64_t>()));
            } else if (param.is_number_float()) {
                result.push_back(std::to_string(param.get<double>()));
            } else if (param.is_boolean()) {
                result.push_back(param.get<bool>() ? "true" : "false");
            } else {
                result.push_back(param.dump());
            }
        }

        return result;
    }

    nlohmann::json AsyncPostgresClient::convertResultToJson(PGresult* res) {
        nlohmann::json result = nlohmann::json::array();

        int nRows = PQntuples(res);
        int nCols = PQnfields(res);

        for (int row = 0; row < nRows; ++row) {
            nlohmann::json rowObj = nlohmann::json::object();

            for (int col = 0; col < nCols; ++col) {
                const char* fieldName = PQfname(res, col);

                if (PQgetisnull(res, row, col)) {
                    rowObj[fieldName] = nullptr;
                } else {
                    const char* value = PQgetvalue(res, row, col);
                    Oid type = PQftype(res, col);

                    // Handle common PostgreSQL types
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

    void AsyncPostgresClient::exec(const std::string& query, SuccessCallback onSuccess, ErrorCallback onError) {
        QueryContext* context = new QueryContext{query, {}, onSuccess, nullptr, onError, 0, "", 0, false};

        Connection* conn = getAvailableConnection();

        if (conn != nullptr) {
            // Execute immediately on available connection
            executeQueryAsync(conn, context);
        } else {
            // No available connection, queue the query
            queryQueue_.push(context);
        }
    }

    void AsyncPostgresClient::exec(const std::string& query,
                                   QueryResultCallback onSuccess,
                                   ErrorCallback onError,
                                   const std::vector<nlohmann::json>& params) {
        QueryContext* context = new QueryContext{query, convertParamsToStrings(params), nullptr, onSuccess, onError, 0, "", 0, true};

        Connection* conn = getAvailableConnection();

        if (conn != nullptr) {
            // Execute immediately on available connection
            executeQueryAsync(conn, context);
        } else {
            // No available connection, queue the query
            queryQueue_.push(context);
        }
    }

    void AsyncPostgresClient::executeQueryAsync(Connection* connection, QueryContext* context) {
        if (connection->conn == nullptr || PQstatus(connection->conn) != CONNECTION_OK) {
            context->error = "Database connection not available";
            context->errorCode = -1;

            if (context->onError) {
                context->onError(context->error, context->errorCode);
            }

            delete context;
            return;
        }

        // Mark connection as busy
        connection->isAvailable = false;
        connection->activeQuery = context;

        // Send query asynchronously - non-blocking
        int sendResult;
        if (context->params.empty()) {
            sendResult = PQsendQuery(connection->conn, context->query.c_str());
        } else {
            std::vector<const char*> paramValues;
            for (const auto& p : context->params) {
                paramValues.push_back(p.c_str());
            }

            sendResult = PQsendQueryParams(connection->conn,
                                           context->query.c_str(),
                                           static_cast<int>(context->params.size()),
                                           nullptr,
                                           paramValues.data(),
                                           nullptr,
                                           nullptr,
                                           0);
        }

        if (sendResult == 0) {
            context->error = PQerrorMessage(connection->conn);
            context->errorCode = -1;

            if (context->onError) {
                context->onError(context->error, context->errorCode);
            }

            delete context;
            connection->activeQuery = nullptr;
            connection->isAvailable = true;
            return;
        }

        // Query has been sent successfully
        VLOG(2) << "Query sent on connection, waiting for results";
    }

    void AsyncPostgresClient::pollResults() {
        // Poll all active connections
        for (auto& conn : connectionPool_) {
            if (!conn.isAvailable && conn.activeQuery != nullptr) {
                handleConnectionResult(&conn);
            }
        }

        // Try to process queued queries if connections became available
        while (!queryQueue_.empty()) {
            Connection* availConn = getAvailableConnection();
            if (availConn == nullptr) {
                break; // No available connections, wait for next poll
            }

            QueryContext* ctx = queryQueue_.front();
            queryQueue_.pop();
            executeQueryAsync(availConn, ctx);
        }
    }

    void AsyncPostgresClient::handleConnectionResult(Connection* connection) {
        if (connection->conn == nullptr || connection->activeQuery == nullptr) {
            return;
        }

        // Consume input from the connection
        if (PQconsumeInput(connection->conn) == 0) {
            connection->activeQuery->error = PQerrorMessage(connection->conn);
            connection->activeQuery->errorCode = -1;

            if (connection->activeQuery->onError) {
                connection->activeQuery->onError(connection->activeQuery->error, connection->activeQuery->errorCode);
            }

            delete connection->activeQuery;
            connection->activeQuery = nullptr;
            connection->isAvailable = true;
            return;
        }

        // Check if connection is busy (still processing)
        if (PQisBusy(connection->conn) == 1) {
            // Results not ready yet, will be called again
            return;
        }

        // Get the result
        PGresult* res = PQgetResult(connection->conn);

        if (res == nullptr) {
            // No result yet or connection ready for next query
            // This happens after all results are consumed
            delete connection->activeQuery;
            connection->activeQuery = nullptr;
            connection->isAvailable = true;
            return;
        }

        ExecStatusType status = PQresultStatus(res);

        switch (status) {
            case PGRES_COMMAND_OK:
            case PGRES_TUPLES_OK: {
                // Query succeeded
                const char* affectedStr = PQcmdTuples(res);
                connection->activeQuery->affectedRows = (affectedStr && *affectedStr) ? std::atoi(affectedStr) : 0;
                connection->activeQuery->error.clear();
                connection->activeQuery->errorCode = 0;

                lastAffectedRows_ = connection->activeQuery->affectedRows;
                lastError_.clear();
                lastErrorCode_ = 0;

                if (connection->activeQuery->expectResults && connection->activeQuery->onResultSuccess) {
                    nlohmann::json result = convertResultToJson(res);
                    connection->activeQuery->onResultSuccess(result);
                } else if (connection->activeQuery->onSuccess) {
                    connection->activeQuery->onSuccess();
                }
                break;
            }

            case PGRES_BAD_RESPONSE:
            case PGRES_NONFATAL_ERROR:
            case PGRES_FATAL_ERROR: {
                // Query failed
                connection->activeQuery->error = PQresultErrorMessage(res);
                if (connection->activeQuery->error.empty()) {
                    connection->activeQuery->error = PQerrorMessage(connection->conn);
                }

                // Try to extract error code
                const char* sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
                if (sqlstate != nullptr) {
                    connection->activeQuery->errorCode = std::atoi(sqlstate);
                } else {
                    connection->activeQuery->errorCode = -1;
                }

                lastError_ = connection->activeQuery->error;
                lastErrorCode_ = connection->activeQuery->errorCode;

                if (connection->activeQuery->onError) {
                    connection->activeQuery->onError(connection->activeQuery->error, connection->activeQuery->errorCode);
                }
                break;
            }

            default: {
                connection->activeQuery->error = "Unexpected query result status: " + std::to_string(status);
                connection->activeQuery->errorCode = -2;

                lastError_ = connection->activeQuery->error;
                lastErrorCode_ = connection->activeQuery->errorCode;

                if (connection->activeQuery->onError) {
                    connection->activeQuery->onError(connection->activeQuery->error, connection->activeQuery->errorCode);
                }
                break;
            }
        }

        PQclear(res);

        // Consume any remaining results
        while ((res = PQgetResult(connection->conn)) != nullptr) {
            PQclear(res);
        }

        // Mark connection as available and clean up
        delete connection->activeQuery;
        connection->activeQuery = nullptr;
        connection->isAvailable = true;
    }

    void AsyncPostgresClient::affectedRows(AffectedRowsCallback onSuccess, ErrorAffectedRowsCallback onError) {
        if (lastErrorCode_ == 0 && onSuccess) {
            onSuccess(lastAffectedRows_);
        } else if (lastErrorCode_ != 0 && onError) {
            onError(lastError_, lastErrorCode_);
        }
    }

} // namespace mqtt::mqtt::lib

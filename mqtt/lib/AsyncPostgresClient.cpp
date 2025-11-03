#include "AsyncPostgresClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <log/Logger.h>
#include <sstream>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mqtt::mqtt::lib {

    AsyncPostgresClient::AsyncPostgresClient(const PostgresConfig& config, size_t poolSize)
        : config_(config)
        , lastAffectedRows_(0)
        , lastErrorCode_(0) {
        initializePool(poolSize);
    }

    AsyncPostgresClient::~AsyncPostgresClient() {
        // Clean up any pending queries in the queue
        while (!queryQueue_.empty()) {
            queryQueue_.pop();
        }

        // Connections will self-destruct via unobservedEvent() of snode.c event loop
        connectionPool_.clear();
    }

    void AsyncPostgresClient::initializePool(size_t poolSize) {
        VLOG(1) << "Initializing PostgreSQL connection pool with " << poolSize << " connections";

        for (size_t i = 0; i < poolSize; ++i) {
            try {
                AsyncPostgresConnection* conn = new AsyncPostgresConnection(config_);

                ConnectionWrapper wrapper{conn, true, false, i};
                connectionPool_.push_back(wrapper);

                size_t index = i;
                connectionPool_[index].connection->connectAsync([this, index](bool success, const std::string& errorMsg) {
                    if (success) {
                        VLOG(2) << "Connection " << index << " established successfully";
                        connectionPool_[index].connecting = false;
                        connectionPool_[index].available = true;

                        processNextQueuedQuery();
                    } else {
                        VLOG(0) << "Connection " << index << " failed: " << errorMsg;
                        connectionPool_[index].connecting = false;
                        connectionPool_[index].available = false;
                    }
                });

                VLOG(2) << "Connection " << i << " initialization started";
            } catch (const std::exception& e) {
                VLOG(0) << "Failed to create connection " << i << ": " << e.what();
            }
        }

        VLOG(1) << "PostgreSQL connection pool initialization started";
    }

    AsyncPostgresClient::ConnectionWrapper* AsyncPostgresClient::getAvailableConnection() {
        for (auto& wrapper : connectionPool_) {
            if (wrapper.available && !wrapper.connecting && wrapper.connection && wrapper.connection->isAvailable()) {
                return &wrapper;
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

    void AsyncPostgresClient::exec(const std::string& query, SuccessCallback onSuccess, ErrorCallback onError) {
        auto queryFunc = [query, onSuccess, onError, this](AsyncPostgresConnection* conn, ConnectionWrapper* wrapper) {
            VLOG(2) << "Sending query on connection " << wrapper->index << ": " << query;

            conn->executeQuery(
                query,
                [onSuccess, wrapper, this](nlohmann::json result) {
                    // Count affected rows from result
                    lastAffectedRows_ = static_cast<int>(result.size());
                    lastError_.clear();
                    lastErrorCode_ = 0;

                    wrapper->available = true;
                    processNextQueuedQuery();
                    if (onSuccess) {
                        onSuccess();
                    }
                },
                [onError, wrapper, this](const std::string& error, int errorCode) {
                    lastError_ = error;
                    lastErrorCode_ = errorCode;
                    lastAffectedRows_ = 0;

                    wrapper->available = true;
                    processNextQueuedQuery();
                    if (onError) {
                        onError(error, errorCode);
                    }
                },
                {} // No parameters
            );
        };

        ConnectionWrapper* wrapper = getAvailableConnection();

        if (wrapper != nullptr) {
            // Execute immediately on available connection
            wrapper->available = false;
            queryFunc(wrapper->connection, wrapper);
        } else {
            VLOG(2) << "No available connection, queueing query";
            queryQueue_.push([queryFunc](ConnectionWrapper* w) {
                queryFunc(w->connection, w);
            });
        }
    }

    void AsyncPostgresClient::exec(const std::string& query,
                                   QueryResultCallback onSuccess,
                                   ErrorCallback onError,
                                   const std::vector<nlohmann::json>& params) {
        std::vector<std::string> paramStrings = convertParamsToStrings(params);

        auto queryFunc = [query, paramStrings, onSuccess, onError, this](AsyncPostgresConnection* conn, ConnectionWrapper* wrapper) {
            VLOG(2) << "Sending query on connection " << wrapper->index << ": " << query;

            conn->executeQuery(
                query,
                [onSuccess, wrapper, this](nlohmann::json result) {
                    lastAffectedRows_ = static_cast<int>(result.size());
                    lastError_.clear();
                    lastErrorCode_ = 0;

                    wrapper->available = true;
                    processNextQueuedQuery();
                    if (onSuccess) {
                        onSuccess(result);
                    }
                },
                [onError, wrapper, this](const std::string& error, int errorCode) {
                    lastError_ = error;
                    lastErrorCode_ = errorCode;
                    lastAffectedRows_ = 0;

                    wrapper->available = true;
                    processNextQueuedQuery();
                    if (onError) {
                        onError(error, errorCode);
                    }
                },
                paramStrings);
        };

        ConnectionWrapper* wrapper = getAvailableConnection();

        if (wrapper != nullptr) {
            // Execute immediately on available connection
            wrapper->available = false;
            queryFunc(wrapper->connection, wrapper);
        } else {
            // No available connection, queue the query
            VLOG(2) << "No available connection, queueing query";
            queryQueue_.push([queryFunc](ConnectionWrapper* w) {
                queryFunc(w->connection, w);
            });
        }
    }

    void AsyncPostgresClient::processNextQueuedQuery() {
        if (queryQueue_.empty()) {
            return;
        }

        ConnectionWrapper* wrapper = getAvailableConnection();
        if (wrapper == nullptr) {
            return; // no available connection
        }
        auto queryFunc = queryQueue_.front();
        queryQueue_.pop();

        wrapper->available = false;
        queryFunc(wrapper);
    }

    void AsyncPostgresClient::affectedRows(AffectedRowsCallback onSuccess, ErrorAffectedRowsCallback onError) {
        if (lastErrorCode_ == 0 && onSuccess) {
            onSuccess(lastAffectedRows_);
        } else if (lastErrorCode_ != 0 && onError) {
            onError(lastError_, lastErrorCode_);
        }
    }

} // namespace mqtt::mqtt::lib

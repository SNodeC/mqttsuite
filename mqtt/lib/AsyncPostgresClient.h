#ifndef MQTT_LIB_ASYNCPOSTGRESCLIENT_H
#define MQTT_LIB_ASYNCPOSTGRESCLIENT_H

#include "AsyncPostgresConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <vector>
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mqtt::mqtt::lib {

    /**
     * Async PostgreSQL client with connection pooling
     *
     * Lets you run Postgres queries asynchronously. Multiple queries can run
     * at the same time using a pool of connections.
     *
     * Default pool size is 5 connections.
     */
    class AsyncPostgresClient {
    public:
        using SuccessCallback = std::function<void()>;
        using ErrorCallback = std::function<void(const std::string&, int)>;
        using QueryResultCallback = std::function<void(nlohmann::json)>;
        using AffectedRowsCallback = std::function<void(int)>;
        using ErrorAffectedRowsCallback = std::function<void(const std::string&, int)>;

        explicit AsyncPostgresClient(const PostgresConfig& config, size_t poolSize = 5);
        ~AsyncPostgresClient();

        AsyncPostgresClient(const AsyncPostgresClient&) = delete;
        AsyncPostgresClient& operator=(const AsyncPostgresClient&) = delete;
        AsyncPostgresClient(AsyncPostgresClient&&) = delete;
        AsyncPostgresClient& operator=(AsyncPostgresClient&&) = delete;

        // Execute a query asynchronously (non-blocking, returns immediately)
        void exec(const std::string& query, SuccessCallback onSuccess, ErrorCallback onError);

        // Execute a parameterized query with results
        void exec(const std::string& query,
                  QueryResultCallback onSuccess,
                  ErrorCallback onError,
                  const std::vector<nlohmann::json>& params = {});

        // Get affected rows after a successful query
        void affectedRows(AffectedRowsCallback onSuccess, ErrorAffectedRowsCallback onError);

    private:
        struct ConnectionWrapper {
            AsyncPostgresConnection* connection;
            bool connecting;
            bool available;
            size_t index; // for testing/logging which connection is used
        };

        void initializePool(size_t poolSize);
        ConnectionWrapper* getAvailableConnection();
        std::vector<std::string> convertParamsToStrings(const std::vector<nlohmann::json>& params);
        void processNextQueuedQuery();

        PostgresConfig config_;
        std::vector<ConnectionWrapper> connectionPool_;
        std::queue<std::function<void(ConnectionWrapper*)>> queryQueue_;
        int lastAffectedRows_;
        std::string lastError_;
        int lastErrorCode_;
    };

} // namespace mqtt::mqtt::lib

#endif // MQTT_LIB_ASYNCPOSTGRESCLIENT_H
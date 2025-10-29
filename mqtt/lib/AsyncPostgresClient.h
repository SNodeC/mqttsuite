#ifndef MQTT_LIB_ASYNCPOSTGRESCLIENT_H
#define MQTT_LIB_ASYNCPOSTGRESCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <functional>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <vector>
#endif

namespace mqtt::mqtt::lib {

struct PostgresConfig {
    std::string hostname = "localhost";
    std::string username;
    std::string password;
    std::string database;
    uint16_t port = 5432;
};

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

    // Check and process async query results for all active connections
    void pollResults();

private:
    struct QueryContext {
        std::string query;
        std::vector<std::string> params;
        SuccessCallback onSuccess;
        QueryResultCallback onResultSuccess;
        ErrorCallback onError;
        int affectedRows;
        std::string error;
        int errorCode;
        bool expectResults;
    };

    struct Connection {
        PGconn* conn;
        QueryContext* activeQuery;
        bool isAvailable;
    };

    PGconn* createConnection();
    void closeConnection(PGconn* conn);
    void initializePool(size_t poolSize);
    Connection* getAvailableConnection();
    void executeQueryAsync(Connection* connection, QueryContext* context);
    void handleConnectionResult(Connection* connection);
    std::string buildConnectionString() const;
    std::vector<std::string> convertParamsToStrings(const std::vector<nlohmann::json>& params);
    nlohmann::json convertResultToJson(PGresult* res);

    PostgresConfig config_;
    std::vector<Connection> connectionPool_;
    std::queue<QueryContext*> queryQueue_;
    int lastAffectedRows_;
    std::string lastError_;
    int lastErrorCode_;
};

} // namespace mqtt::mqtt::lib

#endif // MQTT_LIB_ASYNCPOSTGRESCLIENT_H
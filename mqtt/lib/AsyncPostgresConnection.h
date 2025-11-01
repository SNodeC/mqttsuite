#ifndef MQTT_LIB_ASYNCPOSTGRESCONNECTION_H
#define MQTT_LIB_ASYNCPOSTGRESCONNECTION_H

#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"

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
        // We need to avoid DNS because it would block the event loop
        // see: https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTSTARTPARAMS
        // "The hostaddr parameter must be used appropriately to prevent DNS queries from being made."
        std::string hostaddr = "127.0.0.1";
        std::string username;
        std::string password;
        std::string database;
        uint16_t port = 5432;
    };

    class AsyncPostgresConnection
        : private core::eventreceiver::ReadEventReceiver
        , private core::eventreceiver::WriteEventReceiver {
    public:
        using ConnectCallback = std::function<void(bool success, const std::string& errorMsg)>;
        using QueryResultCallback = std::function<void(nlohmann::json result)>;
        using ErrorCallback = std::function<void(const std::string& error, int errorCode)>;

        struct QueryContext {
            std::string query;
            std::vector<std::string> params;
            QueryResultCallback onSuccess;
            ErrorCallback onError;
            bool expectResults;
        };

        explicit AsyncPostgresConnection(const PostgresConfig& config);
        ~AsyncPostgresConnection() override;

        AsyncPostgresConnection(const AsyncPostgresConnection&) = delete;
        AsyncPostgresConnection& operator=(const AsyncPostgresConnection&) = delete;
        AsyncPostgresConnection(AsyncPostgresConnection&&) = delete;
        AsyncPostgresConnection& operator=(AsyncPostgresConnection&&) = delete;

        void connectAsync(ConnectCallback callback);

        void executeQuery(const std::string& query,
                          QueryResultCallback onSuccess,
                          ErrorCallback onError,
                          const std::vector<std::string>& params = {});

        // Check if connection is established and ready
        bool isConnected() const {
            return connected_;
        }

        // Check if connection is available (not busy with a query)
        bool isAvailable() const {
            return connected_ && currentQuery_ == nullptr;
        }

    protected:
        void readEvent() override;
        void readTimeout() override;

        void writeEvent() override;
        void writeTimeout() override;

        void unobservedEvent() override;

    private:
        enum class State { DISCONNECTED, CONNECTING, CONNECTED, SENDING_QUERY, FLUSHING, READING_RESULT, ERROR };
        void handleConnectPoll();
        void handleFlush();
        void handleReadResult();

        void processResult(PGresult* result);

        nlohmann::json convertResultToJson(PGresult* res);

        void cleanup();

        void reportError(const std::string& error, int code);

        PostgresConfig config_;
        PGconn* conn_ = nullptr;
        State state_ = State::DISCONNECTED;
        bool connected_ = false;
        int fd_ = -1;

        ConnectCallback connectCallback_;
        QueryContext* currentQuery_ = nullptr;
        std::queue<QueryContext*> queryQueue_;
    };

} // namespace mqtt::mqtt::lib

#endif // MQTT_LIB_ASYNCPOSTGRESCONNECTION_H

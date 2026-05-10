/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#ifndef MQTTSTORE_LIB_CONFIGSECTIONS_H
#define MQTTSTORE_LIB_CONFIGSECTIONS_H

#include <net/config/ConfigSection.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <string_view>

#endif

namespace mqtt::mqttstore::lib {

    class ConfigDatabase : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"db"};
        constexpr static std::string_view DESCRIPTION{"Configuration for the database connection (MariaDB/MySQL)"};

        ConfigDatabase(utils::SubCommand* parent);
        ~ConfigDatabase() override;

        [[nodiscard]] std::string getHost() const;
        const ConfigDatabase& setHost(const std::string& host);

        [[nodiscard]] std::string getUsername() const;
        const ConfigDatabase& setUsername(const std::string& username);

        [[nodiscard]] std::string getPassword() const;
        const ConfigDatabase& setPassword(const std::string& password);

        [[nodiscard]] std::string getDatabase() const;
        const ConfigDatabase& setDatabase(const std::string& database);

        [[nodiscard]] uint16_t getPort() const;
        const ConfigDatabase& setPort(uint16_t port);

        [[nodiscard]] std::string getSocket() const;
        const ConfigDatabase& setSocket(const std::string& socket);

        [[nodiscard]] uint32_t getFlags() const;
        const ConfigDatabase& setFlags(uint32_t flags);

    private:
        CLI::Option* hostOpt;
        CLI::Option* usernameOpt;
        CLI::Option* passwordOpt;
        CLI::Option* databaseOpt;
        CLI::Option* portOpt;
        CLI::Option* socketOpt;
        CLI::Option* flagsOpt;
    };

    class ConfigSession : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"session"};
        constexpr static std::string_view DESCRIPTION{"MQTT session behavior"};

        ConfigSession(utils::SubCommand* parent);
        ~ConfigSession() override;

        [[nodiscard]] std::string getClientId() const;
        const ConfigSession& setClientId(const std::string& clientId) const;

        [[nodiscard]] uint8_t getQoS() const;
        const ConfigSession& setQos(uint8_t qoS) const;

        [[nodiscard]] bool getRetainSession() const;
        const ConfigSession& setRetainSession(bool retainSession) const;

        [[nodiscard]] uint16_t getKeepAlive() const;
        const ConfigSession& setKeepAlive(uint16_t keepAlive) const;

        [[nodiscard]] std::string getWillTopic() const;
        const ConfigSession& setWillTopic(const std::string& willTopic) const;

        [[nodiscard]] std::string getWillMessage() const;
        const ConfigSession& setWillMessage(const std::string& willMessage) const;

        [[nodiscard]] uint8_t getWillQoS() const;
        const ConfigSession& setWillQoS(uint8_t willQoS) const;

        [[nodiscard]] bool getWillRetain() const;
        const ConfigSession& setWillRetain(bool willRetain) const;

        [[nodiscard]] std::string getUsername() const;
        const ConfigSession& setUsername(const std::string& username) const;

        [[nodiscard]] std::string getPassword() const;
        const ConfigSession& setPassword(const std::string& password) const;

    private:
        CLI::Option* clientIdOpt;
        CLI::Option* qoSOpt;
        CLI::Option* retainSessionOpt;
        CLI::Option* keepAliveOpt;
        CLI::Option* willTopicOpt;
        CLI::Option* willMessageOpt;
        CLI::Option* willQoSOpt;
        CLI::Option* willRetainOpt;
        CLI::Option* usernameOpt;
        CLI::Option* passwordOpt;
    };

    class ConfigSubscribe : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"sub"};
        constexpr static std::string_view DESCRIPTION{"MQTT subscriptions to persist"};

        ConfigSubscribe(utils::SubCommand* parent);
        ~ConfigSubscribe() override;

        [[nodiscard]] std::list<std::string> getTopic() const;
        const ConfigSubscribe& setTopic(const std::string& topic);

    private:
        CLI::Option* topicOpt;
    };

    class ConfigStorage : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"storage"};
        constexpr static std::string_view DESCRIPTION{"Generic raw MQTT message storage"};

        ConfigStorage(utils::SubCommand* parent);
        ~ConfigStorage() override;

        [[nodiscard]] std::string getTable() const;
        const ConfigStorage& setTable(const std::string& table);

        [[nodiscard]] bool getAutoCreate() const;
        const ConfigStorage& setAutoCreate(bool autoCreate);

        [[nodiscard]] bool getStorePayloadText() const;
        const ConfigStorage& setStorePayloadText(bool storePayloadText);

        [[nodiscard]] bool getStorePayloadJson() const;
        const ConfigStorage& setStorePayloadJson(bool storePayloadJson);

    private:
        CLI::Option* tableOpt;
        CLI::Option* autoCreateOpt;
        CLI::Option* storePayloadTextOpt;
        CLI::Option* storePayloadJsonOpt;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_CONFIGSECTIONS_H

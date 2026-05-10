/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
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

    class ConfigSubscribe : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"sub"};
        constexpr static std::string_view DESCRIPTION{"MQTT subscriptions for messages to persist"};

        ConfigSubscribe(utils::SubCommand* parent);
        ~ConfigSubscribe() override;

        [[nodiscard]] std::list<std::string> getTopic() const;
        const ConfigSubscribe& setTopic(const std::string& topic);

    private:
        CLI::Option* topicOpt;
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
        const ConfigSession& settWillQoS(uint8_t willQoS) const;

        [[nodiscard]] bool getWillRetain() const;
        const ConfigSession& setWillRetain(bool willRetain) const;

        [[nodiscard]] std::string getUsername() const;
        const ConfigSession& settUsername(const std::string& username) const;

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

    class ConfigDatabase : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"db"};
        constexpr static std::string_view DESCRIPTION{"MariaDB/MySQL connection for MQTT persistence"};

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

    class ConfigStorage : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"storage"};
        constexpr static std::string_view DESCRIPTION{"Generic MQTT raw storage behavior"};

        ConfigStorage(utils::SubCommand* parent);
        ~ConfigStorage() override;

        [[nodiscard]] std::string getTable() const;
        const ConfigStorage& setTable(const std::string& table);

        [[nodiscard]] bool getCreateSchema() const;
        const ConfigStorage& setCreateSchema(bool createSchema);

        [[nodiscard]] bool getProjectJson() const;
        const ConfigStorage& setProjectJson(bool projectJson);

    private:
        CLI::Option* tableOpt;
        CLI::Option* createSchemaOpt;
        CLI::Option* projectJsonOpt;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_CONFIGSECTIONS_H

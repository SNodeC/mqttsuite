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
#include <string_view>

#endif

namespace mqtt::mqttstore::lib {

    class ConfigSession : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"session"};
        constexpr static std::string_view DESCRIPTION{"MQTT session behavior"};

        ConfigSession(utils::SubCommand* parent);
        ~ConfigSession() override;

        [[nodiscard]] std::string getClientId() const;
        [[nodiscard]] std::uint8_t getQoS() const;
        [[nodiscard]] bool getRetainSession() const;
        [[nodiscard]] std::uint16_t getKeepAlive() const;
        [[nodiscard]] std::string getWillTopic() const;
        [[nodiscard]] std::string getWillMessage() const;
        [[nodiscard]] std::uint8_t getWillQoS() const;
        [[nodiscard]] bool getWillRetain() const;
        [[nodiscard]] std::string getUsername() const;
        [[nodiscard]] std::string getPassword() const;
        [[nodiscard]] std::string getSessionStore() const;

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
        CLI::Option* sessionStoreOpt;
    };

    class ConfigSubscribe : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"sub"};
        constexpr static std::string_view DESCRIPTION{"MQTT subscriptions to persist"};

        ConfigSubscribe(utils::SubCommand* parent);
        ~ConfigSubscribe() override;

        [[nodiscard]] std::list<std::string> getTopic() const;

    private:
        CLI::Option* topicOpt;
    };

    class ConfigDatabase : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"db"};
        constexpr static std::string_view DESCRIPTION{"MariaDB/MySQL connection"};

        ConfigDatabase(utils::SubCommand* parent);
        ~ConfigDatabase() override;

        [[nodiscard]] std::string getHost() const;
        [[nodiscard]] std::string getUsername() const;
        [[nodiscard]] std::string getPassword() const;
        [[nodiscard]] std::string getDatabase() const;
        [[nodiscard]] std::uint16_t getPort() const;
        [[nodiscard]] std::string getSocket() const;
        [[nodiscard]] std::uint32_t getFlags() const;

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
        constexpr static std::string_view DESCRIPTION{"Generic MQTT persistence behavior"};

        ConfigStorage(utils::SubCommand* parent);
        ~ConfigStorage() override;

        [[nodiscard]] std::string getRawTable() const;
        [[nodiscard]] bool getAutoCreateRawTable() const;
        [[nodiscard]] std::string getProjectionFile() const;

    private:
        CLI::Option* rawTableOpt;
        CLI::Option* autoCreateRawTableOpt;
        CLI::Option* projectionFileOpt;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_CONFIGSECTIONS_H

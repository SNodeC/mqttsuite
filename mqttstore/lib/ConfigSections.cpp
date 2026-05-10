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

#include "ConfigSections.h"

namespace mqtt::mqttstore::lib {

    ConfigSession::ConfigSession(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "MQTT session")
        , clientIdOpt(setConfigurable(addOption("--client-id", "MQTT Client-ID", "string", CLI::TypeValidator<std::string>()), true))
        , qoSOpt(setConfigurable(addOption("--qos", "Default subscription quality of service", "uint8_t", "0", CLI::Range(0, 2)), true))
        , retainSessionOpt(setConfigurable(addFlag("--retain-session{true},-r{true}",
                                                   "Use a persistent MQTT session (sets clean_session=false)",
                                                   "bool",
                                                   "false",
                                                   CLI::IsMember({"true", "false"})),
                                           true)
                               ->needs(clientIdOpt))
        , keepAliveOpt(setConfigurable(
              addOption("--keep-alive", "MQTT keep alive in seconds", "uint16_t", "60", CLI::TypeValidator<std::uint16_t>()), true))
        , willTopicOpt(setConfigurable(addOption("--will-topic", "MQTT will topic", "string", CLI::TypeValidator<std::string>()), true))
        , willMessageOpt(
              setConfigurable(addOption("--will-message", "MQTT will message", "string", CLI::TypeValidator<std::string>()), true))
        , willQoSOpt(setConfigurable(addOption("--will-qos", "MQTT will quality of service", "uint8_t", "0", CLI::Range(0, 2)), true))
        , willRetainOpt(setConfigurable(
              addFlag("--will-retain{true}", "MQTT will message retain", "bool", "false", CLI::IsMember({"true", "false"})), true))
        , usernameOpt(setConfigurable(addOption("--username", "MQTT username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "MQTT password", "string", CLI::TypeValidator<std::string>()), true))
        , sessionStoreOpt(setConfigurable(
              addOption("--session-store", "Path to persistent MQTT session store", "filename", !CLI::ExistingDirectory), true)) {
    }

    ConfigSession::~ConfigSession() = default;

    std::string ConfigSession::getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    std::uint8_t ConfigSession::getQoS() const {
        return qoSOpt->as<std::uint8_t>();
    }

    bool ConfigSession::getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    std::uint16_t ConfigSession::getKeepAlive() const {
        return keepAliveOpt->as<std::uint16_t>();
    }

    std::string ConfigSession::getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    std::string ConfigSession::getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    std::uint8_t ConfigSession::getWillQoS() const {
        return willQoSOpt->as<std::uint8_t>();
    }

    bool ConfigSession::getWillRetain() const {
        return willRetainOpt->as<bool>();
    }

    std::string ConfigSession::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    std::string ConfigSession::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    std::string ConfigSession::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    ConfigSubscribe::ConfigSubscribe(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "MQTT subscriptions")
        , topicOpt(setConfigurable(addOption("--topic",
                                             "Topic filter to persist; append ##<qos> to override QoS",
                                             "string",
                                             CLI::TypeValidator<std::string>()),
                                   true)
                       ->take_all()) {
        required(topicOpt);
    }

    ConfigSubscribe::~ConfigSubscribe() = default;

    std::list<std::string> ConfigSubscribe::getTopic() const {
        std::list<std::string> topicList = topicOpt->as<std::list<std::string>>();

        if (!topicList.empty() && topicList.front().empty()) {
            topicList.pop_front();
        }

        return topicList;
    }

    ConfigDatabase::ConfigDatabase(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Database connection")
        , hostOpt(setConfigurable(
              addOption("--host", "Hostname or IP address of MariaDB server", "hostname|IPv4", CLI::TypeValidator<std::string>()), true))
        , usernameOpt(setConfigurable(addOption("--username", "Database username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "Database password", "string", CLI::TypeValidator<std::string>()), true))
        , databaseOpt(setConfigurable(addOption("--database", "Database name", "string", CLI::TypeValidator<std::string>()), true))
        , portOpt(setConfigurable(addOption("--port", "Database TCP port", "uint16_t", 3306, CLI::TypeValidator<std::uint16_t>()), true))
        , socketOpt(setConfigurable(addOption("--socket",
                                              "Database socket file (overrides host and port when set)",
                                              "string",
                                              "/run/mysqld/mysqld.sock",
                                              CLI::TypeValidator<std::string>()),
                                    true))
        , flagsOpt(setConfigurable(addOption("--flags", "MariaDB connection flags", "uint32_t", "0", CLI::TypeValidator<std::uint32_t>()),
                                   true)) {
    }

    ConfigDatabase::~ConfigDatabase() = default;

    std::string ConfigDatabase::getHost() const {
        return hostOpt->as<std::string>();
    }

    std::string ConfigDatabase::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    std::string ConfigDatabase::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    std::string ConfigDatabase::getDatabase() const {
        return databaseOpt->as<std::string>();
    }

    std::uint16_t ConfigDatabase::getPort() const {
        return portOpt->as<std::uint16_t>();
    }

    std::string ConfigDatabase::getSocket() const {
        return socketOpt->as<std::string>();
    }

    std::uint32_t ConfigDatabase::getFlags() const {
        return flagsOpt->as<std::uint32_t>();
    }

    ConfigStorage::ConfigStorage(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Storage behavior")
        , rawTableOpt(setConfigurable(
              addOption("--raw-table", "Raw MQTT envelope table", "identifier", "mqtt_messages", CLI::TypeValidator<std::string>()), true))
        , autoCreateRawTableOpt(setConfigurable(addFlag("--auto-create-raw-table{true}",
                                                        "Create the raw MQTT envelope table automatically",
                                                        "bool",
                                                        "true",
                                                        CLI::IsMember({"true", "false"})),
                                                true))
        , projectionFileOpt(setConfigurable(
              addOption("--projection-file", "Optional JSON file describing typed projections", "filename", !CLI::ExistingDirectory),
              true)) {
    }

    ConfigStorage::~ConfigStorage() = default;

    std::string ConfigStorage::getRawTable() const {
        return rawTableOpt->as<std::string>();
    }

    bool ConfigStorage::getAutoCreateRawTable() const {
        return autoCreateRawTableOpt->as<bool>();
    }

    std::string ConfigStorage::getProjectionFile() const {
        return projectionFileOpt->as<std::string>();
    }

} // namespace mqtt::mqttstore::lib

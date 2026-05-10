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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif

namespace mqtt::mqttstore::lib {

    ConfigDatabase::ConfigDatabase(utils::SubCommand* parent)
        : SubCommand(parent, this, "Database connection")
        , hostOpt(setConfigurable(
              addOption("--host", "Hostname or IP address of the database server", "hostname|IPv4", CLI::TypeValidator<std::string>()),
              true))
        , usernameOpt(setConfigurable(addOption("--username", "Database username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "Database password", "string", CLI::TypeValidator<std::string>()), true))
        , databaseOpt(setConfigurable(addOption("--database", "Database name", "string", CLI::TypeValidator<std::string>()), true))
        , portOpt(setConfigurable(addOption("--port", "Database port", "uint16_t", 3306, CLI::TypeValidator<uint16_t>()), true))
        , socketOpt(setConfigurable(addOption("--socket",
                                              "Socket file for local database connections (overrides hostname and port)",
                                              "string",
                                              "/run/mysqld/mysqld.sock",
                                              CLI::TypeValidator<std::string>()),
                                    true))
        , flagsOpt(
              setConfigurable(addOption("--flags", "MariaDB connection flags", "uint32_t", "0", CLI::TypeValidator<uint32_t>()), true)) {
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

    uint16_t ConfigDatabase::getPort() const {
        return portOpt->as<uint16_t>();
    }

    std::string ConfigDatabase::getSocket() const {
        return socketOpt->as<std::string>();
    }

    uint32_t ConfigDatabase::getFlags() const {
        return flagsOpt->as<uint32_t>();
    }

    ConfigSession::ConfigSession(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "MQTT session")
        , clientIdOpt(setConfigurable(addOption("--client-id", "MQTT client id", "string", CLI::TypeValidator<std::string>()), true))
        , qoSOpt(setConfigurable(addOption("--qos", "Default subscription QoS", "uint8_t", "0", CLI::Range(0, 2)), true))
        , retainSessionOpt(setConfigurable(addFlag("--retain-session{true},-r{true}",
                                                   "Use a persistent MQTT session",
                                                   "bool",
                                                   "false",
                                                   CLI::IsMember({"true", "false"})),
                                           true)
                               ->needs(clientIdOpt))
        , keepAliveOpt(setConfigurable(
              addOption("--keep-alive", "MQTT keep alive in seconds", "uint16_t", "60", CLI::TypeValidator<uint16_t>()), true))
        , willTopicOpt(setConfigurable(addOption("--will-topic", "MQTT will topic", "string", CLI::TypeValidator<std::string>()), true))
        , willMessageOpt(
              setConfigurable(addOption("--will-message", "MQTT will message", "string", CLI::TypeValidator<std::string>()), true))
        , willQoSOpt(setConfigurable(addOption("--will-qos", "MQTT will QoS", "uint8_t", "0", CLI::Range(0, 2)), true))
        , willRetainOpt(setConfigurable(
              addFlag("--will-retain{true}", "Retain MQTT will message", "bool", "false", CLI::IsMember({"true", "false"})), true))
        , usernameOpt(setConfigurable(addOption("--username", "MQTT username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "MQTT password", "string", CLI::TypeValidator<std::string>()), true)) {
    }

    ConfigSession::~ConfigSession() = default;

    std::string ConfigSession::getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    uint8_t ConfigSession::getQoS() const {
        return qoSOpt->as<uint8_t>();
    }

    bool ConfigSession::getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    uint16_t ConfigSession::getKeepAlive() const {
        return keepAliveOpt->as<uint16_t>();
    }

    std::string ConfigSession::getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    std::string ConfigSession::getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    uint8_t ConfigSession::getWillQoS() const {
        return willQoSOpt->as<uint8_t>();
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

    ConfigStore::ConfigStore(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Storage")
        , topicOpt(setConfigurable(addOption("--topic",
                                             "MQTT topic filters to persist; append ##<qos> to override QoS",
                                             "string",
                                             "#",
                                             CLI::TypeValidator<std::string>()),
                                   true)
                       ->take_all())
        , rawTableOpt(setConfigurable(
              addOption("--raw-table", "Table for raw MQTT envelopes", "string", "mqtt_messages", CLI::TypeValidator<std::string>()), true))
        , fieldTableOpt(setConfigurable(addOption("--field-table",
                                                  "Table for flattened JSON scalar projections",
                                                  "string",
                                                  "mqtt_message_fields",
                                                  CLI::TypeValidator<std::string>()),
                                        true))
        , autoCreateOpt(setConfigurable(
              addFlag("--auto-create{true}", "Create generic storage tables on startup", "bool", "true", CLI::IsMember({"true", "false"})),
              true))
        , flattenJsonOpt(setConfigurable(addFlag("--flatten-json{true}",
                                                 "Project JSON scalar leaves into the field table",
                                                 "bool",
                                                 "true",
                                                 CLI::IsMember({"true", "false"})),
                                         true))
        , maxPayloadBytesOpt(setConfigurable(addOption("--max-payload-bytes",
                                                       "Maximum stored payload size in bytes; 0 disables the limit",
                                                       "size_t",
                                                       "0",
                                                       CLI::TypeValidator<std::size_t>()),
                                             true)) {
        required(topicOpt);
    }

    ConfigStore::~ConfigStore() = default;

    std::list<std::string> ConfigStore::getTopics() const {
        std::list<std::string> topicList = topicOpt->as<std::list<std::string>>();

        if (!topicList.empty() && topicList.front().empty()) {
            topicList.pop_front();
        }

        return topicList;
    }

    std::string ConfigStore::getRawTable() const {
        return rawTableOpt->as<std::string>();
    }

    std::string ConfigStore::getFieldTable() const {
        return fieldTableOpt->as<std::string>();
    }

    bool ConfigStore::getAutoCreate() const {
        return autoCreateOpt->as<bool>();
    }

    bool ConfigStore::getFlattenJson() const {
        return flattenJsonOpt->as<bool>();
    }

    std::size_t ConfigStore::getMaxPayloadBytes() const {
        return maxPayloadBytesOpt->as<std::size_t>();
    }

} // namespace mqtt::mqttstore::lib

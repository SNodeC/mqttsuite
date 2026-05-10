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

    ConfigSubscribe::ConfigSubscribe(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications (at least one required)")
        , topicOpt(
              setConfigurable(addOption("--topic", "List of MQTT topics to persist", "string", CLI::TypeValidator<std::string>()), true)
                  ->take_all()) {
        required(topicOpt);
        forceUnrequired(true);
    }

    ConfigSubscribe::~ConfigSubscribe() = default;

    std::list<std::string> ConfigSubscribe::getTopic() const {
        std::list<std::string> topicList = topicOpt->as<std::list<std::string>>();

        if (!topicList.empty() && topicList.front().empty()) {
            topicList.pop_front();
        }

        return topicList;
    }

    const ConfigSubscribe& ConfigSubscribe::setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return *this;
    }

    ConfigSession::ConfigSession(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications")
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
              addOption("--keep-alive", "MQTT keep-alive in seconds", "uint16_t", "60", CLI::TypeValidator<uint16_t>()), true))
        , willTopicOpt(setConfigurable(addOption("--will-topic", "MQTT will topic", "string", CLI::TypeValidator<std::string>()), true))
        , willMessageOpt(
              setConfigurable(addOption("--will-message", "MQTT will message", "string", CLI::TypeValidator<std::string>()), true))
        , willQoSOpt(setConfigurable(addOption("--will-qos", "MQTT will quality of service", "uint8_t", "0", CLI::Range(0, 2)), true))
        , willRetainOpt(setConfigurable(
              addFlag("--will-retain{true}", "MQTT will message retain", "bool", "false", CLI::IsMember({"true", "false"})), true))
        , usernameOpt(setConfigurable(addOption("--username", "MQTT username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "MQTT password", "string", CLI::TypeValidator<std::string>()), true)) {
    }

    ConfigSession::~ConfigSession() = default;

    std::string ConfigSession::getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setClientId(const std::string& clientId) const {
        clientIdOpt->default_val(clientId)->clear();
        return *this;
    }

    uint8_t ConfigSession::getQoS() const {
        return qoSOpt->as<uint8_t>();
    }

    const ConfigSession& ConfigSession::setQos(uint8_t qoS) const {
        qoSOpt->default_val(qoS)->clear();
        return *this;
    }

    bool ConfigSession::getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    const ConfigSession& ConfigSession::setRetainSession(bool retainSession) const {
        retainSessionOpt->default_val(retainSession)->clear();
        return *this;
    }

    uint16_t ConfigSession::getKeepAlive() const {
        return keepAliveOpt->as<uint16_t>();
    }

    const ConfigSession& ConfigSession::setKeepAlive(uint16_t keepAlive) const {
        keepAliveOpt->default_val(keepAlive)->clear();
        return *this;
    }

    std::string ConfigSession::getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setWillTopic(const std::string& willTopic) const {
        willTopicOpt->default_val(willTopic)->clear();
        return *this;
    }

    std::string ConfigSession::getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setWillMessage(const std::string& willMessage) const {
        willMessageOpt->default_val(willMessage)->clear();
        return *this;
    }

    uint8_t ConfigSession::getWillQoS() const {
        return willQoSOpt->as<uint8_t>();
    }

    const ConfigSession& ConfigSession::settWillQoS(uint8_t willQoS) const {
        willQoSOpt->default_val(willQoS)->clear();
        return *this;
    }

    bool ConfigSession::getWillRetain() const {
        return willRetainOpt->as<bool>();
    }

    const ConfigSession& ConfigSession::setWillRetain(bool willRetain) const {
        willRetainOpt->default_val(willRetain)->clear();
        return *this;
    }

    std::string ConfigSession::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::settUsername(const std::string& username) const {
        usernameOpt->default_val(username)->clear();
        return *this;
    }

    std::string ConfigSession::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setPassword(const std::string& password) const {
        passwordOpt->default_val(password)->clear();
        return *this;
    }

    ConfigDatabase::ConfigDatabase(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Database connection")
        , hostOpt(setConfigurable(
              addOption("--host", "Hostname or IP address of MariaDB server", "hostname|IPv4", CLI::TypeValidator<std::string>()), true))
        , usernameOpt(setConfigurable(addOption("--username", "Database username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(addOption("--password", "Database password", "string", CLI::TypeValidator<std::string>()), true))
        , databaseOpt(setConfigurable(addOption("--database", "Database name", "string", CLI::TypeValidator<std::string>()), true))
        , portOpt(setConfigurable(addOption("--port", "Database port", "uint16_t", 3306, CLI::TypeValidator<uint16_t>()), true))
        , socketOpt(setConfigurable(addOption("--socket",
                                              "Socket file for database connection (overrides hostname and port)",
                                              "string",
                                              "/run/mysqld/mysqld.sock",
                                              CLI::TypeValidator<std::string>()),
                                    true))
        , flagsOpt(setConfigurable(addOption("--flags",
                                             "Connection flags for database connection (see MariaDB connection flags)",
                                             "uint32_t",
                                             "0",
                                             CLI::TypeValidator<uint32_t>()),
                                   true)) {
    }

    ConfigDatabase::~ConfigDatabase() = default;

    std::string ConfigDatabase::getHost() const {
        return hostOpt->as<std::string>();
    }

    const ConfigDatabase& ConfigDatabase::setHost(const std::string& host) {
        hostOpt->default_val(host)->clear();
        return *this;
    }

    std::string ConfigDatabase::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    const ConfigDatabase& ConfigDatabase::setUsername(const std::string& username) {
        usernameOpt->default_val(username)->clear();
        return *this;
    }

    std::string ConfigDatabase::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    const ConfigDatabase& ConfigDatabase::setPassword(const std::string& password) {
        passwordOpt->default_val(password)->clear();
        return *this;
    }

    std::string ConfigDatabase::getDatabase() const {
        return databaseOpt->as<std::string>();
    }

    const ConfigDatabase& ConfigDatabase::setDatabase(const std::string& database) {
        databaseOpt->default_val(database)->clear();
        return *this;
    }

    uint16_t ConfigDatabase::getPort() const {
        return portOpt->as<uint16_t>();
    }

    const ConfigDatabase& ConfigDatabase::setPort(uint16_t port) {
        portOpt->default_val(port)->clear();
        return *this;
    }

    std::string ConfigDatabase::getSocket() const {
        return socketOpt->as<std::string>();
    }

    const ConfigDatabase& ConfigDatabase::setSocket(const std::string& socket) {
        socketOpt->default_val(socket)->clear();
        return *this;
    }

    uint32_t ConfigDatabase::getFlags() const {
        return flagsOpt->as<uint32_t>();
    }

    const ConfigDatabase& ConfigDatabase::setFlags(uint32_t flags) {
        flagsOpt->default_val(flags)->clear();
        return *this;
    }

    ConfigStorage::ConfigStorage(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Storage")
        , tableOpt(setConfigurable(
              addOption("--table", "Raw MQTT message table", "string", "mqtt_messages", CLI::TypeValidator<std::string>()), true))
        , createSchemaOpt(setConfigurable(addFlag("--create-schema{true}",
                                                  "Create the raw MQTT message table if it does not exist",
                                                  "bool",
                                                  "true",
                                                  CLI::IsMember({"true", "false"})),
                                          true))
        , projectJsonOpt(setConfigurable(addFlag("--project-json{true}",
                                                 "Project scalar JSON fields into an auxiliary key/value table",
                                                 "bool",
                                                 "true",
                                                 CLI::IsMember({"true", "false"})),
                                         true)) {
    }

    ConfigStorage::~ConfigStorage() = default;

    std::string ConfigStorage::getTable() const {
        return tableOpt->as<std::string>();
    }

    const ConfigStorage& ConfigStorage::setTable(const std::string& table) {
        tableOpt->default_val(table)->clear();
        return *this;
    }

    bool ConfigStorage::getCreateSchema() const {
        return createSchemaOpt->as<bool>();
    }

    const ConfigStorage& ConfigStorage::setCreateSchema(bool createSchema) {
        createSchemaOpt->default_val(createSchema)->clear();
        return *this;
    }

    bool ConfigStorage::getProjectJson() const {
        return projectJsonOpt->as<bool>();
    }

    const ConfigStorage& ConfigStorage::setProjectJson(bool projectJson) {
        projectJsonOpt->default_val(projectJson)->clear();
        return *this;
    }

} // namespace mqtt::mqttstore::lib

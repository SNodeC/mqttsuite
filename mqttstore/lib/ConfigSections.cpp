/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#include "ConfigSections.h"

namespace mqtt::mqttstore::lib {

    ConfigDatabase::ConfigDatabase(SubCommand* parent)
        : SubCommand(parent, this, "Database connection")
        , hostOpt(setConfigurable(
              addOption("--host", "Hostname or IP address of server", "hostname|IPv4", CLI::TypeValidator<std::string>()), true))
        , usernameOpt(setConfigurable(
              addOption("--username", "Username for database connection", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt(setConfigurable(
              addOption("--password", "Password for database connection", "string", CLI::TypeValidator<std::string>()), true))
        , databaseOpt(setConfigurable(
              addOption("--database", "Database name for database connection", "string", CLI::TypeValidator<std::string>()), true))
        , portOpt(setConfigurable(
              addOption("--port", "Port number for database connection", "uint16_t", 3306, CLI::TypeValidator<uint16_t>()), true))
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

    ConfigSession::ConfigSession(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "MQTT session")
        , clientIdOpt(setConfigurable(addOption("--client-id", "MQTT Client-ID", "string", CLI::TypeValidator<std::string>()), true))
        , qoSOpt(setConfigurable(addOption("--qos", "Default subscription quality of service", "uint8_t", "0", CLI::Range(0, 2)), true))
        , retainSessionOpt(
              setConfigurable(
                  addFlag("--retain-session{true},-r{true}", "Reuse broker session", "bool", "false", CLI::IsMember({"true", "false"})),
                  true)
                  ->needs(clientIdOpt))
        , keepAliveOpt(setConfigurable(
              addOption("--keep-alive", "MQTT keep-alive interval", "uint16_t", "60", CLI::TypeValidator<uint16_t>()), true))
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

    const ConfigSession& ConfigSession::setWillQoS(uint8_t willQoS) const {
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

    const ConfigSession& ConfigSession::setUsername(const std::string& username) const {
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

    ConfigSubscribe::ConfigSubscribe(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "MQTT subscriptions")
        , topicOpt(
              setConfigurable(
                  addOption("--topic", "List of topics to subscribe to and persist", "string", CLI::TypeValidator<std::string>()), true)
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

    ConfigStorage::ConfigStorage(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Raw MQTT storage")
        , tableOpt(setConfigurable(
              addOption("--table", "MariaDB table for raw MQTT messages", "identifier", "mqtt_messages", CLI::TypeValidator<std::string>()),
              true))
        , autoCreateOpt(setConfigurable(addFlag("--auto-create{true}",
                                                "Create the raw message table if it does not exist",
                                                "bool",
                                                "true",
                                                CLI::IsMember({"true", "false"})),
                                        true))
        , storePayloadTextOpt(setConfigurable(
              addFlag(
                  "--store-payload-text{true}", "Persist payload_text for every message", "bool", "true", CLI::IsMember({"true", "false"})),
              true))
        , storePayloadJsonOpt(setConfigurable(addFlag("--store-payload-json{true}",
                                                      "Persist payload_json when payload parses as JSON",
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

    bool ConfigStorage::getAutoCreate() const {
        return autoCreateOpt->as<bool>();
    }

    const ConfigStorage& ConfigStorage::setAutoCreate(bool autoCreate) {
        autoCreateOpt->default_val(autoCreate)->clear();
        return *this;
    }

    bool ConfigStorage::getStorePayloadText() const {
        return storePayloadTextOpt->as<bool>();
    }

    const ConfigStorage& ConfigStorage::setStorePayloadText(bool storePayloadText) {
        storePayloadTextOpt->default_val(storePayloadText)->clear();
        return *this;
    }

    bool ConfigStorage::getStorePayloadJson() const {
        return storePayloadJsonOpt->as<bool>();
    }

    const ConfigStorage& ConfigStorage::setStorePayloadJson(bool storePayloadJson) {
        storePayloadJsonOpt->default_val(storePayloadJson)->clear();
        return *this;
    }

} // namespace mqtt::mqttstore::lib

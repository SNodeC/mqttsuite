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

#ifndef MQTTSTORE_LIB_MARIADBSTORAGE_H
#define MQTTSTORE_LIB_MARIADBSTORAGE_H

#include "MqttMessage.h"
#include "StoragePlan.h"

#include <database/mariadb/MariaDBClient.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

    class MariaDbStorage {
    public:
        struct ConnectionConfig {
            std::string database;
            std::string username;
            std::string password;
            std::string host;
            std::uint16_t port = 3306;
            std::string socket;
            std::uint32_t flags = 0;
        };

        MariaDbStorage(const std::string& connectionName,
                       const ConnectionConfig& connectionConfig,
                       std::string rawTable,
                       bool autoCreateRawTable,
                       StoragePlan storagePlan);

        void store(const MqttMessage& message);

    private:
        [[nodiscard]] static bool isSafeIdentifier(const std::string& identifier);
        [[nodiscard]] static std::string quoteIdentifier(const std::string& identifier);
        [[nodiscard]] static std::string sqlQuote(const std::string& value);
        [[nodiscard]] static std::string sqlValue(const nlohmann::json& value);
        [[nodiscard]] static std::optional<nlohmann::json> parsePayload(const std::string& payload);
        [[nodiscard]] static bool hasBinaryContent(const std::string& payload);
        [[nodiscard]] static std::string
        buildRawInsertSql(const std::string& rawTable, const MqttMessage& message, const std::optional<nlohmann::json>& payloadJson);
        [[nodiscard]] static std::string
        buildProjectionInsertSql(const StoragePlan::Projection& projection, const MqttMessage& message, const nlohmann::json& payloadJson);
        [[nodiscard]] static std::string
        jsonValueForColumn(const StoragePlan::ColumnMapping& mapping, const MqttMessage& message, const nlohmann::json& payloadJson);
        static void execLogFailure(const std::string& connectionName,
                                   const std::string& operation,
                                   const std::string& errorString,
                                   unsigned int errorNumber);
        void createRawTable();
        void storeProjections(const MqttMessage& message, const std::optional<nlohmann::json>& payloadJson);

        std::string connectionName;
        database::mariadb::MariaDBClient mariaDB;
        std::string rawTable;
        StoragePlan storagePlan;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MARIADBSTORAGE_H

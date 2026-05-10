/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#ifndef MQTTSTORE_LIB_MARIADBRAWSTORE_H
#define MQTTSTORE_LIB_MARIADBRAWSTORE_H

#include "RawMessage.h"

#include <database/mariadb/MariaDBClient.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

    class MariaDbRawStore {
    public:
        struct DatabaseConfig {
            std::string connectionName;
            std::string database;
            std::string username;
            std::string password;
            std::string host;
            uint16_t port = 3306;
            std::string socket;
            uint32_t flags = 0;
        };

        struct StorageConfig {
            std::string table = "mqtt_messages";
            bool autoCreate = true;
            bool storePayloadText = true;
            bool storePayloadJson = true;
        };

        MariaDbRawStore(const DatabaseConfig& databaseConfig, const StorageConfig& storageConfig);

        void ensureSchema();
        void store(const RawMessage& message);

    private:
        [[nodiscard]] std::string createTableSql() const;
        [[nodiscard]] std::string insertSql(const RawMessage& message) const;

        database::mariadb::MariaDBClient mariaDB;
        StorageConfig storageConfig;
        std::string quotedTable;
        bool schemaCreateRequested = false;
    };

    [[nodiscard]] std::string sqlQuote(const std::string& value);
    [[nodiscard]] std::string sqlIdentifier(const std::string& identifier);

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MARIADBRAWSTORE_H

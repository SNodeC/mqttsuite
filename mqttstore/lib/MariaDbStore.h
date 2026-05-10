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

#ifndef MQTTSTORE_LIB_MARIADBSTORE_H
#define MQTTSTORE_LIB_MARIADBSTORE_H

#include "MqttMessage.h"

#include <database/mariadb/MariaDBClient.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

    class MariaDbStore {
    public:
        MariaDbStore(const std::string& connectionName,
                     const std::string& database,
                     const std::string& username,
                     const std::string& password,
                     const std::string& host,
                     uint16_t port,
                     const std::string& socket,
                     uint32_t flags,
                     const std::string& table,
                     bool createSchema,
                     bool projectJson);

        void store(const MqttMessage& message);

    private:
        [[nodiscard]] std::string createSchemaSql() const;
        [[nodiscard]] std::string createFieldsSchemaSql() const;
        [[nodiscard]] std::string insertSql(const MqttMessage& message) const;
        void storeJsonProjections(const MqttMessage& message);

        database::mariadb::MariaDBClient mariaDB;
        std::string connectionName;
        std::string table;
        bool projectJson = true;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MARIADBSTORE_H

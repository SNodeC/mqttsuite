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

#ifndef MQTTSTORE_LIB_STORAGE_H
#define MQTTSTORE_LIB_STORAGE_H

#include <database/mariadb/MariaDBClient.h>
#include <nlohmann/json_fwd.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#endif

namespace iot::mqtt::packets {
    class Publish;
}

namespace mqtt::mqttstore::lib {

    struct MqttMessageEnvelope {
        std::string storageId;
        std::string connectionName;
        std::string topic;
        std::string payload;
        std::string payloadHex;
        std::string payloadFormat;
        std::optional<std::string> payloadText;
        std::optional<std::string> payloadJson;
        std::uint8_t qoS = 0;
        bool retain = false;
        bool dup = false;
        std::uint16_t packetIdentifier = 0;
        bool truncated = false;
    };

    class MariaDbStorage {
    public:
        MariaDbStorage(const std::string& connectionName,
                       const std::string& database,
                       const std::string& usernameDb,
                       const std::string& passwordDb,
                       const std::string& host,
                       uint16_t port,
                       const std::string& socket,
                       uint32_t flags,
                       const std::string& rawTable,
                       const std::string& fieldTable,
                       bool autoCreate,
                       bool flattenJson,
                       std::size_t maxPayloadBytes);

        void ensureSchema();
        void store(const MqttMessageEnvelope& envelope) const;

    private:
        friend MqttMessageEnvelope
        makeEnvelope(const std::string& connectionName, const iot::mqtt::packets::Publish& publish, std::size_t maxPayloadBytes);
        struct JsonScalarField {
            std::string path;
            std::string type;
            std::string value;
        };

        [[nodiscard]] static std::string sqlIdentifier(const std::string& identifier);
        [[nodiscard]] static std::string sqlQuote(const std::string& value);
        [[nodiscard]] static std::string sqlNullableString(const std::optional<std::string>& value);
        [[nodiscard]] static std::string sqlHexLiteral(const std::string& hexValue);
        [[nodiscard]] static bool isValidUtf8(const std::string& value);
        [[nodiscard]] static std::string toHex(const std::string& value);
        [[nodiscard]] static std::vector<JsonScalarField> flattenJsonScalars(const nlohmann::json& json);
        static void flattenJsonScalars(const nlohmann::json& json, const std::string& path, std::vector<JsonScalarField>& fields);
        void storeFlattenedFields(const std::string& storageId, const std::vector<JsonScalarField>& fields) const;

        database::mariadb::MariaDBClient mariaDB;
        std::string connectionName;
        std::string rawTable;
        std::string fieldTable;
        bool autoCreate;
        bool flattenJson;
        std::size_t maxPayloadBytes;
    };

    [[nodiscard]] MqttMessageEnvelope
    makeEnvelope(const std::string& connectionName, const iot::mqtt::packets::Publish& publish, std::size_t maxPayloadBytes);

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_STORAGE_H

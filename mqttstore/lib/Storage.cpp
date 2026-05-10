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

#include "Storage.h"

#include <iot/mqtt/packets/Publish.h>
#include <nlohmann/json.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <chrono>
#include <log/Logger.h>
#include <sstream>
#include <stdexcept>

#endif

namespace mqtt::mqttstore::lib {

    namespace {

        [[nodiscard]] std::string boolSql(bool value) {
            return value ? "TRUE" : "FALSE";
        }

        [[nodiscard]] std::string scalarToString(const nlohmann::json& json) {
            if (json.is_string()) {
                return json.get<std::string>();
            }

            if (json.is_boolean()) {
                return json.get<bool>() ? "true" : "false";
            }

            return json.dump();
        }

        [[nodiscard]] std::string scalarType(const nlohmann::json& json) {
            if (json.is_string()) {
                return "string";
            }
            if (json.is_number()) {
                return "number";
            }
            if (json.is_boolean()) {
                return "boolean";
            }
            if (json.is_null()) {
                return "null";
            }

            return "unknown";
        }

    } // namespace

    MariaDbStorage::MariaDbStorage(const std::string& connectionName,
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
                                   std::size_t maxPayloadBytes)
        : mariaDB(
              {
                  .connectionName = connectionName,
                  .hostname = host,
                  .username = usernameDb,
                  .password = passwordDb,
                  .database = database,
                  .port = port,
                  .socket = socket,
                  .flags = flags,
              },
              [connectionName](const database::mariadb::MariaDBState& state) {
                  if (state.connected) {
                      VLOG(0) << connectionName << " MariaDB: Connected";
                  } else if (state.error != 0) {
                      VLOG(0) << connectionName << " MariaDB: " << state.errorMessage << " [" << state.error << "]";
                  } else {
                      VLOG(0) << connectionName << " MariaDB: Lost connection";
                  }
              })
        , connectionName(connectionName)
        , rawTable(rawTable)
        , fieldTable(fieldTable)
        , autoCreate(autoCreate)
        , flattenJson(flattenJson)
        , maxPayloadBytes(maxPayloadBytes) {
    }

    void MariaDbStorage::ensureSchema() {
        if (!autoCreate) {
            return;
        }

        const std::string rawTableName = sqlIdentifier(rawTable);
        const std::string fieldTableName = sqlIdentifier(fieldTable);

        const std::string createRawTableSql = "CREATE TABLE IF NOT EXISTS " + rawTableName +
                                              " ("
                                              "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
                                              "`received_at` TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
                                              "`storage_id` VARCHAR(255) NOT NULL UNIQUE,"
                                              "`connection_name` VARCHAR(255) NOT NULL,"
                                              "`topic` VARCHAR(1024) NOT NULL,"
                                              "`qos` TINYINT UNSIGNED NOT NULL,"
                                              "`retain_flag` BOOLEAN NOT NULL,"
                                              "`dup_flag` BOOLEAN NOT NULL,"
                                              "`packet_identifier` INT UNSIGNED NOT NULL,"
                                              "`payload_format` ENUM('json','text','binary') NOT NULL,"
                                              "`payload_blob` LONGBLOB NOT NULL,"
                                              "`payload_text` LONGTEXT NULL,"
                                              "`payload_json` JSON NULL,"
                                              "`truncated` BOOLEAN NOT NULL DEFAULT FALSE,"
                                              "KEY `idx_received_at` (`received_at`),"
                                              "KEY `idx_topic` (`topic`(255))"
                                              ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

        const std::string createFieldTableSql = "CREATE TABLE IF NOT EXISTS " + fieldTableName +
                                                " ("
                                                "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
                                                "`storage_id` VARCHAR(255) NOT NULL,"
                                                "`field_path` VARCHAR(1024) NOT NULL,"
                                                "`value_type` ENUM('string','number','boolean','null','unknown') NOT NULL,"
                                                "`value_text` LONGTEXT NULL,"
                                                "KEY `idx_storage_id` (`storage_id`),"
                                                "KEY `idx_field_path` (`field_path`(255))"
                                                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

        mariaDB.exec(
            createRawTableSql,
            [connectionName = this->connectionName]() {
                VLOG(0) << connectionName << " MariaDB: raw message table is ready";
            },
            [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) {
                VLOG(0) << connectionName << " MariaDB: raw message table creation failed: " << errorString << " : " << errorNumber;
            });

        if (flattenJson) {
            mariaDB.exec(
                createFieldTableSql,
                [connectionName = this->connectionName]() {
                    VLOG(0) << connectionName << " MariaDB: JSON field table is ready";
                },
                [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(0) << connectionName << " MariaDB: JSON field table creation failed: " << errorString << " : " << errorNumber;
                });
        }
    }

    void MariaDbStorage::store(const MqttMessageEnvelope& envelope) const {
        const std::string sql =
            "INSERT INTO " + sqlIdentifier(rawTable) +
            "(`storage_id`, `connection_name`, `topic`, `qos`, `retain_flag`, `dup_flag`, `packet_identifier`, `payload_format`, "
            "`payload_blob`, `payload_text`, `payload_json`, `truncated`) VALUES (" +
            sqlQuote(envelope.storageId) + ", " + sqlQuote(envelope.connectionName) + ", " + sqlQuote(envelope.topic) + ", " +
            std::to_string(static_cast<unsigned int>(envelope.qoS)) + ", " + boolSql(envelope.retain) + ", " + boolSql(envelope.dup) +
            ", " + std::to_string(envelope.packetIdentifier) + ", " + sqlQuote(envelope.payloadFormat) + ", " +
            sqlHexLiteral(envelope.payloadHex) + ", " + sqlNullableString(envelope.payloadText) + ", " +
            sqlNullableString(envelope.payloadJson) + ", " + boolSql(envelope.truncated) + ")";

        std::vector<JsonScalarField> fields;
        if (flattenJson && envelope.payloadJson.has_value()) {
            try {
                fields = flattenJsonScalars(nlohmann::json::parse(*envelope.payloadJson));
            } catch (const nlohmann::json::parse_error&) {
                fields.clear();
            }
        }

        mariaDB.exec(
            sql,
            [this, envelope, fields = std::move(fields)]() {
                VLOG(0) << connectionName << " MariaDB: stored MQTT message";
                storeFlattenedFields(envelope.storageId, fields);
            },
            [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) {
                VLOG(0) << connectionName << " MariaDB: MQTT message insert failed: " << errorString << " : " << errorNumber;
            });
    }

    std::string MariaDbStorage::sqlIdentifier(const std::string& identifier) {
        std::string quoted;
        quoted.reserve(identifier.size() + 2);
        quoted.push_back('`');

        for (const char character : identifier) {
            if (character == '`') {
                quoted += "``";
            } else {
                quoted.push_back(character);
            }
        }

        quoted.push_back('`');
        return quoted;
    }

    std::string MariaDbStorage::sqlQuote(const std::string& value) {
        std::string quoted;
        quoted.reserve(value.size() + 2);
        quoted.push_back('\'');

        for (const char character : value) {
            switch (character) {
                case '\0':
                    quoted += "\\0";
                    break;
                case '\n':
                    quoted += "\\n";
                    break;
                case '\r':
                    quoted += "\\r";
                    break;
                case '\\':
                    quoted += "\\\\";
                    break;
                case '\'':
                    quoted += "\\'";
                    break;
                case '"':
                    quoted += "\\\"";
                    break;
                case '\x1a':
                    quoted += "\\Z";
                    break;
                default:
                    quoted.push_back(character);
                    break;
            }
        }

        quoted.push_back('\'');
        return quoted;
    }

    std::string MariaDbStorage::sqlNullableString(const std::optional<std::string>& value) {
        if (!value.has_value()) {
            return "NULL";
        }

        return sqlQuote(*value);
    }

    std::string MariaDbStorage::sqlHexLiteral(const std::string& hexValue) {
        return "X'" + hexValue + "'";
    }

    bool MariaDbStorage::isValidUtf8(const std::string& value) {
        std::size_t remainingBytes = 0;

        for (const unsigned char character : value) {
            if (remainingBytes == 0) {
                if ((character & 0b10000000) == 0) {
                    continue;
                }
                if ((character & 0b11100000) == 0b11000000) {
                    remainingBytes = 1;
                } else if ((character & 0b11110000) == 0b11100000) {
                    remainingBytes = 2;
                } else if ((character & 0b11111000) == 0b11110000) {
                    remainingBytes = 3;
                } else {
                    return false;
                }
            } else if ((character & 0b11000000) == 0b10000000) {
                --remainingBytes;
            } else {
                return false;
            }
        }

        return remainingBytes == 0;
    }

    std::string MariaDbStorage::toHex(const std::string& value) {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string encoded;
        encoded.reserve(value.size() * 2);

        for (const unsigned char character : value) {
            encoded.push_back(hex[character >> 4]);
            encoded.push_back(hex[character & 0x0F]);
        }

        return encoded;
    }

    std::vector<MariaDbStorage::JsonScalarField> MariaDbStorage::flattenJsonScalars(const nlohmann::json& json) {
        std::vector<JsonScalarField> fields;
        flattenJsonScalars(json, "", fields);
        return fields;
    }

    void MariaDbStorage::flattenJsonScalars(const nlohmann::json& json, const std::string& path, std::vector<JsonScalarField>& fields) {
        if (json.is_object()) {
            for (const auto& [key, value] : json.items()) {
                flattenJsonScalars(value, path.empty() ? key : path + "." + key, fields);
            }
        } else if (json.is_array()) {
            for (std::size_t index = 0; index < json.size(); ++index) {
                flattenJsonScalars(json.at(index), path + "[" + std::to_string(index) + "]", fields);
            }
        } else {
            fields.push_back({.path = path, .type = scalarType(json), .value = scalarToString(json)});
        }
    }

    void MariaDbStorage::storeFlattenedFields(const std::string& storageId, const std::vector<JsonScalarField>& fields) const {
        if (fields.empty()) {
            return;
        }

        std::ostringstream sql;
        sql << "INSERT INTO " << sqlIdentifier(fieldTable) << "(`storage_id`, `field_path`, `value_type`, `value_text`) VALUES ";

        bool first = true;
        for (const JsonScalarField& field : fields) {
            sql << (first ? "" : ", ") << "(" << sqlQuote(storageId) << ", " << sqlQuote(field.path) << ", " << sqlQuote(field.type) << ", "
                << sqlQuote(field.value) << ")";
            first = false;
        }

        mariaDB.exec(
            sql.str(),
            [connectionName = this->connectionName, count = fields.size()]() {
                VLOG(0) << connectionName << " MariaDB: stored " << count << " JSON field projection(s)";
            },
            [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) {
                VLOG(0) << connectionName << " MariaDB: JSON field projection insert failed: " << errorString << " : " << errorNumber;
            });
    }

    MqttMessageEnvelope
    makeEnvelope(const std::string& connectionName, const iot::mqtt::packets::Publish& publish, std::size_t maxPayloadBytes) {
        static const auto storageIdPrefix = std::to_string(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        static std::uint64_t nextStorageSequence = 0;

        MqttMessageEnvelope envelope;
        envelope.storageId = connectionName + "-" + storageIdPrefix + "-" + std::to_string(nextStorageSequence++);
        envelope.connectionName = connectionName;
        envelope.topic = publish.getTopic();
        envelope.payload = publish.getMessage();
        envelope.qoS = publish.getQoS();
        envelope.retain = publish.getRetain() != 0;
        envelope.dup = publish.getDup() != 0;
        envelope.packetIdentifier = publish.getPacketIdentifier();

        if (maxPayloadBytes > 0 && envelope.payload.size() > maxPayloadBytes) {
            envelope.payload.resize(maxPayloadBytes);
            envelope.truncated = true;
        }

        envelope.payloadHex = MariaDbStorage::toHex(envelope.payload);

        if (MariaDbStorage::isValidUtf8(envelope.payload)) {
            envelope.payloadText = envelope.payload;
            try {
                envelope.payloadJson = nlohmann::json::parse(envelope.payload).dump();
                envelope.payloadFormat = "json";
            } catch (const nlohmann::json::parse_error&) {
                envelope.payloadFormat = "text";
            }
        } else {
            envelope.payloadFormat = "binary";
        }

        return envelope;
    }

} // namespace mqtt::mqttstore::lib

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

#include "MariaDbStore.h"

#include <nlohmann/json.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <array>
#include <cstdint>
#include <iomanip>
#include <log/Logger.h>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#endif

namespace mqtt::mqttstore::lib {

    namespace {

        [[nodiscard]] std::string sqlIdentifier(const std::string& identifier) {
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

        [[nodiscard]] std::string sqlQuote(const std::string& value) {
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

        [[nodiscard]] std::string toBase64(std::string_view value) {
            constexpr std::array<char, 64> alphabet = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                                       'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                                       'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                                       'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

            std::string encoded;
            encoded.reserve(((value.size() + 2) / 3) * 4);

            for (std::size_t offset = 0; offset < value.size(); offset += 3) {
                const std::size_t remaining = value.size() - offset;
                const auto first = static_cast<std::uint32_t>(static_cast<unsigned char>(value[offset]));
                const auto second = remaining > 1 ? static_cast<std::uint32_t>(static_cast<unsigned char>(value[offset + 1])) : 0U;
                const auto third = remaining > 2 ? static_cast<std::uint32_t>(static_cast<unsigned char>(value[offset + 2])) : 0U;

                encoded.push_back(alphabet[static_cast<std::size_t>(first >> 2U)]);
                encoded.push_back(alphabet[static_cast<std::size_t>(((first & 0x03U) << 4U) | (second >> 4U))]);
                encoded.push_back(remaining > 1 ? alphabet[static_cast<std::size_t>(((second & 0x0FU) << 2U) | (third >> 6U))] : '=');
                encoded.push_back(remaining > 2 ? alphabet[static_cast<std::size_t>(third & 0x3FU)] : '=');
            }

            return encoded;
        }

        struct JsonProjection {
            std::string path;
            std::string type;
            std::string value;
            std::string numericValue = "NULL";
            std::string booleanValue = "NULL";
        };

        void flattenJson(const nlohmann::json& json, const std::string& path, std::vector<JsonProjection>& projections) {
            if (json.is_object()) {
                for (auto iterator = json.begin(); iterator != json.end(); ++iterator) {
                    flattenJson(*iterator, path.empty() ? iterator.key() : path + "." + iterator.key(), projections);
                }
            } else if (json.is_array()) {
                for (std::size_t index = 0; index < json.size(); ++index) {
                    flattenJson(json.at(index), path + "[" + std::to_string(index) + "]", projections);
                }
            } else if (json.is_string()) {
                projections.push_back({.path = path, .type = "string", .value = json.get<std::string>()});
            } else if (json.is_number()) {
                projections.push_back({.path = path, .type = "number", .value = json.dump(), .numericValue = json.dump()});
            } else if (json.is_boolean()) {
                projections.push_back({.path = path,
                                       .type = "boolean",
                                       .value = json.get<bool>() ? "true" : "false",
                                       .booleanValue = json.get<bool>() ? "TRUE" : "FALSE"});
            } else if (json.is_null()) {
                projections.push_back({.path = path, .type = "null", .value = ""});
            }
        }

        [[nodiscard]] std::string nullableJsonPayload(const std::string& payload, std::string& payloadFormat) {
            try {
                const nlohmann::json json = nlohmann::json::parse(payload);
                payloadFormat = "json";

                return sqlQuote(json.dump());
            } catch (const nlohmann::json::parse_error&) {
                payloadFormat = "raw";

                return "NULL";
            }
        }

    } // namespace

    MariaDbStore::MariaDbStore(const std::string& connectionName,
                               const std::string& database,
                               const std::string& username,
                               const std::string& password,
                               const std::string& host,
                               uint16_t port,
                               const std::string& socket,
                               uint32_t flags,
                               const std::string& table,
                               bool createSchema,
                               bool projectJson)
        : mariaDB(
              {
                  .connectionName = connectionName,
                  .hostname = host,
                  .username = username,
                  .password = password,
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
        , table(table)
        , projectJson(projectJson) {
        if (createSchema) {
            mariaDB.exec(
                createSchemaSql(),
                [connectionName]() -> void {
                    VLOG(0) << connectionName << " MariaDB: raw MQTT schema is ready";
                },
                [connectionName](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << connectionName << " MariaDB schema creation failed: " << errorString << " : " << errorNumber;
                });

            if (projectJson) {
                mariaDB.exec(
                    createFieldsSchemaSql(),
                    [connectionName]() -> void {
                        VLOG(0) << connectionName << " MariaDB: JSON projection schema is ready";
                    },
                    [connectionName](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << connectionName << " MariaDB projection schema creation failed: " << errorString << " : " << errorNumber;
                    });
            }
        }
    }

    std::string MariaDbStore::createSchemaSql() const {
        const std::string tableName = sqlIdentifier(table);

        return "CREATE TABLE IF NOT EXISTS " + tableName +
               " ("
               "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
               "`message_uuid` VARCHAR(255) NOT NULL UNIQUE,"
               "`received_at` TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
               "`connection_name` VARCHAR(255) NOT NULL,"
               "`topic` VARCHAR(1024) NOT NULL,"
               "`qos` TINYINT UNSIGNED NOT NULL,"
               "`retain_flag` BOOLEAN NOT NULL,"
               "`dup_flag` BOOLEAN NOT NULL,"
               "`packet_identifier` INT UNSIGNED NOT NULL,"
               "`payload_format` ENUM('json','raw') NOT NULL,"
               "`payload_text` LONGTEXT NOT NULL,"
               "`payload_base64` LONGTEXT NOT NULL,"
               "`payload_json` JSON NULL,"
               "INDEX `idx_received_at` (`received_at`),"
               "INDEX `idx_topic` (`topic`(255))"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin";
    }

    std::string MariaDbStore::createFieldsSchemaSql() const {
        const std::string tableName = sqlIdentifier(table + "_fields");

        return "CREATE TABLE IF NOT EXISTS " + tableName +
               " ("
               "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
               "`message_uuid` VARCHAR(255) NOT NULL,"
               "`field_path` VARCHAR(1024) NOT NULL,"
               "`json_type` VARCHAR(32) NOT NULL,"
               "`string_value` LONGTEXT NOT NULL,"
               "`numeric_value` DOUBLE NULL,"
               "`boolean_value` BOOLEAN NULL,"
               "INDEX `idx_message_uuid` (`message_uuid`),"
               "INDEX `idx_field_path` (`field_path`(255)),"
               "INDEX `idx_field_numeric` (`field_path`(255), `numeric_value`)"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin";
    }

    std::string MariaDbStore::insertSql(const MqttMessage& message) const {
        std::string payloadFormat;
        const std::string payloadJson = nullableJsonPayload(message.payload, payloadFormat);

        return "INSERT INTO " + sqlIdentifier(table) +
               "(`message_uuid`, `connection_name`, `topic`, `qos`, `retain_flag`, `dup_flag`, `packet_identifier`, `payload_format`, "
               "`payload_text`, "
               "`payload_base64`, `payload_json`) VALUES (" +
               sqlQuote(message.messageId) + ", " + sqlQuote(message.connectionName) + ", " + sqlQuote(message.topic) + ", " +
               std::to_string(static_cast<unsigned int>(message.qoS)) + ", " + (message.retain ? "TRUE" : "FALSE") + ", " +
               (message.dup ? "TRUE" : "FALSE") + ", " + std::to_string(message.packetIdentifier) + ", " + sqlQuote(payloadFormat) + ", " +
               sqlQuote(message.payload) + ", " + sqlQuote(toBase64(message.payload)) + ", " + payloadJson + ")";
    }

    void MariaDbStore::storeJsonProjections(const MqttMessage& message) {
        if (!projectJson) {
            return;
        }

        try {
            const nlohmann::json json = nlohmann::json::parse(message.payload);
            std::vector<JsonProjection> projections;
            flattenJson(json, "", projections);

            for (const JsonProjection& projection : projections) {
                const std::string sql =
                    "INSERT INTO " + sqlIdentifier(table + "_fields") +
                    "(`message_uuid`, `field_path`, `json_type`, `string_value`, `numeric_value`, `boolean_value`) VALUES (" +
                    sqlQuote(message.messageId) + ", " + sqlQuote(projection.path) + ", " + sqlQuote(projection.type) + ", " +
                    sqlQuote(projection.value) + ", " + projection.numericValue + ", " + projection.booleanValue + ")";

                mariaDB.exec(
                    sql,
                    [connectionName = this->connectionName, path = projection.path]() -> void {
                        VLOG(2) << connectionName << " MariaDB: stored JSON projection " << path;
                    },
                    [connectionName = this->connectionName, path = projection.path](const std::string& errorString,
                                                                                    unsigned int errorNumber) -> void {
                        VLOG(0) << connectionName << " MariaDB JSON projection insert failed for " << path << ": " << errorString << " : "
                                << errorNumber;
                    });
            }
        } catch (const nlohmann::json::parse_error&) {
        }
    }

    void MariaDbStore::store(const MqttMessage& message) {
        mariaDB.exec(
            insertSql(message),
            [this, message]() -> void {
                VLOG(1) << connectionName << " MariaDB: stored MQTT message from topic " << message.topic;
                storeJsonProjections(message);
            },
            [connectionName = this->connectionName, topic = message.topic](const std::string& errorString,
                                                                           unsigned int errorNumber) -> void {
                VLOG(0) << connectionName << " MariaDB MQTT insert failed for topic " << topic << ": " << errorString << " : "
                        << errorNumber;
            });
    }

} // namespace mqtt::mqttstore::lib

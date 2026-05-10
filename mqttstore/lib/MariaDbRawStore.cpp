/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#include "MariaDbRawStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype>
#include <log/Logger.h>
#include <sstream>
#include <stdexcept>

#endif

namespace mqtt::mqttstore::lib {

    namespace {

        [[nodiscard]] bool containsNullByte(const std::string& value) {
            return value.find('\0') != std::string::npos;
        }

        [[nodiscard]] std::string sqlNullableString(const std::string& value, bool enabled) {
            return enabled && !containsNullByte(value) ? sqlQuote(value) : "NULL";
        }

        [[nodiscard]] std::string sqlNullableJson(const RawMessage& message, bool enabled) {
            if (!enabled || !message.payloadIsJson) {
                return "NULL";
            }

            return sqlQuote(message.payload);
        }

        [[nodiscard]] std::string sqlHex(const std::string& value) {
            constexpr char hexDigits[] = "0123456789ABCDEF";
            std::string hex;
            hex.reserve(value.size() * 2);

            for (const unsigned char character : value) {
                hex.push_back(hexDigits[character >> 4]);
                hex.push_back(hexDigits[character & 0x0F]);
            }

            return hex;
        }

        [[nodiscard]] std::string sqlPayloadFormat(const RawMessage& message) {
            if (message.payloadIsJson) {
                return "json";
            }

            return containsNullByte(message.payload) ? "binary" : "text";
        }

        [[nodiscard]] std::string sqlBool(bool value) {
            return value ? "1" : "0";
        }

        [[nodiscard]] std::string sqlNullablePacketIdentifier(uint16_t packetIdentifier) {
            if (packetIdentifier == 0) {
                return "NULL";
            }

            return std::to_string(packetIdentifier);
        }

        [[nodiscard]] bool isIdentifierChar(char character) {
            const unsigned char unsignedCharacter = static_cast<unsigned char>(character);
            return std::isalnum(unsignedCharacter) != 0 || character == '_';
        }

    } // namespace

    MariaDbRawStore::MariaDbRawStore(const DatabaseConfig& databaseConfig, const StorageConfig& storageConfig)
        : mariaDB(
              {
                  .connectionName = databaseConfig.connectionName,
                  .hostname = databaseConfig.host,
                  .username = databaseConfig.username,
                  .password = databaseConfig.password,
                  .database = databaseConfig.database,
                  .port = databaseConfig.port,
                  .socket = databaseConfig.socket,
                  .flags = databaseConfig.flags,
              },
              [connectionName = databaseConfig.connectionName](const database::mariadb::MariaDBState& state) {
                  if (state.connected) {
                      VLOG(0) << connectionName << " MariaDB: connected";
                  } else if (state.error != 0) {
                      VLOG(0) << connectionName << " MariaDB: " << state.errorMessage << " [" << state.error << "]";
                  } else {
                      VLOG(0) << connectionName << " MariaDB: lost connection";
                  }
              })
        , storageConfig(storageConfig)
        , quotedTable(sqlIdentifier(storageConfig.table)) {
    }

    void MariaDbRawStore::ensureSchema() {
        if (!storageConfig.autoCreate || schemaCreateRequested) {
            return;
        }

        schemaCreateRequested = true;

        mariaDB.exec(
            createTableSql(),
            [table = storageConfig.table]() -> void {
                VLOG(0) << "MariaDB: raw MQTT table ready: " << table;
            },
            [table = storageConfig.table](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "MariaDB: raw MQTT table create failed for " << table << ": " << errorString << " : " << errorNumber;
            });
    }

    void MariaDbRawStore::store(const RawMessage& message) {
        ensureSchema();

        mariaDB.exec(
            insertSql(message),
            [topic = message.topic]() -> void {
                VLOG(1) << "MariaDB: stored MQTT message for topic " << topic;
            },
            [topic = message.topic](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "MariaDB: MQTT message insert failed for topic " << topic << ": " << errorString << " : " << errorNumber;
            });
    }

    std::string MariaDbRawStore::createTableSql() const {
        return "CREATE TABLE IF NOT EXISTS " + quotedTable +
               " ("
               "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, "
               "`received_at` TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), "
               "`source_instance` VARCHAR(255) NULL, "
               "`client_id` VARCHAR(255) NULL, "
               "`topic` VARCHAR(1024) NOT NULL, "
               "`qos` TINYINT UNSIGNED NOT NULL, "
               "`retain_flag` BOOLEAN NOT NULL, "
               "`dup_flag` BOOLEAN NOT NULL, "
               "`packet_identifier` INT UNSIGNED NULL, "
               "`payload_format` ENUM('json','text','binary') NOT NULL, "
               "`payload_blob` LONGBLOB NOT NULL, "
               "`payload_text` LONGTEXT NULL, "
               "`payload_json` JSON NULL, "
               "PRIMARY KEY (`id`), "
               "KEY `idx_mqtt_messages_received_at` (`received_at`), "
               "KEY `idx_mqtt_messages_topic` (`topic`(255)), "
               "KEY `idx_mqtt_messages_source_instance` (`source_instance`)"
               ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    }

    std::string MariaDbRawStore::insertSql(const RawMessage& message) const {
        return "INSERT INTO " + quotedTable +
               " (`source_instance`, `client_id`, `topic`, `qos`, `retain_flag`, `dup_flag`, `packet_identifier`, "
               "`payload_format`, `payload_blob`, `payload_text`, `payload_json`) VALUES (" +
               sqlQuote(message.sourceInstance) + ", " + sqlQuote(message.clientId) + ", " + sqlQuote(message.topic) + ", " +
               std::to_string(message.qoS) + ", " + sqlBool(message.retain) + ", " + sqlBool(message.dup) + ", " +
               sqlNullablePacketIdentifier(message.packetIdentifier) + ", " + sqlQuote(sqlPayloadFormat(message)) + ", UNHEX(" +
               sqlQuote(sqlHex(message.payload)) + "), " + sqlNullableString(message.payload, storageConfig.storePayloadText) + ", " +
               sqlNullableJson(message, storageConfig.storePayloadJson) + ")";
    }

    std::string sqlQuote(const std::string& value) {
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

    std::string sqlIdentifier(const std::string& identifier) {
        if (identifier.empty()) {
            throw std::invalid_argument("SQL identifier must not be empty");
        }

        std::ostringstream quoted;
        std::string part;
        bool previousWasSeparator = true;

        const auto flushPart = [&quoted, &part, &previousWasSeparator]() {
            if (part.empty()) {
                throw std::invalid_argument("SQL identifier contains an empty part");
            }

            if (!previousWasSeparator) {
                quoted << '.';
            }

            quoted << '`' << part << '`';
            part.clear();
            previousWasSeparator = false;
        };

        for (const char character : identifier) {
            if (character == '.') {
                flushPart();
            } else if (isIdentifierChar(character)) {
                part.push_back(character);
            } else {
                throw std::invalid_argument("SQL identifier contains unsupported character: " + std::string(1, character));
            }
        }

        flushPart();

        return quoted.str();
    }

} // namespace mqtt::mqttstore::lib

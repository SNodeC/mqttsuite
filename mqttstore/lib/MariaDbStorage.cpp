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

#include "MariaDbStorage.h"

#include <nlohmann/json.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <log/Logger.h>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#endif

namespace mqtt::mqttstore::lib {

    namespace {

        [[nodiscard]] std::vector<std::string> splitTopic(const std::string& topic) {
            std::vector<std::string> result;
            std::stringstream topicStream(topic);

            for (std::string level; std::getline(topicStream, level, '/');) {
                result.push_back(level);
            }

            if (!topic.empty() && topic.back() == '/') {
                result.emplace_back();
            }

            return result;
        }

    } // namespace

    MariaDbStorage::MariaDbStorage(const std::string& connectionName,
                                   const ConnectionConfig& connectionConfig,
                                   std::string rawTable,
                                   bool autoCreateRawTable,
                                   StoragePlan storagePlan)
        : connectionName(connectionName)
        , mariaDB(
              {
                  .connectionName = connectionName,
                  .hostname = connectionConfig.host,
                  .username = connectionConfig.username,
                  .password = connectionConfig.password,
                  .database = connectionConfig.database,
                  .port = connectionConfig.port,
                  .socket = connectionConfig.socket,
                  .flags = connectionConfig.flags,
              },
              [connectionName = this->connectionName](const database::mariadb::MariaDBState& state) {
                  if (state.connected) {
                      VLOG(0) << connectionName << " MariaDB: connected";
                  } else if (state.error != 0) {
                      VLOG(0) << connectionName << " MariaDB: " << state.errorMessage << " [" << state.error << "]";
                  } else {
                      VLOG(0) << connectionName << " MariaDB: lost connection";
                  }
              })
        , rawTable(std::move(rawTable))
        , storagePlan(std::move(storagePlan)) {
        if (!isSafeIdentifier(this->rawTable)) {
            throw std::runtime_error("Unsafe raw table name: " + this->rawTable);
        }

        if (autoCreateRawTable) {
            createRawTable();
        }
    }

    void MariaDbStorage::store(const MqttMessage& message) {
        const std::optional<nlohmann::json> payloadJson = parsePayload(message.payload);
        const std::string rawInsertSql = buildRawInsertSql(rawTable, message, payloadJson);

        mariaDB.exec(
            rawInsertSql,
            [connectionName = this->connectionName, topic = message.topic]() -> void {
                VLOG(1) << connectionName << " MariaDB: stored raw MQTT message for topic '" << topic << "'";
            },
            [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) -> void {
                execLogFailure(connectionName, "raw MQTT message insert", errorString, errorNumber);
            });

        storeProjections(message, payloadJson);
    }

    bool MariaDbStorage::isSafeIdentifier(const std::string& identifier) {
        return !identifier.empty() && std::all_of(identifier.begin(), identifier.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '_';
        });
    }

    std::string MariaDbStorage::quoteIdentifier(const std::string& identifier) {
        if (!isSafeIdentifier(identifier)) {
            throw std::runtime_error("Unsafe SQL identifier: " + identifier);
        }

        return "`" + identifier + "`";
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

    std::string MariaDbStorage::sqlValue(const nlohmann::json& value) {
        if (value.is_null()) {
            return "NULL";
        }
        if (value.is_boolean()) {
            return value.get<bool>() ? "TRUE" : "FALSE";
        }
        if (value.is_number()) {
            return value.dump();
        }
        if (value.is_string()) {
            return sqlQuote(value.get<std::string>());
        }

        return sqlQuote(value.dump());
    }

    std::optional<nlohmann::json> MariaDbStorage::parsePayload(const std::string& payload) {
        try {
            return nlohmann::json::parse(payload);
        } catch (const nlohmann::json::parse_error&) {
            return std::nullopt;
        }
    }

    bool MariaDbStorage::hasBinaryContent(const std::string& payload) {
        return std::any_of(payload.begin(), payload.end(), [](unsigned char character) {
            return character == '\0' || (character < 0x09) || (character > 0x0D && character < 0x20);
        });
    }

    std::string MariaDbStorage::buildRawInsertSql(const std::string& rawTable,
                                                  const MqttMessage& message,
                                                  const std::optional<nlohmann::json>& payloadJson) {
        const bool binaryPayload = !payloadJson.has_value() && hasBinaryContent(message.payload);
        const std::string payloadFormat = payloadJson.has_value() ? "json" : (binaryPayload ? "binary" : "text");
        const std::string payloadTextValue = binaryPayload ? "NULL" : sqlQuote(message.payload);
        const std::string payloadJsonValue = payloadJson.has_value() ? sqlQuote(payloadJson->dump()) : "NULL";

        return "INSERT INTO " + quoteIdentifier(rawTable) +
               "(`received_at`, `source_instance`, `topic`, `qos`, `retain_flag`, `dup_flag`, `packet_identifier`, `payload`, "
               "`payload_text`, `payload_json`, `payload_format`) VALUES (CURRENT_TIMESTAMP(6), " +
               sqlQuote(message.connectionName) + ", " + sqlQuote(message.topic) + ", " +
               std::to_string(static_cast<unsigned int>(message.qoS)) + ", " + (message.retain ? "TRUE" : "FALSE") + ", " +
               (message.dup ? "TRUE" : "FALSE") + ", " + std::to_string(message.packetIdentifier) + ", " + sqlQuote(message.payload) +
               ", " + payloadTextValue + ", " + payloadJsonValue + ", " + sqlQuote(payloadFormat) + ")";
    }

    std::string MariaDbStorage::buildProjectionInsertSql(const StoragePlan::Projection& projection,
                                                         const MqttMessage& message,
                                                         const nlohmann::json& payloadJson) {
        std::string columnList;
        std::string valueList;

        for (const StoragePlan::ColumnMapping& mapping : projection.columns) {
            const std::string value = jsonValueForColumn(mapping, message, payloadJson);
            if (value.empty() && !mapping.required) {
                continue;
            }

            columnList += (columnList.empty() ? "" : ", ") + quoteIdentifier(mapping.column);
            valueList += (valueList.empty() ? "" : ", ") + (value.empty() ? "NULL" : value);
        }

        if (columnList.empty()) {
            throw std::runtime_error("Projection '" + projection.name + "' produced no columns");
        }

        return "INSERT INTO " + quoteIdentifier(projection.table) + "(" + columnList + ") VALUES (" + valueList + ")";
    }

    std::string MariaDbStorage::jsonValueForColumn(const StoragePlan::ColumnMapping& mapping,
                                                   const MqttMessage& message,
                                                   const nlohmann::json& payloadJson) {
        if (mapping.literal.has_value()) {
            return sqlQuote(*mapping.literal);
        }

        if (mapping.topicLevel.has_value()) {
            const std::vector<std::string> topicLevels = splitTopic(message.topic);
            if (*mapping.topicLevel >= topicLevels.size()) {
                return {};
            }

            return sqlQuote(topicLevels[*mapping.topicLevel]);
        }

        if (mapping.jsonPointer.empty()) {
            return {};
        }

        const nlohmann::json::json_pointer pointer(mapping.jsonPointer);
        if (!payloadJson.contains(pointer)) {
            return {};
        }

        return sqlValue(payloadJson.at(pointer));
    }

    void MariaDbStorage::execLogFailure(const std::string& connectionName,
                                        const std::string& operation,
                                        const std::string& errorString,
                                        unsigned int errorNumber) {
        VLOG(0) << connectionName << " MariaDB " << operation << " failed: " << errorString << " : " << errorNumber;
    }

    void MariaDbStorage::createRawTable() {
        const std::string sql = "CREATE TABLE IF NOT EXISTS " + quoteIdentifier(rawTable) +
                                "("
                                "`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
                                "`received_at` TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),"
                                "`source_instance` VARCHAR(255) NULL,"
                                "`topic` VARCHAR(1024) NOT NULL,"
                                "`qos` TINYINT UNSIGNED NOT NULL,"
                                "`retain_flag` BOOLEAN NOT NULL,"
                                "`dup_flag` BOOLEAN NOT NULL,"
                                "`packet_identifier` INT UNSIGNED NULL,"
                                "`payload` LONGBLOB NOT NULL,"
                                "`payload_text` LONGTEXT NULL,"
                                "`payload_json` JSON NULL,"
                                "`payload_format` ENUM('json', 'text', 'binary') NOT NULL,"
                                "INDEX `idx_received_at` (`received_at`),"
                                "INDEX `idx_topic` (`topic`(255))"
                                ")";

        mariaDB.exec(
            sql,
            [connectionName = this->connectionName, rawTable = this->rawTable]() -> void {
                VLOG(0) << connectionName << " MariaDB: ensured raw MQTT table '" << rawTable << "'";
            },
            [connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) -> void {
                execLogFailure(connectionName, "raw MQTT table creation", errorString, errorNumber);
            });
    }

    void MariaDbStorage::storeProjections(const MqttMessage& message, const std::optional<nlohmann::json>& payloadJson) {
        if (!payloadJson.has_value()) {
            return;
        }

        for (const StoragePlan::Projection* projection : storagePlan.match(message.topic)) {
            try {
                const std::string sql = buildProjectionInsertSql(*projection, message, *payloadJson);
                mariaDB.exec(
                    sql,
                    [connectionName = this->connectionName, projectionName = projection->name]() -> void {
                        VLOG(1) << connectionName << " MariaDB: projection insert completed for '" << projectionName << "'";
                    },
                    [connectionName = this->connectionName, projectionName = projection->name](const std::string& errorString,
                                                                                               unsigned int errorNumber) -> void {
                        execLogFailure(connectionName, "projection '" + projectionName + "' insert", errorString, errorNumber);
                    });
            } catch (const std::exception& error) {
                VLOG(0) << connectionName << " MariaDB projection '" << projection->name << "' skipped: " << error.what();
            }
        }
    }

} // namespace mqtt::mqttstore::lib

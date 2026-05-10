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

#ifndef MQTTSTORE_LIB_STORAGEPLAN_H
#define MQTTSTORE_LIB_STORAGEPLAN_H

#include <nlohmann/json_fwd.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <optional>
#include <string>
#include <vector>

#endif

namespace mqtt::mqttstore::lib {

    class StoragePlan {
    public:
        struct ColumnMapping {
            std::string column;
            std::string jsonPointer;
            std::optional<std::size_t> topicLevel;
            std::optional<std::string> literal;
            bool required = false;
        };

        struct Projection {
            std::string name;
            std::string topic;
            std::string table;
            std::vector<ColumnMapping> columns;
        };

        [[nodiscard]] static StoragePlan fromFile(const std::string& fileName);
        [[nodiscard]] static StoragePlan fromJson(const nlohmann::json& json);

        [[nodiscard]] const std::vector<Projection>& getProjections() const;
        [[nodiscard]] std::vector<const Projection*> match(const std::string& topic) const;

    private:
        static bool topicMatches(const std::string& filter, const std::string& topic);
        std::vector<Projection> projections;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_STORAGEPLAN_H

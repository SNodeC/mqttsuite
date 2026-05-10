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

#include "StoragePlan.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#endif

namespace mqtt::mqttstore::lib {

#include "projection-schema.json.h" // IWYU pragma: keep

    namespace {

        void validateProjectionConfiguration(const nlohmann::json& json) {
            try {
                const nlohmann::json projectionJsonSchema = nlohmann::json::parse(projectionJsonSchemaString);
                const nlohmann::json_schema::json_validator validator(
                    projectionJsonSchema, nullptr, nlohmann::json_schema::default_string_format_check);

                static_cast<void>(validator.validate(json));
            } catch (const std::exception& exception) {
                throw std::runtime_error("Validating mqttstore projection file failed: " + std::string(exception.what()));
            }
        }

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

    StoragePlan StoragePlan::fromFile(const std::string& fileName) {
        if (fileName.empty()) {
            return {};
        }

        std::ifstream planFile(fileName);
        if (!planFile.is_open()) {
            throw std::runtime_error("Cannot open mqttstore projection file '" + fileName + "'");
        }

        return fromJson(nlohmann::json::parse(std::string(std::istreambuf_iterator<char>(planFile), std::istreambuf_iterator<char>())));
    }

    StoragePlan StoragePlan::fromJson(const nlohmann::json& json) {
        validateProjectionConfiguration(json);

        StoragePlan plan;

        const nlohmann::json& projectionsJson = json.contains("projections") ? json.at("projections") : json;
        if (!projectionsJson.is_array()) {
            throw std::runtime_error("mqttstore projection configuration must be an array or contain a projections array");
        }

        for (const nlohmann::json& projectionJson : projectionsJson) {
            Projection projection;
            projection.name = projectionJson.value("name", "");
            projection.topic = projectionJson.at("topic").get<std::string>();
            projection.table = projectionJson.at("table").get<std::string>();

            const nlohmann::json& columnsJson = projectionJson.at("columns");
            if (!columnsJson.is_object()) {
                throw std::runtime_error("mqttstore projection columns must be an object");
            }

            for (auto columnIterator = columnsJson.begin(); columnIterator != columnsJson.end(); ++columnIterator) {
                ColumnMapping mapping;
                mapping.column = columnIterator.key();

                if (columnIterator.value().is_string()) {
                    mapping.jsonPointer = columnIterator.value().get<std::string>();
                } else {
                    const nlohmann::json& mappingJson = columnIterator.value();
                    mapping.jsonPointer = mappingJson.value("json_pointer", "");
                    mapping.required = mappingJson.value("required", false);

                    if (mappingJson.contains("topic_level")) {
                        mapping.topicLevel = mappingJson.at("topic_level").get<std::size_t>();
                    }
                    if (mappingJson.contains("literal")) {
                        mapping.literal = mappingJson.at("literal").get<std::string>();
                    }
                }

                projection.columns.push_back(mapping);
            }

            plan.projections.push_back(projection);
        }

        return plan;
    }

    const std::vector<StoragePlan::Projection>& StoragePlan::getProjections() const {
        return projections;
    }

    std::vector<const StoragePlan::Projection*> StoragePlan::match(const std::string& topic) const {
        std::vector<const Projection*> matches;

        for (const Projection& projection : projections) {
            if (topicMatches(projection.topic, topic)) {
                matches.push_back(&projection);
            }
        }

        return matches;
    }

    bool StoragePlan::topicMatches(const std::string& filter, const std::string& topic) {
        const std::vector<std::string> filterLevels = splitTopic(filter);
        const std::vector<std::string> topicLevels = splitTopic(topic);

        for (std::size_t level = 0; level < filterLevels.size(); ++level) {
            if (filterLevels[level] == "#") {
                return level + 1 == filterLevels.size();
            }

            if (level >= topicLevels.size()) {
                return false;
            }

            if (filterLevels[level] != "+" && filterLevels[level] != topicLevels[level]) {
                return false;
            }
        }

        return filterLevels.size() == topicLevels.size();
    }

} // namespace mqtt::mqttstore::lib

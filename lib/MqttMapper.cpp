/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MqttMapper.h"

#include <core/DynamicLoader.h>
#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Publish.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <dlfcn.h>
#include <initializer_list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

#endif

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

namespace mqtt::lib {

    MqttMapper::MqttMapper(const nlohmann::json& mappingJson)
        : mappingJson(mappingJson) {
        if (mappingJson.contains("plugins")) {
            VLOG(1) << "Loading plugins ...";

            for (const nlohmann::json& pluginJson : mappingJson["plugins"]) {
                std::string plugin = pluginJson;
                VLOG(1) << "Loading plugin: " << plugin;

                void* handle = dlOpen(plugin, RTLD_LOCAL | RTLD_LAZY);

                std::vector<mqtt::lib::Function> (*getFunctions)() =
                    reinterpret_cast<std::vector<mqtt::lib::Function> (*)()>(dlsym(handle, "getFunctions"));

                if (getFunctions != nullptr) {
                    for (const mqtt::lib::Function& function : getFunctions()) {
                        VLOG(1) << "Registering Function " << function.name;

                        if (function.numArgs >= 0) {
                            injaEnvironment.add_callback(function.name, function.numArgs, function.function);
                        } else {
                            injaEnvironment.add_callback(function.name, function.function);
                        }
                    }
                } else {
                    VLOG(1) << "No function getFunctions found";
                }
            }

            VLOG(1) << "Loading plugins done";
        }
    }

    MqttMapper::~MqttMapper() {
    }

    std::string MqttMapper::dump() {
        return mappingJson.dump();
    }

    void MqttMapper::extractTopic(const nlohmann::json& topicLevel, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        std::string name = topicLevel["name"];

        if (topicLevel.contains("subscription")) {
            uint8_t qoS = topicLevel["subscription"]["qos"];

            topicList.push_back(iot::mqtt::Topic(topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, qoS));
        }

        if (topicLevel.contains("topic_level")) {
            extractTopics(topicLevel, topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, topicList);
        }
    }

    void MqttMapper::extractTopics(const nlohmann::json& mappingJson, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        const nlohmann::json& topicLevels = mappingJson["topic_level"];

        if (topicLevels.is_object()) {
            extractTopic(topicLevels, topic, topicList);
        } else {
            for (const nlohmann::json& topicLevel : topicLevels) {
                extractTopic(topicLevel, topic, topicList);
            }
        }
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        extractTopics(mappingJson, "", topicList);

        return topicList;
    }

    void MqttMapper::publishMappedTemplate(const nlohmann::json& templateMapping,
                                           const nlohmann::json& json,
                                           const iot::mqtt::packets::Publish& publish) {
        VLOG(1) << "  -> " << templateMapping["mapped_topic"] << ":" << templateMapping["mapping_template"].dump();

        const std::string& commandTopic = templateMapping["mapped_topic"];
        const std::string& mappingTemplate = templateMapping["mapping_template"];

        try {
            // Render
            std::string message = injaEnvironment.render(mappingTemplate, json);
            VLOG(1) << "     \"" << publish.getMessage() << "\" -> \"" << message << "\"";

            const nlohmann::json& suppressions = templateMapping["suppressions"];
            bool retain = templateMapping["retain_message"];

            if (std::find(suppressions.begin(), suppressions.end(), message) == suppressions.end() || (retain && message == "")) {
                uint8_t qoS = templateMapping.value("qos_override", publish.getQoS());

                VLOG(1) << "      send mapping: " << commandTopic << ":" << message << "";
                VLOG(1) << "                    qos:" << static_cast<int>(qoS);

                publishMapping(commandTopic, message, qoS, retain);
            } else {
                VLOG(1) << "       result message '" << message << "' in suppression list:";
                for (const nlohmann::json& item : suppressions) {
                    VLOG(1) << "         '" << item.get<std::string>() << "'";
                }
                VLOG(1) << "      send mapping: suppressed";
            }
        } catch (const inja::InjaError& e) {
            LOG(ERROR) << e.what();
            LOG(ERROR) << "INJA " << e.type << ": " << e.message;
            LOG(ERROR) << "INJA (line:column):" << e.location.line << ":" << e.location.column;
            LOG(ERROR) << "Template rendering failed: " << mappingTemplate << " : " << json.dump();
        }
    }

    void MqttMapper::publishMappedTemplates(const nlohmann::json& templateMapping,
                                            const nlohmann::json& json,
                                            const iot::mqtt::packets::Publish& publish) {
        if (templateMapping.is_object()) {
            publishMappedTemplate(templateMapping, json, publish);
        } else {
            for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                publishMappedTemplate(concreteTemplateMapping, json, publish);
            }
        }
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping,
                                          const std::string& message,
                                          const iot::mqtt::packets::Publish& publish) {
        const std::string& commandTopic = staticMapping["mapped_topic"];
        bool retain = staticMapping["retain_message"];
        uint8_t qoS = staticMapping.value("qos_override", publish.getQoS());

        VLOG(1) << "     \"" << publish.getMessage() << "\" -> \"" << message << "\"";
        VLOG(1) << "      send mapping: " << commandTopic << ":" << message;
        VLOG(1) << "                    qos:" << static_cast<int>(qoS);

        publishMapping(commandTopic, message, qoS, retain);
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json& messageMapping = staticMapping["message_mapping"];

        if (messageMapping.is_object()) {
            VLOG(1) << "  -> " << staticMapping["mapped_topic"] << ":" << messageMapping.dump();

            if (messageMapping["message"] == publish.getMessage()) {
                publishMappedMessage(staticMapping, messageMapping["mapped_message"], publish);
            } else {
                VLOG(1) << "      no matching mapped message found";
            }
        } else {
            const nlohmann::json::const_iterator matchedMessageMappingIterator =
                std::find_if(messageMapping.begin(),
                             messageMapping.end(),
                             [&publish, &messageMapping, &staticMapping](const nlohmann::json& messageMappingCandidat) {
                                 VLOG(1) << "  -> " << staticMapping["mapped_topic"] << ":" << messageMapping.dump();

                                 return messageMappingCandidat["message"] == publish.getMessage();
                             });

            if (matchedMessageMappingIterator != messageMapping.end()) {
                publishMappedMessage(staticMapping, (*matchedMessageMappingIterator)["mapped_message"], publish);
            } else {
                VLOG(1) << "      no matching mapped message found";
            }
        }
    }

    void MqttMapper::publishMappedMessages(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        if (staticMapping.is_object()) {
            publishMappedMessage(staticMapping, publish);
        } else if (staticMapping.is_array()) {
            for (const nlohmann::json& concreteStaticMapping : staticMapping) {
                publishMappedMessage(concreteStaticMapping, publish);
            }
        }
    }

    nlohmann::json MqttMapper::findMatchingTopicLevel(const nlohmann::json& topicLevel, const std::string& topic) {
        nlohmann::json foundTopicLevel;

        if (topicLevel.is_object()) {
            std::string::size_type slashPosition = topic.find("/");
            std::string topicLevelName = topic.substr(0, slashPosition);

            if (topicLevel["name"] == topicLevelName) {
                if (slashPosition == std::string::npos) {
                    foundTopicLevel = topicLevel;
                } else if (topicLevel.contains("topic_level")) {
                    foundTopicLevel = findMatchingTopicLevel(topicLevel["topic_level"], topic.substr(slashPosition + 1));
                }
            }
        } else if (topicLevel.is_array()) {
            for (const nlohmann::json& topicLevelEntry : topicLevel) {
                foundTopicLevel = findMatchingTopicLevel(topicLevelEntry, topic);

                if (!foundTopicLevel.empty()) {
                    break;
                }
            }
        }

        return foundTopicLevel;
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        if (!mappingJson.empty()) {
            nlohmann::json matchingTopicLevel = findMatchingTopicLevel(mappingJson["topic_level"], publish.getTopic());

            if (!matchingTopicLevel.empty()) {
                const nlohmann::json& mapping = matchingTopicLevel["subscription"];

                if (mapping.contains("static")) {
                    VLOG(1) << "Topic mapping (static) found: \"" << publish.getTopic() << "\":\"" << publish.getMessage() << "\"";

                    publishMappedMessages(mapping["static"], publish);
                } else {
                    nlohmann::json json;
                    nlohmann::json templateMapping;

                    if (mapping.contains("value")) {
                        VLOG(1) << "Topic mapping (value) found: \"" << publish.getTopic() << "\":\"" << publish.getMessage() << "\"";

                        templateMapping = mapping["value"];

                        json["value"] = publish.getMessage();

                    } else if (mapping.contains("json")) {
                        VLOG(1) << "Topic mapping (json) found: \"" << publish.getTopic() << "\":\"" << publish.getMessage() << "\"";

                        templateMapping = mapping["json"];

                        try {
                            json = nlohmann::json::parse(publish.getMessage());
                        } catch (const nlohmann::json::parse_error& e) {
                            LOG(ERROR) << e.what() << ": " << e.id;
                            LOG(ERROR) << "Parsing message into json failed: " << publish.getMessage();
                            LOG(ERROR) << "Parse Error: Message: " << e.what() << '\n'
                                       << "Exception Id: " << e.id << '\n'
                                       << "Byte position of error: " << e.byte;
                            json.clear();
                        }
                    }

                    if (!json.empty()) {
                        publishMappedTemplates(templateMapping, json, publish);
                    } else {
                        VLOG(1) << "No valid mapping section found: " << matchingTopicLevel.dump();
                    }
                }
            }
        }
    }

} // namespace mqtt::lib

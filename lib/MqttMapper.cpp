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

#include "MqttMapperPlugin.h"

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

                void* handle = dlOpen(plugin, RTLD_LOCAL | RTLD_LAZY);

                if (handle != nullptr) {
                    VLOG(1) << "  Loading plugin: " << plugin << " ...";

                    std::vector<mqtt::lib::Function>* functions =
                        static_cast<std::vector<mqtt::lib::Function>*>(dlsym(handle, "functions"));
                    if (functions != nullptr) {
                        VLOG(0) << "  Registering inja 'none void callbacks'";
                        for (const mqtt::lib::Function& function : *functions) {
                            VLOG(1) << "    " << function.name;

                            if (function.numArgs >= 0) {
                                injaEnvironment.add_callback(function.name, function.numArgs, function.function);
                            } else {
                                injaEnvironment.add_callback(function.name, function.function);
                            }
                        }
                        VLOG(0) << "  Registering inja 'none void callbacks done'";
                    } else {
                        VLOG(1) << "  No inja none 'void callbacks found' in plugin " << plugin;
                    }

                    std::vector<mqtt::lib::VoidFunction>* voidFunctions =
                        static_cast<std::vector<mqtt::lib::VoidFunction>*>(dlsym(handle, "voidFunctions"));
                    if (voidFunctions != nullptr) {
                        VLOG(0) << "  Registering inja 'void callbacks'";
                        for (const mqtt::lib::VoidFunction& voidFunction : *voidFunctions) {
                            VLOG(1) << "    " << voidFunction.name;

                            if (voidFunction.numArgs >= 0) {
                                injaEnvironment.add_void_callback(voidFunction.name, voidFunction.numArgs, voidFunction.function);
                            } else {
                                injaEnvironment.add_void_callback(voidFunction.name, voidFunction.function);
                            }
                        }
                        VLOG(0) << "  Registering inja 'void callbacks' done";
                    } else {
                        VLOG(1) << "  No inja 'void callbacks' found in plugin " << plugin;
                    }

                    VLOG(1) << "  Loading plugin done: " << plugin;
                } else {
                    VLOG(1) << "  Error loading plugin: " << plugin;
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

    std::list<iot::mqtt::Topic> MqttMapper::extractSubscriptions() {
        std::list<iot::mqtt::Topic> topicList;

        extractSubscriptions(mappingJson, "", topicList);

        return topicList;
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        if (!mappingJson.empty()) {
            nlohmann::json matchingTopicLevel = findMatchingTopicLevel(mappingJson["topic_level"], publish.getTopic());

            if (!matchingTopicLevel.empty()) {
                const nlohmann::json& subscription = matchingTopicLevel["subscription"];

                if (subscription.contains("static")) {
                    VLOG(1) << "Topic mapping found:";
                    VLOG(1) << "  Type: static";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();

                    publishMappedMessages(subscription["static"], publish);
                }

                if (subscription.contains("value")) {
                    VLOG(1) << "Topic mapping found:";
                    VLOG(1) << "  Type: value";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();

                    nlohmann::json json;
                    json["message"] = publish.getMessage();

                    publishMappedTemplates(subscription["value"], json, publish);
                }

                if (subscription.contains("json")) {
                    VLOG(1) << "Topic mapping found";
                    VLOG(1) << "  Type: json";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();

                    try {
                        nlohmann::json json;
                        json["message"] = nlohmann::json::parse(publish.getMessage());

                        publishMappedTemplates(subscription["json"], json, publish);
                    } catch (const nlohmann::json::parse_error& e) {
                        LOG(ERROR) << "  Parsing message into json failed: " << publish.getMessage();
                        LOG(ERROR) << "     What: " << e.what() << '\n'
                                   << "     Exception Id: " << e.id << '\n'
                                   << "     Byte position of error: " << e.byte;
                    }
                }
            }
        }
    }

    void
    MqttMapper::extractSubscription(const nlohmann::json& topicLevel, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        std::string name = topicLevel["name"];

        if (topicLevel.contains("subscription")) {
            uint8_t qoS = topicLevel["subscription"]["qos"];

            topicList.push_back(iot::mqtt::Topic(topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, qoS));
        }

        if (topicLevel.contains("topic_level")) {
            extractSubscriptions(topicLevel, topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, topicList);
        }
    }

    void
    MqttMapper::extractSubscriptions(const nlohmann::json& mappingJson, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        const nlohmann::json& topicLevels = mappingJson["topic_level"];

        if (topicLevels.is_object()) {
            extractSubscription(topicLevels, topic, topicList);
        } else {
            for (const nlohmann::json& topicLevel : topicLevels) {
                extractSubscription(topicLevel, topic, topicList);
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

    void MqttMapper::publishMappedTemplate(const nlohmann::json& templateMapping,
                                           nlohmann::json& json,
                                           const iot::mqtt::packets::Publish& publish) {
        const std::string& mappingTemplate = templateMapping["mapping_template"];
        const std::string& mappedTopic = templateMapping["mapped_topic"];

        try {
            // Render topic
            std::string renderedTopic = injaEnvironment.render(mappedTopic, json);
            json["mapped_topic"] = renderedTopic;

            VLOG(1) << "  Mapped topic template: " << mappedTopic;
            VLOG(1) << "    -> " << renderedTopic;

            try {
                // Render message
                std::string renderedMessage = injaEnvironment.render(mappingTemplate, json);
                VLOG(1) << "  Mapped message template: " << mappingTemplate;
                VLOG(1) << "    -> " << renderedMessage;

                const nlohmann::json& suppressions = templateMapping["suppressions"];
                bool retain = templateMapping.value("retain", publish.getRetain());

                if (std::find(suppressions.begin(), suppressions.end(), renderedMessage) == suppressions.end() ||
                    (retain && renderedMessage == "")) {
                    uint8_t qoS = templateMapping.value("qos", publish.getQoS());

                    VLOG(1) << "  Send mapping:";
                    VLOG(1) << "    Topic: " << renderedTopic;
                    VLOG(1) << "    Message: " << renderedMessage << "";
                    VLOG(1) << "    qos: " << static_cast<int>(qoS);
                    VLOG(1) << "    retain: " << retain;

                    publishMapping(renderedTopic, renderedMessage, qoS, retain);
                } else {
                    VLOG(1) << "    rendered message '" << renderedMessage << "' in suppression list:";
                    for (const nlohmann::json& item : suppressions) {
                        VLOG(1) << "         '" << item.get<std::string>() << "'";
                    }
                    VLOG(1) << "  send mapping: suppressed";
                }
            } catch (const inja::InjaError& e) {
                LOG(ERROR) << "  Message template rendering failed: " << mappingTemplate << " : " << json.dump();
                LOG(ERROR) << "    What: " << e.what();
                LOG(ERROR) << "    INJA: " << e.type << ": " << e.message;
                LOG(ERROR) << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
            }
        } catch (const inja::InjaError& e) {
            LOG(ERROR) << "  Topic template rendering failed: " << mappingTemplate << " : " << json.dump();
            LOG(ERROR) << "    What: " << e.what();
            LOG(ERROR) << "    INJA: " << e.type << ": " << e.message;
            LOG(ERROR) << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
        }
    }

    void MqttMapper::publishMappedTemplates(const nlohmann::json& templateMapping,
                                            nlohmann::json& json,
                                            const iot::mqtt::packets::Publish& publish) {
        json["topic"] = publish.getTopic();
        json["qos"] = publish.getQoS();
        json["retain"] = publish.getRetain();
        json["package_identifier"] = publish.getPacketIdentifier();

        VLOG(0) << "  Render data: " << json.dump();

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
        const std::string& mappedTopic = staticMapping["mapped_topic"];
        bool retain = staticMapping.value("retain", publish.getRetain());
        uint8_t qoS = staticMapping.value("qos", publish.getQoS());

        VLOG(1) << "  Mapped topic:";
        VLOG(1) << "    -> " << mappedTopic;
        VLOG(1) << "  Mapped message:";
        VLOG(1) << "    -> " << message;
        VLOG(1) << "  Send mapping:";
        VLOG(1) << "    Topic: " << mappedTopic;
        VLOG(1) << "    Message: " << message;
        VLOG(1) << "    qos: " << static_cast<int>(qoS);
        VLOG(1) << "    retain: " << retain;

        publishMapping(mappedTopic, message, qoS, retain);
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json& messageMapping = staticMapping["message_mapping"];

        VLOG(0) << "  Message mapping: " << messageMapping.dump();

        if (messageMapping.is_object()) {
            if (messageMapping["message"] == publish.getMessage()) {
                publishMappedMessage(staticMapping, messageMapping["mapped_message"], publish);
            } else {
                VLOG(1) << "    no matching mapped message found";
            }
        } else {
            const nlohmann::json::const_iterator matchedMessageMappingIterator =
                std::find_if(messageMapping.begin(), messageMapping.end(), [&publish](const nlohmann::json& messageMappingCandidat) {
                    return messageMappingCandidat["message"] == publish.getMessage();
                });

            if (matchedMessageMappingIterator != messageMapping.end()) {
                publishMappedMessage(staticMapping, (*matchedMessageMappingIterator)["mapped_message"], publish);
            } else {
                VLOG(1) << "    no matching mapped message found";
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

} // namespace mqtt::lib

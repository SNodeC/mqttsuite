/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "MqttMapper.h"

#include "MqttMapperPlugin.h"

#include <cmath>
#include <core/DynamicLoader.h>
#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Publish.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#endif
#include "inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <dlfcn.h>
#include <SemanticLog.h>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#endif

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

namespace mqtt::lib {

    MqttMapper::MqttMapper(const nlohmann::json& mappingJson)
        : mappingJson(mappingJson) {
        injaEnvironment = new inja::Environment;

        if (mappingJson.contains("plugins")) {
            snode::semantic::appLog().trace() << "Loading plugins ...";

            for (const nlohmann::json& pluginJson : mappingJson["plugins"]) {
                const std::string plugin = pluginJson;

                void* handle = core::DynamicLoader::dlOpen(plugin);

                if (handle != nullptr) {
                    pluginHandles.push_back(handle);

                    snode::semantic::appLog().trace() << "  Loading plugin: " << plugin << " ...";

                    const std::vector<mqtt::lib::Function>* loadedFunctions =
                        static_cast<std::vector<mqtt::lib::Function>*>(dlsym(handle, "functions"));
                    if (loadedFunctions != nullptr) {
                        snode::semantic::appLog().trace() << "  Registering inja 'none void callbacks'";
                        for (const mqtt::lib::Function& function : *loadedFunctions) {
                            snode::semantic::appLog().trace() << "    " << function.name;

                            if (function.numArgs >= 0) {
                                injaEnvironment->add_callback(function.name, function.numArgs, function.function);
                            } else {
                                injaEnvironment->add_callback(function.name, function.function);
                            }
                        }
                        snode::semantic::appLog().trace() << "  Registering inja 'none void callbacks done'";
                    } else {
                        snode::semantic::appLog().trace() << "  No inja none 'void callbacks found' in plugin " << plugin;
                    }

                    const std::vector<mqtt::lib::VoidFunction>* loadedVoidFunctions =
                        static_cast<std::vector<mqtt::lib::VoidFunction>*>(dlsym(handle, "voidFunctions"));
                    if (loadedVoidFunctions != nullptr) {
                        snode::semantic::appLog().trace() << "  Registering inja 'void callbacks'";
                        for (const mqtt::lib::VoidFunction& voidFunction : *loadedVoidFunctions) {
                            snode::semantic::appLog().trace() << "    " << voidFunction.name;

                            if (voidFunction.numArgs >= 0) {
                                injaEnvironment->add_void_callback(voidFunction.name, voidFunction.numArgs, voidFunction.function);
                            } else {
                                injaEnvironment->add_void_callback(voidFunction.name, voidFunction.function);
                            }
                        }
                        snode::semantic::appLog().trace() << "  Registering inja 'void callbacks' done";
                    } else {
                        snode::semantic::appLog().trace() << "  No inja 'void callbacks' found in plugin " << plugin;
                    }

                    snode::semantic::appLog().trace() << "  Loading plugin done: " << plugin;
                } else {
                    snode::semantic::appLog().trace() << "  Error loading plugin: " << plugin;
                }
            }

            snode::semantic::appLog().trace() << "Loading plugins done";
        }
    }

    MqttMapper::~MqttMapper() {
        delete injaEnvironment;

        for (void* pluginHandle : pluginHandles) {
            core::DynamicLoader::dlClose(pluginHandle);
        }
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
                    snode::semantic::appLog().trace() << "Topic mapping found for:";
                    snode::semantic::appLog().trace() << "  Type: static";
                    snode::semantic::appLog().trace() << "  Topic: " << publish.getTopic();
                    snode::semantic::appLog().trace() << "  Message: " << publish.getMessage();
                    snode::semantic::appLog().trace() << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    snode::semantic::appLog().trace() << "  Retain: " << publish.getRetain();

                    publishMappedMessages(subscription["static"], publish);
                }

                if (subscription.contains("value")) {
                    snode::semantic::appLog().trace() << "Topic mapping found for:";
                    snode::semantic::appLog().trace() << "  Type: value";
                    snode::semantic::appLog().trace() << "  Topic: " << publish.getTopic();
                    snode::semantic::appLog().trace() << "  Message: " << publish.getMessage();
                    snode::semantic::appLog().trace() << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    snode::semantic::appLog().trace() << "  Retain: " << publish.getRetain();

                    nlohmann::json json;
                    json["message"] = publish.getMessage();

                    publishMappedTemplates(subscription["value"], json, publish);
                }

                if (subscription.contains("json")) {
                    snode::semantic::appLog().trace() << "Topic mapping found for:";
                    snode::semantic::appLog().trace() << "  Type: json";
                    snode::semantic::appLog().trace() << "  Topic: " << publish.getTopic();
                    snode::semantic::appLog().trace() << "  Message: " << publish.getMessage();
                    snode::semantic::appLog().trace() << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    snode::semantic::appLog().trace() << "  Retain: " << publish.getRetain();

                    try {
                        nlohmann::json json;
                        json["message"] = nlohmann::json::parse(publish.getMessage());

                        publishMappedTemplates(subscription["json"], json, publish);
                    } catch (const nlohmann::json::parse_error& e) {
                        snode::semantic::appLog().trace() << "  Parsing message into json failed: " << publish.getMessage();
                        snode::semantic::appLog().trace() << "     What: " << e.what() << '\n'
                                << "     Exception Id: " << e.id << '\n'
                                << "     Byte position of error: " << e.byte;
                    }
                }
            }
        }
    }

    void MqttMapper::extractSubscription(const nlohmann::json& topicLevelJson,
                                         const std::string& topic,
                                         std::list<iot::mqtt::Topic>& topicList) {
        const std::string name = topicLevelJson["name"];

        if (topicLevelJson.contains("subscription")) {
            const uint8_t qoS = topicLevelJson["subscription"]["qos"];

            topicList.emplace_back(topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, qoS);
        }

        if (topicLevelJson.contains("topic_level")) {
            extractSubscriptions(topicLevelJson, topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, topicList);
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
            const std::string::size_type slashPosition = topic.find('/');
            const std::string topicLevelName = topic.substr(0, slashPosition);

            if (topicLevel["name"] == topicLevelName || topicLevel["name"] == "+" || topicLevel["name"] == "#") {
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

    void MqttMapper::publishMappedTemplate(const nlohmann::json& templateMapping, nlohmann::json& json) {
        const std::string& mappingTemplate = templateMapping["mapping_template"];
        const std::string& mappedTopic = templateMapping["mapped_topic"];

        try {
            // Render topic
            const std::string renderedTopic = injaEnvironment->render(mappedTopic, json);
            json["mapped_topic"] = renderedTopic;

            snode::semantic::appLog().trace() << "  Mapped topic template: " << mappedTopic;
            snode::semantic::appLog().trace() << "    -> " << renderedTopic;

            try {
                // Render message
                const std::string renderedMessage = injaEnvironment->render(mappingTemplate, json);
                snode::semantic::appLog().trace() << "  Mapped message template: " << mappingTemplate;
                snode::semantic::appLog().trace() << "    -> " << renderedMessage;

                const nlohmann::json& suppressions = templateMapping["suppressions"];
                const bool retain = templateMapping["retain"];

                if (suppressions.empty() || std::find(suppressions.begin(), suppressions.end(), renderedMessage) == suppressions.end() ||
                    (retain && renderedMessage.empty())) {
                    const uint8_t qoS = templateMapping["qos"];

                    snode::semantic::appLog().trace() << "  Send mapping:";
                    snode::semantic::appLog().trace() << "    Topic: " << renderedTopic;
                    snode::semantic::appLog().trace() << "    Message: " << renderedMessage << "";
                    snode::semantic::appLog().trace() << "    QoS: " << static_cast<int>(qoS);
                    snode::semantic::appLog().trace() << "    retain: " << retain;

                    publishMapping(renderedTopic, renderedMessage, qoS, retain);
                } else {
                    snode::semantic::appLog().trace() << "    Rendered message: '" << renderedMessage << "' in suppression list:";
                    for (const nlohmann::json& item : suppressions) {
                        snode::semantic::appLog().trace() << "         '" << item.get<std::string>() << "'";
                    }
                    snode::semantic::appLog().trace() << "  Send mapping: suppressed";
                }
            } catch (const inja::InjaError& e) {
                snode::semantic::appLog().trace() << "  Message template rendering failed: " << mappingTemplate << " : " << json.dump();
                snode::semantic::appLog().trace() << "    What: " << e.what();
                snode::semantic::appLog().trace() << "    INJA: " << e.type << ": " << e.message;
                snode::semantic::appLog().trace() << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
            }
        } catch (const inja::InjaError& e) {
            snode::semantic::appLog().trace() << "  Topic template rendering failed: " << mappingTemplate << " : " << json.dump();
            snode::semantic::appLog().trace() << "    What: " << e.what();
            snode::semantic::appLog().trace() << "    INJA: " << e.type << ": " << e.message;
            snode::semantic::appLog().trace() << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
        }
    }

    void MqttMapper::publishMappedTemplates(const nlohmann::json& templateMapping,
                                            nlohmann::json& json,
                                            const iot::mqtt::packets::Publish& publish) {
        json["topic"] = publish.getTopic();
        json["qos"] = publish.getQoS();
        json["retain"] = publish.getRetain();
        json["package_identifier"] = publish.getPacketIdentifier();

        try {
            snode::semantic::appLog().trace() << "  Render data: " << json.dump();

            if (templateMapping.is_object()) {
                publishMappedTemplate(templateMapping, json);
            } else {
                for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                    publishMappedTemplate(concreteTemplateMapping, json);
                }
            }
        } catch (const nlohmann::json::exception& e) {
            snode::semantic::appLog().trace() << "JSON Exception during Render data:\n" << e.what();
        }
    }

    void MqttMapper::publishMappedMessage(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        snode::semantic::appLog().trace() << "  Mapped topic:";
        snode::semantic::appLog().trace() << "    -> " << topic;
        snode::semantic::appLog().trace() << "  Mapped message:";
        snode::semantic::appLog().trace() << "    -> " << message;
        snode::semantic::appLog().trace() << "  Send mapping:";
        snode::semantic::appLog().trace() << "    Topic: " << topic;
        snode::semantic::appLog().trace() << "    Message: " << message;
        snode::semantic::appLog().trace() << "    QoS: " << static_cast<int>(qoS);
        snode::semantic::appLog().trace() << "    retain: " << retain;

        publishMapping(topic, message, qoS, retain);
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json& messageMapping = staticMapping["message_mapping"];

        snode::semantic::appLog().trace() << "  Message mapping: " << messageMapping.dump();

        if (messageMapping.is_object()) {
            if (messageMapping["message"] == publish.getMessage()) {
                publishMappedMessage(
                    staticMapping["mapped_topic"], messageMapping["mapped_message"], staticMapping["qos"], staticMapping["retain"]);
            } else {
                snode::semantic::appLog().trace() << "    no matching mapped message found";
            }
        } else {
            const nlohmann::json::const_iterator matchedMessageMappingIterator =
                std::find_if(messageMapping.begin(), messageMapping.end(), [&publish](const nlohmann::json& messageMappingCandidat) {
                    return messageMappingCandidat["message"] == publish.getMessage();
                });

            if (matchedMessageMappingIterator != messageMapping.end()) {
                publishMappedMessage(staticMapping["mapped_topic"],
                                     (*matchedMessageMappingIterator)["mapped_message"],
                                     staticMapping["qos"],
                                     staticMapping["retain"]);
            } else {
                snode::semantic::appLog().trace() << "    no matching mapped message found";
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

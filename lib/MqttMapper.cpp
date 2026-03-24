/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
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

#include <core/DynamicLoader.h>
#include <iot/mqtt/Topic.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <cmath>
#include <exception>

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#if __has_warning("-Wsuggest-override")
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#if __has_warning("-Wmissing-noreturn")
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#if __has_warning("-Wdeprecated-copy-with-user-provided-dtor")
#pragma GCC diagnostic ignored "-Wdeprecated-copy-with-user-provided-dtor"
#endif
#endif
#endif
#include "inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

#endif

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

namespace mqtt::lib {

#include "mapping-schema.json.h" // definition of mappingJsonSchemaString

    const nlohmann::json_schema::json_validator
        MqttMapper::validator(nlohmann::json::parse(mappingJsonSchemaString), nullptr, nlohmann::json_schema::default_string_format_check);

    MqttMapper::MqttMapper()
        : injaEnvironment(new inja::Environment) {
    }

    MqttMapper::~MqttMapper() {
        delete injaEnvironment;

        for (void* pluginHandle : pluginHandles) {
            core::DynamicLoader::dlClose(pluginHandle);
        }
    }

    const std::string& MqttMapper::getSchema() {
        return mappingJsonSchemaString;
    }

    bool MqttMapper::setMapping(nlohmann::json mappingJson) { // can throw
        delete injaEnvironment;

        for (void* handle : pluginHandles) {
            core::DynamicLoader::dlClose(handle);
        }
        pluginHandles.clear();

        injaEnvironment = new inja::Environment;

        nlohmann::json defaultPatch;

        try {
            defaultPatch = validator.validate(mappingJson);
        } catch (const std::exception& e) {
            throw std::runtime_error("Validating JSON failed: Mapping JSON = " + mappingJson.dump(4) + "\n" + e.what());
        }

        try {
            mappingJson = mappingJson.patch(defaultPatch);
        } catch (const std::exception& e) {
            throw std::runtime_error("Patching JSON with default patch failed: Default patch = " + defaultPatch.dump(4) + "\n" + e.what());
        }

        bool mustReconnect = this->mappingJson["connection"] != mappingJson["connection"];

        this->mappingJson = mappingJson;

        if (mappingJson["mapping"].contains("plugins")) {
            for (const nlohmann::json& pluginJson : mappingJson["mapping"]["plugins"]) {
                const std::string plugin = pluginJson;

                void* handle = core::DynamicLoader::dlOpen(plugin);

                if (handle != nullptr) {
                    pluginHandles.push_back(handle);

                    VLOG(1) << "  Loading plugin: " << plugin << " ...";

                    const std::vector<mqtt::lib::Function>* loadedFunctions =
                        static_cast<std::vector<mqtt::lib::Function>*>(core::DynamicLoader::dlSym(handle, "functions"));
                    if (loadedFunctions != nullptr) {
                        VLOG(1) << "  Registering inja 'none void callbacks'";
                        for (const mqtt::lib::Function& function : *loadedFunctions) {
                            VLOG(1) << "    " << function.name;

                            if (function.numArgs >= 0) {
                                injaEnvironment->add_callback(function.name, function.numArgs, function.function);
                            } else {
                                injaEnvironment->add_callback(function.name, function.function);
                            }
                        }
                        VLOG(1) << "  Registering inja 'none void callbacks done'";
                    } else {
                        VLOG(1) << "  No inja none 'void callbacks found' in plugin " << plugin;
                    }

                    const std::vector<mqtt::lib::VoidFunction>* loadedVoidFunctions =
                        static_cast<std::vector<mqtt::lib::VoidFunction>*>(core::DynamicLoader::dlSym(handle, "voidFunctions"));
                    if (loadedVoidFunctions != nullptr) {
                        VLOG(1) << "  Registering inja 'void callbacks'";
                        for (const mqtt::lib::VoidFunction& voidFunction : *loadedVoidFunctions) {
                            VLOG(1) << "    " << voidFunction.name;

                            if (voidFunction.numArgs >= 0) {
                                injaEnvironment->add_void_callback(voidFunction.name, voidFunction.numArgs, voidFunction.function);
                            } else {
                                injaEnvironment->add_void_callback(voidFunction.name, voidFunction.function);
                            }
                        }
                        VLOG(1) << "  Registering inja 'void callbacks' done";
                    } else {
                        VLOG(1) << "  No inja 'void callbacks' found in plugin " << plugin;
                    }

                    VLOG(1) << "  Loading plugin done: " << plugin;
                } else {
                    VLOG(1) << "  Error loading plugin: " << plugin;
                    throw std::runtime_error("Error loading plugin '" + plugin + "': " + core::DynamicLoader::dlError());
                }
            }

            VLOG(1) << "Loading plugins done";
        }

        return mustReconnect;
    }

    const nlohmann::json& MqttMapper::getMapping() const {
        return mappingJson;
    }

    std::string MqttMapper::dump() {
        return mappingJson.dump();
    }

    const nlohmann::json& MqttMapper::getConnection() const {
        return mappingJson["connection"];
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractSubscriptions() const {
        std::list<iot::mqtt::Topic> topicList;

        extractSubscriptions(mappingJson["mapping"], "", topicList);

        return topicList;
    }

    const nlohmann::json MqttMapper::validate(const nlohmann::json& json) {
        return validator.validate(json);
    }

    const nlohmann::json MqttMapper::validate(const nlohmann::json& json, nlohmann::json_schema::basic_error_handler& err) {
        return validator.validate(json, err);
    }

    MqttMapper::MappedPublishes MqttMapper::getMappings(const iot::mqtt::packets::Publish& publish) {
        MappedPublishes mappedPublishes;
        if (mappingJson.contains("mapping") && !mappingJson["mapping"].empty()) {
            nlohmann::json matchingTopicLevel = findMatchingTopicLevel(mappingJson["mapping"]["topic_level"], publish.getTopic());

            if (!matchingTopicLevel.empty()) {
                const nlohmann::json& subscription = matchingTopicLevel["subscription"];

                if (subscription.contains("static")) {
                    VLOG(1) << "Topic mapping found for:";
                    VLOG(1) << "  Type: static";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();
                    VLOG(1) << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    VLOG(1) << "  Retain: " << publish.getRetain();

                    getStaticMappings(subscription["static"], publish, mappedPublishes);
                }

                if (subscription.contains("value")) {
                    VLOG(1) << "Topic mapping found for:";
                    VLOG(1) << "  Type: value";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();
                    VLOG(1) << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    VLOG(1) << "  Retain: " << publish.getRetain();

                    nlohmann::json json;
                    json["message"] = publish.getMessage();

                    getTemplateMappings(subscription["value"], json, publish, mappedPublishes);
                }

                if (subscription.contains("json")) {
                    VLOG(1) << "Topic mapping found for:";
                    VLOG(1) << "  Type: json";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();
                    VLOG(1) << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    VLOG(1) << "  Retain: " << publish.getRetain();

                    try {
                        nlohmann::json json;
                        json["message"] = nlohmann::json::parse(publish.getMessage());

                        getTemplateMappings(subscription["json"], json, publish, mappedPublishes);
                    } catch (const nlohmann::json::parse_error& e) {
                        VLOG(1) << "  Parsing message into json failed: " << publish.getMessage();
                        VLOG(1) << "     What: " << e.what() << '\n'
                                << "     Exception Id: " << e.id << '\n'
                                << "     Byte position of error: " << e.byte;
                    }
                }
            }
        }

        return mappedPublishes;
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
        if (mappingJson.contains("topic_level")) {
            const nlohmann::json& topicLevels = mappingJson["topic_level"];

            if (topicLevels.is_object()) {
                extractSubscription(topicLevels, topic, topicList);
            } else {
                for (const nlohmann::json& topicLevel : topicLevels) {
                    extractSubscription(topicLevel, topic, topicList);
                }
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

    void MqttMapper::publishMappedTemplate(const nlohmann::json& templateMapping, nlohmann::json& json, MappedPublishes& mappedPublishes) {
        const std::string& mappingTemplate = templateMapping["mapping_template"];
        const std::string& mappedTopic = templateMapping["mapped_topic"];

        try {
            // Render topic
            const std::string renderedTopic = injaEnvironment->render(mappedTopic, json);
            json["mapped_topic"] = renderedTopic;

            VLOG(1) << "  Mapped topic template: " << mappedTopic;
            VLOG(1) << "    -> " << renderedTopic;

            try {
                // Render message
                const std::string renderedMessage = injaEnvironment->render(mappingTemplate, json);
                VLOG(1) << "  Mapped message template: " << mappingTemplate;
                VLOG(1) << "    -> " << renderedMessage;

                const nlohmann::json& suppressions = templateMapping["suppressions"];
                const bool retain = templateMapping["retain"];

                if (suppressions.empty() || std::find(suppressions.begin(), suppressions.end(), renderedMessage) == suppressions.end() ||
                    (retain && renderedMessage.empty())) {
                    const uint8_t qoS = templateMapping["qos"];
                    const double delay = templateMapping["delay"];

                    VLOG(1) << "  Send mapping:" << (delay > 0 ? " delayed" : "");
                    VLOG(1) << "    Topic: " << renderedTopic;
                    VLOG(1) << "    Message: " << renderedMessage << "";
                    VLOG(1) << "    QoS: " << static_cast<int>(qoS);
                    VLOG(1) << "    retain: " << retain;
                    VLOG(1) << "    Delay: " << delay;

                    publishMappedMessage(renderedTopic, renderedMessage, qoS, retain, delay, mappedPublishes);
                } else {
                    VLOG(1) << "    Rendered message: '" << renderedMessage << "' in suppression list:";
                    for (const nlohmann::json& item : suppressions) {
                        VLOG(1) << "         '" << item.get<std::string>() << "'";
                    }
                    VLOG(1) << "  Send mapping: suppressed";
                }
            } catch (const inja::InjaError& e) {
                VLOG(1) << "  Message template rendering failed: " << mappingTemplate << " : " << json.dump();
                VLOG(1) << "    What: " << e.what();
                VLOG(1) << "    INJA: " << e.type << ": " << e.message;
                VLOG(1) << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
            }
        } catch (const inja::InjaError& e) {
            VLOG(1) << "  Topic template rendering failed: " << mappingTemplate << " : " << json.dump();
            VLOG(1) << "    What: " << e.what();
            VLOG(1) << "    INJA: " << e.type << ": " << e.message;
            VLOG(1) << "    INJA (line:column):" << e.location.line << ":" << e.location.column;
        }
    }

    void MqttMapper::getTemplateMappings(const nlohmann::json& templateMapping,
                                         nlohmann::json& json,
                                         const iot::mqtt::packets::Publish& publish,
                                         MappedPublishes& mappedPublishes) {
        json["topic"] = publish.getTopic();
        json["qos"] = publish.getQoS();
        json["retain"] = publish.getRetain();
        json["package_identifier"] = publish.getPacketIdentifier();

        try {
            VLOG(1) << "  Render data: " << json.dump();

            if (templateMapping.is_object()) {
                publishMappedTemplate(templateMapping, json, mappedPublishes);
            } else {
                for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                    publishMappedTemplate(concreteTemplateMapping, json, mappedPublishes);
                }
            }
        } catch (const nlohmann::json::exception& e) {
            VLOG(1) << "JSON Exception during Render data:\n" << e.what();
        }
    }

    void MqttMapper::publishMappedMessage(
        const std::string& topic, const std::string& message, uint8_t qoS, bool retain, double delay, MappedPublishes& mappedPublishes) {
        VLOG(1) << "  Mapped topic:";
        VLOG(1) << "    -> " << topic;
        VLOG(1) << "  Mapped message:";
        VLOG(1) << "    -> " << message;
        VLOG(1) << "  Send mapping:" << (delay > 0 ? " delayed" : "");
        VLOG(1) << "    Topic: " << topic;
        VLOG(1) << "    Message: " << message;
        VLOG(1) << "    QoS: " << static_cast<int>(qoS);
        VLOG(1) << "    retain: " << retain;
        VLOG(1) << "    Delay: " << delay;

        if (delay < 0.0) {
            mappedPublishes.first.emplace_back(0, topic, message, qoS, false, retain);
        } else {
            mappedPublishes.second.push_back({delay, iot::mqtt::packets::Publish(0, topic, message, qoS, false, retain)});
        }
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping,
                                          const iot::mqtt::packets::Publish& publish,
                                          MappedPublishes& mappedPublishes) {
        const nlohmann::json& messageMapping = staticMapping["message_mapping"];

        VLOG(1) << "  Message mapping: " << messageMapping.dump();

        if (messageMapping.is_object()) {
            if (messageMapping["message"] == publish.getMessage()) {
                publishMappedMessage(staticMapping["mapped_topic"],
                                     messageMapping["mapped_message"],
                                     staticMapping["qos"],
                                     staticMapping["retain"],
                                     staticMapping["delay"],
                                     mappedPublishes);
            } else {
                VLOG(1) << "    no matching mapped message found";
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
                                     staticMapping["retain"],
                                     staticMapping["delay"],
                                     mappedPublishes);
            } else {
                VLOG(1) << "    no matching mapped message found";
            }
        }
    }

    void MqttMapper::getStaticMappings(const nlohmann::json& staticMapping,
                                       const iot::mqtt::packets::Publish& publish,
                                       MappedPublishes& mappedPublishes) {
        if (staticMapping.is_object()) {
            publishMappedMessage(staticMapping, publish, mappedPublishes);
        } else if (staticMapping.is_array()) {
            for (const nlohmann::json& concreteStaticMapping : staticMapping) {
                publishMappedMessage(concreteStaticMapping, publish, mappedPublishes);
            }
        }
    }

} // namespace mqtt::lib

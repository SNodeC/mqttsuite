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

#include <cmath>
#include <core/DynamicLoader.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Publish.h>

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
#include <functional>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

#endif

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

namespace mqtt::lib {

    MqttMapper::MqttMapper(const nlohmann::json& mappingJson)
        : mappingJson(mappingJson)
        , delayedQueue(this) {
        injaEnvironment = new inja::Environment;

        if (mappingJson.contains("plugins")) {
            VLOG(1) << "Loading plugins ...";

            for (const nlohmann::json& pluginJson : mappingJson["plugins"]) {
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
                }
            }

            VLOG(1) << "Loading plugins done";
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
                    VLOG(1) << "Topic mapping found for:";
                    VLOG(1) << "  Type: static";
                    VLOG(1) << "  Topic: " << publish.getTopic();
                    VLOG(1) << "  Message: " << publish.getMessage();
                    VLOG(1) << "  QoS: " << static_cast<uint16_t>(publish.getQoS());
                    VLOG(1) << "  Retain: " << publish.getRetain();

                    publishMappedMessages(subscription["static"], publish);
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

                    publishMappedTemplates(subscription["value"], json, publish);
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

                        publishMappedTemplates(subscription["json"], json, publish);
                    } catch (const nlohmann::json::parse_error& e) {
                        VLOG(1) << "  Parsing message into json failed: " << publish.getMessage();
                        VLOG(1) << "     What: " << e.what() << '\n'
                                << "     Exception Id: " << e.id << '\n'
                                << "     Byte position of error: " << e.byte;
                    }
                }
            }
        }
    }

    void MqttMapper::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain, double delay) {
        if (delay < 0.0) {
            publishMapping(topic, message, qoS, retain);
        } else {
            delayedQueue.delayPublish(delay, topic, message, qoS, retain);
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

                    publishMapping(renderedTopic, renderedMessage, qoS, retain, delay);
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

    void MqttMapper::publishMappedTemplates(const nlohmann::json& templateMapping,
                                            nlohmann::json& json,
                                            const iot::mqtt::packets::Publish& publish) {
        json["topic"] = publish.getTopic();
        json["qos"] = publish.getQoS();
        json["retain"] = publish.getRetain();
        json["package_identifier"] = publish.getPacketIdentifier();

        try {
            VLOG(1) << "  Render data: " << json.dump();

            if (templateMapping.is_object()) {
                publishMappedTemplate(templateMapping, json);
            } else {
                for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                    publishMappedTemplate(concreteTemplateMapping, json);
                }
            }
        } catch (const nlohmann::json::exception& e) {
            VLOG(1) << "JSON Exception during Render data:\n" << e.what();
        }
    }

    void MqttMapper::publishMappedMessage(const std::string& topic, const std::string& message, uint8_t qoS, bool retain, double delay) {
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

        publishMapping(topic, message, qoS, retain, delay);
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json& messageMapping = staticMapping["message_mapping"];

        VLOG(1) << "  Message mapping: " << messageMapping.dump();

        if (messageMapping.is_object()) {
            if (messageMapping["message"] == publish.getMessage()) {
                publishMappedMessage(staticMapping["mapped_topic"],
                                     messageMapping["mapped_message"],
                                     staticMapping["qos"],
                                     staticMapping["retain"],
                                     staticMapping["delay"]);
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
                                     staticMapping["delay"]);
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

    MqttMapper::DelayedQueue::DelayedQueue(MqttMapper* mqttMapper)
        : mqttMapper(mqttMapper) {
    }

    MqttMapper::DelayedQueue::~DelayedQueue() {
        delayTimer.cancel();
    }

    void MqttMapper::DelayedQueue::processDue() {
        const auto now = utils::Timeval::currentTime();

        while (!empty() && top().when <= now) {
            const auto& sheduledPublish = top();
            VLOG(1) << "Publish delayed message";
            VLOG(1) << "  Topic: " << sheduledPublish.topic;
            VLOG(1) << "  Message: " << sheduledPublish.message;
            VLOG(1) << "  QoS: " << static_cast<int>(sheduledPublish.qoS);
            VLOG(1) << "  retain: " << sheduledPublish.retain;
            VLOG(1) << "  Delay: " << sheduledPublish.delay;

            mqttMapper->publishMapping(sheduledPublish.topic, sheduledPublish.message, sheduledPublish.qoS, sheduledPublish.retain);
            pop();
        }
    }

    void MqttMapper::DelayedQueue::armDelayTimer() {
        delayTimer.cancel();

        auto delay = top().when - utils::Timeval::currentTime();

        // clamp negative delay to 0 (if top().when is already due)
        if (delay < utils::Timeval{}) {
            delay = utils::Timeval{};
        }

        delayTimer = core::timer::Timer::singleshotTimer(
            [this]() {
                processDue();

                if (!empty()) {
                    armDelayTimer(); // re-arm for the next item
                }
            },
            delay);
    }

    void MqttMapper::DelayedQueue::delayPublish(
        const utils::Timeval& timeval, const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        minHeap.push(ScheduledPublish{
            utils::Timeval::currentTime() + timeval, timeval, nextSeq++, std::move(topic), std::move(message), qoS, retain});

        armDelayTimer();
    }

    bool MqttMapper::DelayedQueue::empty() const {
        return minHeap.empty();
    }

    std::size_t MqttMapper::DelayedQueue::size() const {
        return minHeap.size();
    }

    const MqttMapper::ScheduledPublish& MqttMapper::DelayedQueue::top() const {
        return minHeap.top();
    }

    void MqttMapper::DelayedQueue::pop() {
        minHeap.pop();
    }

    bool MqttMapper::EarlierFirst::operator()(const ScheduledPublish& a, const ScheduledPublish& b) const {
        if (a.when != b.when)
            return a.when > b.when; // "later" has lower priority
        return a.seq > b.seq;
    }

} // namespace mqtt::lib

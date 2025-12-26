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

#include "MqttModel.h"

#include "Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// #include <cctype>
// #include <core/socket/SocketAddress.h>

#include <core/socket/stream/SocketConnection.h>
#include <express/Response.h>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/server/broker/Broker.h>
#include <net/SocketAddress.h>
#include <nlohmann/json.hpp>
#include <web/http/server/SocketContext.h>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include <ctime>
#include <functional>
#include <iomanip>
#include <log/Logger.h>
#include <sstream>
#include <utility>

struct tm;

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    /*
        {
            "clientId": "sensor-01",
            "protocol": "MQTT",
            "since": "2025-12-25 10:30:00 UTC",
            "duration": "2 days, 03:45:12",
            "connectionName": "mqtt_connection_12345",
            "localAddress": "127.0.0.1:1883",
            "remoteAddress": "192.168.1.45:54321",
            "cleanSession": true,
            "connectFlags": 194,
            "username": "sensor_user",
            "usernameFlag": true,
            "password": "secret123",
            "passwordFlag": true,
            "keepAlive": 60,
            "protocolLevel": 4,
            "loopPrevention": true,
            "willMessage": "sensor-01 disconnected unexpectedly",
            "willTopic": "sensors/status/sensor-01",
            "willQoS": 1,
            "willFlag": true,
            "willRetain": true
        }
    */
    void to_json(nlohmann::json& j, const MqttModel::MqttModelEntry& mqttModelEntry) {
        const Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        j = {{"clientId", mqtt->getClientId()},
             {"since", mqttModelEntry.onlineSince()},
             {"duration", mqttModelEntry.onlineDuration()},
             {"connectionName", mqtt->getConnectionName()},
             {"cleanSession", mqtt->getCleanSession()},
             {"connectFlags", mqtt->getConnectFlags()},
             {"username", mqtt->getUsername()},
             {"usernameFlag", mqtt->getUsernameFlag()},
             {"password", mqtt->getPassword()},
             {"passwordFlag", mqtt->getPasswordFlag()},
             {"keepAlive", mqtt->getKeepAlive()},
             {"protocol", mqtt->getProtocol()},
             {"protocolLevel", mqtt->getLevel()},
             {"loopPrevention", !mqtt->getReflect()},
             {"willMessage", mqtt->getWillMessage()},
             {"willTopic", mqtt->getWillTopic()},
             {"willQoS", mqtt->getWillQoS()},
             {"willFlag", mqtt->getWillFlag()},
             {"willRetain", mqtt->getWillRetain()},
             {"localAddress", socketConnection->getLocalAddress().toString()},
             {"remoteAddress", socketConnection->getRemoteAddress().toString()}};
    }

    struct subscribe {
        const std::string& topic;
        const std::string& clientId;
        uint8_t qoS;
    };

    void to_json(nlohmann::json& j, const subscribe& subscribe) {
        j = {{"clientId", subscribe.clientId}, {"topic", subscribe.topic}, {"qos", subscribe.qoS}};
    }

    struct unsubscribe {
        const std::string& clientId;
        const std::string& topic;
    };

    void to_json(nlohmann::json& j, const unsubscribe& unsubscribe) {
        j = {{"clientId", unsubscribe.clientId}, {"topic", unsubscribe.topic}};
    }

    struct retaine {
        const std::string& topic;
        const std::string& message;
        uint8_t qoS;
    };

    void to_json(nlohmann::json& j, const retaine& retaine) {
        j = {{"topic", retaine.topic}, {"message", retaine.message}, {"qos", retaine.qoS}};
    }

    struct release {
        const std::string& topic;
    };

    void to_json(nlohmann::json& j, const release& release) {
        j = {{"topic", release.topic}};
    }

    MqttModel::MqttModelEntry::MqttModelEntry(Mqtt* mqtt)
        : mqtt(mqtt) {
    }

    MqttModel::MqttModelEntry::~MqttModelEntry() {
    }

    std::string MqttModel::MqttModelEntry::onlineSince() const {
        return mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineSince();
    }

    std::string MqttModel::MqttModelEntry::onlineDuration() const {
        return mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineDuration();
    }

    Mqtt* MqttModel::MqttModelEntry::getMqtt() const {
        return mqtt;
    }

    MqttModel::EventReceiver::EventReceiver(const std::shared_ptr<express::Response>& response)
        : response(response)
        , heartbeatTimer(core::timer::Timer::intervalTimer(
              [response] {
                  response->sendFragment(":keep-alive");
                  response->sendFragment();
              },
              39)) {
    }

    MqttModel::EventReceiver::~EventReceiver() {
        heartbeatTimer.cancel();
    }

    bool MqttModel::EventReceiver::operator==(const EventReceiver& other) {
        return response.lock() == other.response.lock();
    }

    MqttModel::MqttModel()
        : onlineSinceTimePoint(std::chrono::system_clock::now()) {
    }

    MqttModel& MqttModel::instance() {
        static MqttModel mqttModel;

        return mqttModel;
    }

    void MqttModel::addEventReceiver(const std::shared_ptr<express::Response>& response,
                                     [[maybe_unused]] const std::string& lastEventId,
                                     const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker) {
        auto& eventReceiver = eventReceiverList.emplace_back(response);

        response->getSocketContext()->onDisconnected([this, &eventReceiver]() {
            eventReceiverList.remove(eventReceiver);
        });

        /*
            {
                "title": "MQTTBroker",
                "creator": {
                    "name": "Volker Christian",
                    "url": "https://github.com/VolkerChristian/"
                },
                "broker": {
                    "name": "MQTTBroker",
                    "url": "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker"
                },
                "suite": {
                    "name": "MQTTSuite",
                    "url": "https://github.com/SNodeC/mqttsuite"
                },
                "snodec": {
                    "name": "SNode.C",
                    "url": "https://github.com/SNodeC/snode.c"
                },
                "since": "2025-12-25 10:30:00 UTC",
                "duration": "2 days, 03:45:12"
            }
        */
        sendJsonEvent(response,
                      {
                          {"title", "MQTTBroker"},
                          {"creator", {{"name", "Volker Christian"}, {"url", "https://github.com/VolkerChristian"}}},
                          {"broker", {{"name", "MQTTBroker"}, {"url", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker"}}},
                          {"suite", {{"name", "MQTTSuite"}, {"url", "https://github.com/SNodeC/mqttsuite"}}},
                          {"snodec", {{"name", "SNode.C"}, {"url", "https://github.com/SNodeC/snode.c"}}},
                          {"since", onlineSince()},
                          {"duration", onlineDuration()},
                      },
                      "ui-initialize",
                      std::to_string(id++));

        for (const auto& modelMapEntry : modelMap) {
            sendJsonEvent(response, modelMapEntry.second, "client-connected", std::to_string(id++));
        }

        for (const auto& [topic, clients] : broker->getSubscriptionTree()) {
            for (const auto& client : clients) {
                sendJsonEvent(response, subscribe{topic, client.first, client.second}, "client-subscribed", std::to_string(id++));
            }
        }

        for (const auto& [topic, retained] : broker->getRetainTree()) {
            sendJsonEvent(response, retaine(topic, retained.first, retained.second), "retained-message-set", std::to_string(id++));
        }
    }

    void MqttModel::connectClient(Mqtt* mqtt) {
        MqttModelEntry& mqttModelEntry = modelMap.emplace(mqtt->getClientId(), mqtt).first->second;

        sendJsonEvent(mqttModelEntry, "client-connected", std::to_string(id++));
    }

    void MqttModel::disconnectClient(const std::string& clientId) {
        if (modelMap.contains(clientId)) {
            sendJsonEvent(modelMap[clientId], "client-disconnected", std::to_string(id++));

            modelMap.erase(clientId);
        }
    }

    void MqttModel::subscribeClient(const std::string& clientId, const std::string& topic, const uint8_t qos) {
        sendJsonEvent(subscribe{topic, clientId, qos}, "client-subscribed", std::to_string(id++));
    }

    void MqttModel::unsubscribeClient(const std::string& clientId, const std::string& topic) {
        sendJsonEvent(unsubscribe{clientId, topic}, "client-unsubscribed", std::to_string(id++));
    }

    void MqttModel::publishMessage(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        if (retain) {
            if (!message.empty()) {
                sendJsonEvent(retaine{topic, message, qoS}, "retained-message-set", std::to_string(id++));
            } else {
                sendJsonEvent(release{topic}, "retained-message-deleted", std::to_string(id++));
            }
        }
    }

    const std::map<std::string, MqttModel::MqttModelEntry>& MqttModel::getClients() const {
        return modelMap;
    }

    Mqtt* MqttModel::getMqtt(const std::string& clientId) const {
        Mqtt* mqtt = nullptr;

        auto modelIt = modelMap.find(clientId);
        if (modelIt != modelMap.end()) {
            mqtt = modelIt->second.getMqtt();
        }

        return mqtt;
    }

    std::string MqttModel::onlineSince() const {
        return timePointToString(onlineSinceTimePoint);
    }

    std::string MqttModel::onlineDuration() const {
        return durationToString(onlineSinceTimePoint);
    }

    void MqttModel::sendEvent(const std::shared_ptr<express::Response>& response,
                              const std::string& data,
                              const std::string& event,
                              const std::string& id) {
        if (response->isConnected()) {
            if (!event.empty()) {
                response->sendFragment("event:" + event);
            }
            if (!id.empty()) {
                response->sendFragment("id:" + id);
            }
            response->sendFragment("data:" + data);
            response->sendFragment();
        }
    }

    void MqttModel::sendJsonEvent(const std::shared_ptr<express::Response>& response,
                                  const nlohmann::json& json,
                                  const std::string& event,
                                  const std::string& id) {
        sendEvent(response, json.dump(), event, id);
    }

    void MqttModel::sendEvent(const std::string& data, const std::string& event, const std::string& id) const {
        for (auto& eventReceiver : eventReceiverList) {
            if (const auto& response = eventReceiver.response.lock()) {
                sendEvent(response, data, event, id);
            }
        }
    }

    void MqttModel::sendJsonEvent(const nlohmann::json& json, const std::string& event, const std::string& id) const {
        VLOG(0) << "Server sent event: " << event << "\n" << json.dump(4);

        sendEvent(json.dump(), event, id);
    }

    std::string MqttModel::timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        std::tm* tm_ptr = std::gmtime(&time);

        char buffer[100];
        std::string onlineSince = "Formatting error";

        // Format: "2025-02-02 14:30:00"
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr)) {
            onlineSince = std::string(buffer) + " UTC";
        }

        return onlineSince;
    }

    std::string MqttModel::durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                                            const std::chrono::time_point<std::chrono::system_clock>& later) {
        using seconds_duration_type = std::chrono::duration<std::chrono::seconds::rep>::rep;

        seconds_duration_type totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(later - bevore).count();

        // Compute days, hours, minutes, and seconds
        seconds_duration_type days = totalSeconds / 86400; // 86400 seconds in a day
        seconds_duration_type remainder = totalSeconds % 86400;
        seconds_duration_type hours = remainder / 3600;
        remainder = remainder % 3600;
        seconds_duration_type minutes = remainder / 60;
        seconds_duration_type seconds = remainder % 60;

        // Format the components into a string using stringstream
        std::ostringstream oss;
        if (days > 0) {
            oss << days << " day" << (days == 1 ? "" : "s") << ", ";
        }
        oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
            << std::setfill('0') << seconds;

        return oss.str();
    }

} // namespace mqtt::mqttbroker::lib

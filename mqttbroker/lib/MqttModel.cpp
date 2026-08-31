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

#include "MqttModel.h"

#include "Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <express/Response.h>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/server/broker/Broker.h>
#include <net/SocketAddress.h>
#include <nlohmann/json.hpp>
#include <web/http/server/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "lib/SemanticLog.h"

#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

struct tm;

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    static void to_json(nlohmann::json& j, const Mqtt* mqtt) {
        j = {{"clientId", mqtt->getClientId()},
             {"connectionName", mqtt->getConnectionName()},
             {"cleanSession", mqtt->getCleanSession()},
             {"connectFlags", mqtt->getConnectFlags()},
             {"username", mqtt->getUsername()},
             {"usernameFlag", mqtt->getUsernameFlag()},
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
             {"since", mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineSince()},
             {"duration", mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineDuration()},
             {"localAddress", mqtt->getMqttContext()->getSocketConnection()->getLocalAddress().toString()},
             {"remoteAddress", mqtt->getMqttContext()->getSocketConnection()->getRemoteAddress().toString()}};
    }

    struct subscribe {
        const std::string& topic;
        const std::string& clientId;
        uint8_t qoS;
    };

    static void to_json(nlohmann::json& j, const subscribe& value) {
        j = {{"clientId", value.clientId}, {"topic", value.topic}, {"qos", value.qoS}};
    }

    struct unsubscribe {
        const std::string& clientId;
        const std::string& topic;
    };

    static void to_json(nlohmann::json& j, const unsubscribe& value) {
        j = {{"clientId", value.clientId}, {"topic", value.topic}};
    }

    struct retaine {
        const std::string& topic;
        const std::string& message;
        uint8_t qoS;
    };

    static void to_json(nlohmann::json& j, const retaine& value) {
        j = {{"topic", value.topic}, {"message", value.message}, {"qos", value.qoS}};
    }

    struct release {
        const std::string& topic;
    };

    static void to_json(nlohmann::json& j, const release& value) {
        j = {{"topic", value.topic}};
    }

    MqttModel::EventReceiver::EventReceiver(std::uint64_t id, const std::shared_ptr<express::Response>& response)
        : id(id)
        , response(response)
        , heartbeatTimer(core::timer::Timer::intervalTimer(
              [response] {
                  if (response->isConnected()) {
                      response->sendFragment(":keep-alive");
                      response->sendFragment();
                  }
              },
              39)) {
    }

    MqttModel::EventReceiver::~EventReceiver() {
        heartbeatTimer.cancel();
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
        const std::uint64_t eventReceiverId = nextEventReceiverId++;
        eventReceiverList.emplace_back(eventReceiverId, response);

        response->getSocketContext()->setOnDisconnected([this, eventReceiverId]() {
            eventReceiverList.remove_if([eventReceiverId](const EventReceiver& eventReceiver) {
                return eventReceiver.getId() == eventReceiverId;
            });
        });

        sendJsonEvent(response,
                      {{"title", "MQTTBroker"},
                       {"creator", {{"name", "Volker Christian"}, {"url", "https://github.com/VolkerChristian"}}},
                       {"broker", {{"name", "MQTTBroker"}, {"url", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker"}}},
                       {"suite", {{"name", "MQTTSuite"}, {"url", "https://github.com/SNodeC/mqttsuite"}}},
                       {"snodec", {{"name", "SNode.C"}, {"url", "https://github.com/SNodeC/snode.c"}}},
                       {"since", onlineSince()},
                       {"duration", onlineDuration()}},
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
            sendJsonEvent(response, retaine{topic, retained.first, retained.second}, "retained-message-set", std::to_string(id++));
        }
    }

    void MqttModel::connectClient(Mqtt* mqtt) {
        modelMap.emplace(mqtt->getClientId(), mqtt);
        sendJsonEvent(mqtt, "client-connected", std::to_string(id++));
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

    const std::map<std::string, Mqtt*>& MqttModel::getClients() const {
        return modelMap;
    }

    Mqtt* MqttModel::getMqtt(const std::string& clientId) const {
        Mqtt* mqtt = nullptr;
        auto modelIt = modelMap.find(clientId);
        if (modelIt != modelMap.end()) {
            mqtt = modelIt->second;
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
        mqttsuite::semantic::brokerLog().info() << "Server sent event: " << event;
        sendEvent(json.dump(), event, id);
    }

    std::string MqttModel::timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        std::tm* tmPtr = std::gmtime(&time);

        char buffer[100];
        std::string onlineSince = "Formatting error";
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmPtr)) {
            onlineSince = std::string(buffer) + " UTC";
        }
        return onlineSince;
    }

    std::string MqttModel::durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                                            const std::chrono::time_point<std::chrono::system_clock>& later) {
        using seconds_duration_type = std::chrono::duration<std::chrono::seconds::rep>::rep;

        seconds_duration_type totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(later - bevore).count();
        seconds_duration_type days = totalSeconds / 86400;
        seconds_duration_type remainder = totalSeconds % 86400;
        seconds_duration_type hours = remainder / 3600;
        remainder %= 3600;
        seconds_duration_type minutes = remainder / 60;
        seconds_duration_type seconds = remainder % 60;

        std::ostringstream oss;
        if (days > 0) {
            oss << days << " day" << (days == 1 ? "" : "s") << ", ";
        }
        oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
            << std::setfill('0') << seconds;
        return oss.str();
    }

} // namespace mqtt::mqttbroker::lib

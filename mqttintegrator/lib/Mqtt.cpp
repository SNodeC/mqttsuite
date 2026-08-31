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

#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

#include <cstring>
#include <list>
#include <SemanticLog.h>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

#endif

namespace mqtt::mqttintegrator::lib {

    Mqtt::Mqtt(const nlohmann::json& connectionJson, const nlohmann::json& mappingJson)
        : iot::mqtt::client::Mqtt(connectionJson["client_id"])
        , mqtt::lib::MqttMapper(mappingJson)
        , connectionJson(connectionJson)
        , keepAlive(connectionJson["keep_alive"])
        , cleanSession(connectionJson["clean_session"])
        , willTopic(connectionJson["will_topic"])
        , willMessage(connectionJson["will_message"])
        , willQoS(connectionJson["will_qos"])
        , willRetain(connectionJson["will_retain"])
        , username(connectionJson["username"])
        , password(connectionJson["password"]) {
        snode::semantic::appLog().trace() << "Keep Alive: " << keepAlive;
        snode::semantic::appLog().trace() << "Client Id: " << clientId;
        snode::semantic::appLog().trace() << "Clean Session: " << cleanSession;
        snode::semantic::appLog().trace() << "Will Topic: " << willTopic;
        snode::semantic::appLog().trace() << "Will Message: " << willMessage;
        snode::semantic::appLog().trace() << "Will QoS: " << static_cast<uint16_t>(willQoS);
        snode::semantic::appLog().trace() << "Will Retain " << willRetain;
        snode::semantic::appLog().trace() << "Username: " << username;
        snode::semantic::appLog().trace() << "Password: " << password;
    }

    void Mqtt::onConnected() {
        snode::semantic::appLog().trace() << "MQTT: Initiating Session";

        sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    void Mqtt::onExit(int signum) {
        snode::semantic::appLog().trace() << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendPublish("snode.c/_cfg_/connection", connectionJson.dump(), 0, true);

            std::list<iot::mqtt::Topic> topicList = MqttMapper::extractSubscriptions();

            for (const iot::mqtt::Topic& topic : topicList) {
                snode::semantic::appLog().trace() << "MQTT: Subscribe Topic: " << topic.getName() << ", qoS: " << static_cast<uint16_t>(topic.getQoS());
            }

            sendSubscribe(topicList);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void Mqtt::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        sendPublish(topic, message, qoS, retain);
    }

} // namespace mqtt::mqttintegrator::lib

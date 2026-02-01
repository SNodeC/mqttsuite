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

#include "Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttintegrator::lib {

    std::set<Mqtt*> Mqtt::instances;

    Mqtt::Mqtt(const std::string& connectionName,
               const nlohmann::json& connectionJson,
               const nlohmann::json& mappingJson,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, //
                                  connectionJson["client_id"],
                                  connectionJson["keep_alive"],
                                  sessionStoreFileName)
        , mqtt::lib::MqttMapper(mappingJson)
        , connectionJson(connectionJson) {
        instances.insert(this);
        
        VLOG(1) << "  Will QoS: " << static_cast<uint16_t>(connectionJson["will_qos"]);
        VLOG(1) << "  Will Retain " << connectionJson["will_retain"];
        VLOG(1) << "  Username: " << connectionJson["username"];
        VLOG(1) << "  Password: " << connectionJson["password"];
    }

    Mqtt::~Mqtt() {
        instances.erase(this);
    }

    void Mqtt::reloadAll() {
        for (auto* instance : instances) {
            if (instance->getMqttContext() && instance->getMqttContext()->getSocketConnection()) {
                instance->getMqttContext()->getSocketConnection()->close();
            }
        }
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(connectionJson["clean_session"],
                    connectionJson["will_topic"],
                    connectionJson["will_message"],
                    connectionJson["will_qos"],
                    connectionJson["will_retain"],
                    connectionJson["username"],
                    connectionJson["password"]);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();

        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendPublish("snode.c/_cfg_/connection", connectionJson.dump(), 0, true);

            const std::list<iot::mqtt::Topic> topicList = MqttMapper::extractSubscriptions();

            for (const iot::mqtt::Topic& topic : topicList) {
                VLOG(1) << "MQTT: Subscribe Topic: " << topic.getName() << ", qoS: " << static_cast<uint16_t>(topic.getQoS());
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

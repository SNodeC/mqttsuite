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

#include "mqttbroker/lib/MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Subscribe.h>
#include <iot/mqtt/packets/Unsubscribe.h>
#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <log/Logger.h>

#endif

namespace mqtt::mqttbroker::lib {

    Mqtt::Mqtt(const std::string& connectionName,
               const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
               const nlohmann::json& mappingJson)
        : iot::mqtt::server::Mqtt(connectionName, broker)
        , mqtt::lib::MqttMapper(mappingJson) {
    }

    void Mqtt::subscribe(const std::string& topic, uint8_t qoS) {
        broker->subscribe(clientId, topic, qoS);

        onSubscribe(iot::mqtt::packets::Subscribe(0, {{topic, qoS}}));
    }

    void Mqtt::unsubscribe(const std::string& topic) {
        broker->unsubscribe(clientId, topic);

        onUnsubscribe(iot::mqtt::packets::Unsubscribe(0, {{topic, 0}}));
    }

    void Mqtt::onConnect([[maybe_unused]] const iot::mqtt::packets::Connect& connect) {
        VLOG(1) << "MQTT: Connected";

        MqttModel::instance().connectClient(this);
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        VLOG(1) << "MQTT: Publish";

        MqttModel::instance().publishMessage(publish.getTopic(), publish.getMessage(), publish.getQoS(), publish.getRetain());

        publishMappings(publish);
    }

    void Mqtt::onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) {
        VLOG(1) << "MQTT: Subscribe";

        for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
            MqttModel::instance().subscribeClient(clientId, topic.getName(), topic.getQoS());
        }
    }

    void Mqtt::onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) {
        VLOG(1) << "MQTT: Unsubscribe";

        for (const std::string& topic : unsubscribe.getTopics()) {
            MqttModel::instance().unsubscribeClient(clientId, topic);
        }
    }

    void Mqtt::onDisconnected() {
        MqttModel::instance().disconnectClient(clientId);

        VLOG(1) << "MQTT: Disconnected";
    }

    void Mqtt::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        broker->publish(clientId, topic, message, qoS, retain);

        publishMappings(iot::mqtt::packets::Publish(getPacketIdentifier(), topic, message, qoS, false, retain));
    }

} // namespace mqtt::mqttbroker::lib

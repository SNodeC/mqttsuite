/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "mqttbroker/lib/MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>

#endif

namespace mqtt::mqttbroker::lib {

    Mqtt::Mqtt(const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker, const nlohmann::json& mappingJson)
        : iot::mqtt::server::Mqtt(broker)
        , mqtt::lib::MqttMapper(mappingJson) {
    }

    void Mqtt::onConnect(const iot::mqtt::packets::Connect& connect) {
        VLOG(1) << "MQTT: Connected";
        MqttModel::instance().addConnectedClient(this, connect);
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void Mqtt::onDisconnected() {
        MqttModel::instance().delDisconnectedClient(this);
        VLOG(1) << "MQTT: Disconnected";
    }

    void Mqtt::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        broker->publish(clientId, topic, message, qoS, retain);

        publishMappings(iot::mqtt::packets::Publish(getPacketIdentifier(), topic, message, qoS, false, retain));
    }

} // namespace mqtt::mqttbroker::lib

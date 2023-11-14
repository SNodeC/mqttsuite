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

#include "Bridge.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Mqtt.h>
#include <iot/mqtt/packets/Publish.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    Bridge::Bridge(const nlohmann::json& connectionJson)
        : connectionJson(connectionJson) {
    }

    nlohmann::json& Bridge::getConnectionJson() {
        return connectionJson;
    }

    void Bridge::addMqtt(iot::mqtt::Mqtt* mqtt) {
        mqttList.push_back(mqtt);
    }

    void Bridge::removeMqtt(iot::mqtt::Mqtt* mqtt) {
        mqttList.remove(mqtt);
    }

    void Bridge::publish(iot::mqtt::Mqtt* originMqtt, const iot::mqtt::packets::Publish& publish) {
        for (iot::mqtt::Mqtt* destinationMqtt : mqttList) {
            if (originMqtt != destinationMqtt) { // Do not reflect message to origin broker
                destinationMqtt->sendPublish(publish.getTopic(), publish.getMessage(), publish.getQoS(), publish.getRetain());
            }
        }
    }

} // namespace mqtt::bridge::lib
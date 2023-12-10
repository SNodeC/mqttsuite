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

#ifndef MQTTBROKER_LIB_MQTT_H
#define MQTTBROKER_LIB_MQTT_H

#include "lib/MqttMapper.h"

namespace iot::mqtt {
    namespace packets {
        class Connect;
        class Publish;
    } // namespace packets
    namespace server::broker {
        class Broker;
    } // namespace server::broker
} // namespace iot::mqtt

#include <iot/mqtt/server/Mqtt.h> // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <string>

#endif

namespace mqtt::mqttbroker::lib {

    class Mqtt
        : public iot::mqtt::server::Mqtt
        , public mqtt::lib::MqttMapper {
    public:
        explicit Mqtt(const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker, const nlohmann::json& mappingJson);

    private:
        // inherited from iot::mqtt::server::SocketContext - the plain and base MQTT broker
        void onConnect(const iot::mqtt::packets::Connect& connect) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;

        // inherited from core::socket::SocketContext (the root class of all SocketContext classes) via iot::mqtt::server::SocketContext
        void onDisconnected() final;

        // implement poor virtual method from apps::mqtt::lib::MqttMapper
        void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) final;
    };

} // namespace mqtt::mqttbroker::lib

#endif // MQTTBROKER_LIB_MQTT_H

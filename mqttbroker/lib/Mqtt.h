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

#ifndef MQTTBROKER_LIB_MQTT_H
#define MQTTBROKER_LIB_MQTT_H

#include "lib/MqttMapper.h"

namespace iot::mqtt::server::broker {
    class Broker;
} // namespace iot::mqtt::server::broker

#include <iot/mqtt/server/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    class Mqtt
        : public iot::mqtt::server::Mqtt
        , public mqtt::lib::MqttMapper {
    public:
        explicit Mqtt(const std::string& connectionName,
                      const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                      const nlohmann::json& mappingJson);

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

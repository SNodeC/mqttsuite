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

#ifndef APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H

#include "lib/MqttMapper.h"

#include <iot/mqtt/client/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif

namespace mqtt::mqttintegrator::lib {

    class Mqtt
        : public iot::mqtt::client::Mqtt
        , public mqtt::lib::MqttMapper {
    public:
        explicit Mqtt(const nlohmann::json& connectionJson, const nlohmann::json& mappingJson);

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;

        void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) final;

        const nlohmann::json& connectionJson;

        uint16_t keepAlive;
        bool cleanSession;
        std::string willTopic;
        std::string willMessage;
        uint8_t willQoS;
        bool willRetain;
        std::string username;
        std::string password;
    };

} // namespace mqtt::mqttintegrator::lib

#endif // APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H

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

#ifndef APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXT_H

namespace iot::mqtt::packets {
    class Connack;
    class Publish;
} // namespace iot::mqtt::packets

namespace mqtt::bridge::lib {
    class Bridge;
}

#include <iot/mqtt/client/Mqtt.h> // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Topic.h>
#include <list>
#include <string>

#endif

namespace mqtt::bridge::lib {

    class Mqtt : public iot::mqtt::client::Mqtt {
    public:
        explicit Mqtt(Bridge& bridge, const std::list<iot::mqtt::Topic>& topics);

    private:
        void onConnected() final;
        void onDisconnected() final;
        void onExit(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;

        mqtt::bridge::lib::Bridge& bridge;
        std::list<iot::mqtt::Topic> topics;

        uint16_t keepAlive;
        bool cleanSession;
        std::string willTopic;
        std::string willMessage;
        uint8_t willQoS;
        bool willRetain;
        std::string username;
        std::string password;
    };

} // namespace mqtt::bridge::lib

#endif // APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXT_H

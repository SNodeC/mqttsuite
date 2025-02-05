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

#ifndef IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H
#define IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {
    class Mqtt;

    namespace packets {
        class Publish;
    }
} // namespace iot::mqtt

#include <list>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class Bridge {
    public:
        explicit Bridge(const std::string& name);

        const std::string& getName();

        void addMqtt(iot::mqtt::Mqtt* mqtt);
        void removeMqtt(iot::mqtt::Mqtt* mqtt);

        void publish(const iot::mqtt::Mqtt* originMqtt, const iot::mqtt::packets::Publish& publish);

        const std::list<iot::mqtt::Mqtt*>& getMqttList() const;

    private:
        std::string name;

        std::list<iot::mqtt::Mqtt*> mqttList;
    };

} // namespace mqtt::bridge::lib

#endif // IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H

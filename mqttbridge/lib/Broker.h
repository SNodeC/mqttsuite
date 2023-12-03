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

#ifndef MQTT_BRIDGE_LIB_BROKER_H
#define MQTT_BRIDGE_LIB_BROKER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Topic.h>
#include <list>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class Broker {
    public:
        Broker() = default;

        Broker(const std::string& instanceName,
               const std::string& protocol,
               const std::string& encryption,
               const std::string& transport,
               std::list<iot::mqtt::Topic>& topics);

        Broker(const Broker&) = delete;

        ~Broker();

        const std::string& getInstanceName() const;
        const std::string& getProtocol() const;
        const std::string& getEncryption() const;
        const std::string& getTransport() const;
        const std::list<iot::mqtt::Topic>& getTopics() const;

    private:
        std::string instanceName;
        std::string protocol;
        std::string encryption;
        std::string transport;
        std::list<iot::mqtt::Topic> topics;
    };

} // namespace mqtt::bridge::lib

#endif // MQTT_BRIDGE_LIB_BROKER_H

/*
             const std::string& name = brokerJsonConfig["name"];
             const std::string& protocol = brokerJsonConfig["protocol"];
             const std::string& encryption = brokerJsonConfig["encryption"];
             const std::string& transport = brokerJsonConfig["transport"];

             std::list<iot::mqtt::Topic> topics;
             topics.emplace_back(topicJson["topic"], topicJson["qos"]);

                 VLOG(1) << "    Topic: " << topicJson["topic"];
                 VLOG(1) << "      Qos: " << topicJson["qos"];
 */

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

namespace mqtt::bridge::lib {
    class Bridge;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Topic.h>
#include <list>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class Broker {
    public:
        Broker(Bridge& bridge,
               std::string&& clientId,
               uint16_t keepAlive,
               bool cleanSession,
               std::string&& willTopic,
               std::string&& willMessage,
               uint8_t willQoS,
               bool willRetain,
               std::string&& username,
               std::string&& password,
               bool loopPrevention,
               std::string&& instanceName,
               std::string&& protocol,
               std::string&& encryption,
               std::string&& transport,
               std::list<iot::mqtt::Topic>&& topics);

        Broker(const Broker&) = delete;

        Broker(Broker&&) = default;

        Bridge& getBridge() const;

        const std::string& getClientId() const;
        uint16_t getKeepAlive() const;
        bool getCleanSession() const;
        const std::string& getWillTopic() const;
        const std::string& getWillMessage() const;
        uint8_t getWillQoS() const;
        bool getWillRetain() const;
        const std::string& getUsername() const;
        const std::string& getPassword() const;
        bool getLoopPrevention() const;

        const std::string& getInstanceName() const;
        const std::string& getProtocol() const;
        const std::string& getEncryption() const;
        const std::string& getTransport() const;
        const std::list<iot::mqtt::Topic>& getTopics() const;

    private:
        Bridge& bridge;

        std::string clientId;
        uint16_t keepAlive;
        bool cleanSession;
        std::string willTopic;
        std::string willMessage;
        uint8_t willQoS;
        bool willRetain;
        std::string username;
        std::string password;
        bool loopPrevention;

        std::string instanceName;
        std::string protocol;
        std::string encryption;
        std::string transport;
        std::list<iot::mqtt::Topic> topics;
    };

} // namespace mqtt::bridge::lib

#endif // MQTT_BRIDGE_LIB_BROKER_H

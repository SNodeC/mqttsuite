/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Fre
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

#ifndef MQTT_BRIDGE_LIB_BROKER_H
#define MQTT_BRIDGE_LIB_BROKER_H

namespace mqtt::bridge::lib {
    class Bridge;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <iot/mqtt/Topic.h> // IWYU pragma: export
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

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

#include "Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    Broker::Broker(Bridge& bridge,
                   const std::string& sessionStoreFileName,
                   std::string&& clientId,
                   uint16_t keepAlive,
                   bool cleanSession,
                   std::string&& willTopic,
                   std::string&& willMessage,
                   uint8_t willQoS,
                   bool willRetain,
                   std::string&& userName,
                   std::string&& passWord,
                   bool loopPrevention,
                   std::string&& instanceName,
                   std::string&& protocol,
                   std::string&& encryption,
                   std::string&& transport,
                   const nlohmann::json& address,
                   std::list<iot::mqtt::Topic>&& topics)
        : bridge(bridge)
        , sessionStoreFileName(sessionStoreFileName)
        , clientId(clientId)
        , keepAlive(keepAlive)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(userName)
        , password(passWord)
        , loopPrevention(loopPrevention)
        , instanceName(std::move(instanceName))
        , protocol(std::move(protocol))
        , encryption(std::move(encryption))
        , transport(std::move(transport))
        , address(address)
        , topics(std::move(topics)) {
    }

    Bridge& Broker::getBridge() const {
        return bridge;
    }

    const std::string& Broker::getSessionStoreFileName() const {
        return sessionStoreFileName;
    }

    const std::string& Broker::getClientId() const {
        return clientId;
    }

    uint16_t Broker::getKeepAlive() const {
        return keepAlive;
    }

    bool Broker::getCleanSession() const {
        return cleanSession;
    }

    const std::string& Broker::getWillTopic() const {
        return willTopic;
    }

    const std::string& Broker::getWillMessage() const {
        return willMessage;
    }

    uint8_t Broker::getWillQoS() const {
        return willQoS;
    }

    bool Broker::getWillRetain() const {
        return willRetain;
    }

    const std::string& Broker::getUsername() const {
        return username;
    }

    const std::string& Broker::getPassword() const {
        return password;
    }

    bool Broker::getLoopPrevention() const {
        return loopPrevention;
    }

    const std::string& Broker::getInstanceName() const {
        return instanceName;
    }

    const std::string& Broker::getProtocol() const {
        return protocol;
    }

    const std::string& Broker::getEncryption() const {
        return encryption;
    }

    const std::string& Broker::getTransport() const {
        return transport;
    }

    const std::list<iot::mqtt::Topic>& Broker::getTopics() const {
        return topics;
    }

    const nlohmann::json& Broker::getAddress() const {
        return address;
    }

} // namespace mqtt::bridge::lib

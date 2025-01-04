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

#include "Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    Broker::Broker(Bridge& bridge,
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
                   std::list<iot::mqtt::Topic>&& topics)
        : bridge(bridge)
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
        , topics(std::move(topics)) {
    }

    Bridge& Broker::getBridge() const {
        return bridge;
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

} // namespace mqtt::bridge::lib

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

#include "Mqtt.h"

#include "Bridge.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <list>
#include <log/Logger.h>
#include <utils/system/signal.h>

#endif

namespace mqtt::bridge::lib {

    Mqtt::Mqtt(Bridge* bridge, const std::list<iot::mqtt::Topic>& topics)
        : iot::mqtt::client::Mqtt(bridge->getClientId())
        , bridge(bridge)
        , topics(topics)
        , keepAlive(bridge->getKeepAlive())
        , cleanSession(bridge->getCleanSession())
        , willTopic(bridge->getWillTopic())
        , willMessage(bridge->getWillMessage())
        , willQoS(bridge->getWillQoS())
        , willRetain(bridge->getWillRetain())
        , username(bridge->getUsername())
        , password(bridge->getPassword()) {
        LOG(TRACE) << "Keep Alive: " << keepAlive;
        LOG(TRACE) << "Client Id: " << clientId;
        LOG(TRACE) << "Clean Session: " << cleanSession;
        LOG(TRACE) << "Will Topic: " << willTopic;
        LOG(TRACE) << "Will Message: " << willMessage;
        LOG(TRACE) << "Will QoS: " << static_cast<uint16_t>(willQoS);
        LOG(TRACE) << "Will Retain " << willRetain;
        LOG(TRACE) << "Username: " << username;
        LOG(TRACE) << "Password: " << password;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password, false);
    }

    void Mqtt::onDisconnected() {
        bridge->removeMqtt(this);
        VLOG(1) << "MQTT: Disconnected";
    }

    void Mqtt::onExit(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0) {
            bridge->addMqtt(this);

            sendSubscribe(topics);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        bridge->publish(this, publish);
    }

} // namespace mqtt::bridge::lib

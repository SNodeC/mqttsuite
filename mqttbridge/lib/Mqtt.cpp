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
#include "Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <cstring>
#include <iot/mqtt/packets/Connack.h>
#include <log/Logger.h>
#include <utils/system/signal.h>

#endif

namespace mqtt::bridge::lib {

    Mqtt::Mqtt(const Broker& broker)
        : iot::mqtt::client::Mqtt(broker.getClientId())
        , broker(broker) {
        LOG(TRACE) << "Keep Alive: " << broker.getKeepAlive();
        LOG(TRACE) << "Client Id: " << broker.getClientId();
        LOG(TRACE) << "Clean Session: " << broker.getCleanSession();
        LOG(TRACE) << "Will Topic: " << broker.getWillTopic();
        LOG(TRACE) << "Will Message: " << broker.getWillMessage();
        LOG(TRACE) << "Will QoS: " << static_cast<uint16_t>(broker.getWillQoS());
        LOG(TRACE) << "Will Retain " << broker.getWillRetain();
        LOG(TRACE) << "Username: " << broker.getUsername();
        LOG(TRACE) << "Password: " << broker.getPassword();
        LOG(TRACE) << "Loop Prevention: " << broker.getLoopPrevention();
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(broker.getKeepAlive(),
                    broker.getClientId(),
                    broker.getCleanSession(),
                    broker.getWillTopic(),
                    broker.getWillMessage(),
                    broker.getWillQoS(),
                    broker.getWillRetain(),
                    broker.getUsername(),
                    broker.getPassword(),
                    broker.getLoopPrevention());
    }

    void Mqtt::onDisconnected() {
        broker.getBridge().removeMqtt(this);
        VLOG(1) << "MQTT: Disconnected";
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();

        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0) {
            broker.getBridge().addMqtt(this);

            sendSubscribe(broker.getTopics());
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        broker.getBridge().publish(this, publish);
    }

} // namespace mqtt::bridge::lib

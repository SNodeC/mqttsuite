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

#include "Mqtt.h"

#include "lib/BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/socket/stream/SocketConnection.h>
#include <cstdint>
#include <cstring>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/packets/Connack.h>
#include <log/Logger.h>
#include <utils/system/signal.h>

#endif

namespace mqtt::bridge::lib {

    Mqtt::Mqtt(const std::string& connectionName, const Broker& broker)
        : iot::mqtt::client::Mqtt(connectionName, //
                                  broker.getClientId(),
                                  broker.getKeepAlive(),
                                  broker.getSessionStoreFileName())
        , broker(broker) {
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "  Keep Alive: " << keepAlive;
        VLOG(1) << "  Prefix: " << broker.getPrefix();
        VLOG(1) << "  Clean Session: " << broker.getCleanSession();
        VLOG(1) << "  Will Topic: " << broker.getWillTopic();
        VLOG(1) << "  Will Message: " << broker.getWillMessage();
        VLOG(1) << "  Will QoS: " << static_cast<uint16_t>(broker.getWillQoS());
        VLOG(1) << "  Will Retain " << broker.getWillRetain();
        VLOG(1) << "  Username: " << broker.getUsername();
        VLOG(1) << "  Password: " << broker.getPassword();
        VLOG(1) << "  Loop Prevention: " << broker.getLoopPrevention();
    }

    const Broker& Mqtt::getBroker() const {
        return broker;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(broker.getCleanSession(),
                    broker.getWillTopic(),
                    broker.getWillMessage(),
                    broker.getWillQoS(),
                    broker.getWillRetain(),
                    broker.getUsername(),
                    broker.getPassword(),
                    broker.getLoopPrevention());
    }

    void Mqtt::onDisconnected() {
        const Broker* broker =
            mqtt::bridge::lib::BridgeStore::instance().getBroker(getMqttContext()->getSocketConnection()->getInstanceName());

        if (broker != nullptr) {
            broker->getBridge().removeMqtt(this);
        }

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
        const Broker* broker =
            mqtt::bridge::lib::BridgeStore::instance().getBroker(getMqttContext()->getSocketConnection()->getInstanceName());

        if (broker != nullptr) {
            broker->getBridge().publish(this, publish);
        }
    }

} // namespace mqtt::bridge::lib

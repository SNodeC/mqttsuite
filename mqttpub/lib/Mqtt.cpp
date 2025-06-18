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

#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <log/Logger.h>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttpub::lib {

    Mqtt::Mqtt(const std::string& connectionName,
               const std::string& clientId,
               const std::string& topic,
               const std::string& message,
               uint8_t qoS,
               bool retain,
               uint16_t keepAlive,
               bool cleanSession,
               const std::string& willTopic,
               const std::string& willMessage,
               uint8_t willQoS,
               bool willRetain,
               const std::string& username,
               const std::string& password,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, sessionStoreFileName)
        , topic(topic)
        , message(message)
        , qoS(qoS)
        , retain(retain)
        , keepAlive(keepAlive)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password) {
        VLOG(1) << "Keep Alive: " << keepAlive;
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "Clean Session: " << cleanSession;
        VLOG(1) << "Will Topic: " << willTopic;
        VLOG(1) << "Will Message: " << willMessage;
        VLOG(1) << "Will QoS: " << static_cast<uint16_t>(willQoS);
        VLOG(1) << "Will Retain " << willRetain;
        VLOG(1) << "Username: " << username;
        VLOG(1) << "Password: " << password;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();

        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendPublish(topic, message, qoS, retain);

            if (qoS == 0) {
                sendDisconnect();
            }
        } else {
            sendDisconnect();
        }
    }

    void Mqtt::onPuback([[maybe_unused]] const iot::mqtt::packets::Puback& puback) {
        if (qoS == 1) {
            sendDisconnect();
        }
    }

    void Mqtt::onPubcomp([[maybe_unused]] const iot::mqtt::packets::Pubcomp& pubcomp) {
        if (qoS == 2) {
            sendDisconnect();
        }
    }

} // namespace mqtt::mqttpub::lib

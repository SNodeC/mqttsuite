/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
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

#include "SubProtocolFactory.h"

#include "lib/BridgeStore.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <log/Logger.h>

#endif

namespace mqtt::mqttbridge::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        const mqtt::bridge::lib::Broker* broker =
            mqtt::bridge::lib::BridgeStore::instance().getBroker(subProtocolContext->getSocketConnection()->getInstanceName());

        if (broker != nullptr) {
            VLOG(1) << "  Creating Broker instance '" << broker->getName() << "' of Bridge '" << broker->getBridge().getName() << "'";
            VLOG(1) << "    Bridge client id : " << broker->getClientId();
            VLOG(1) << "    Transport: " << broker->getTransport();
            VLOG(1) << "    Protocol: " << broker->getProtocol();
            VLOG(1) << "    Encryption: " << broker->getEncryption();

            VLOG(1) << "    Topics:";
            const std::list<iot::mqtt::Topic>& topics = broker->getTopics();
            for (const iot::mqtt::Topic& topic : topics) {
                VLOG(1) << "      " << static_cast<uint16_t>(topic.getQoS()) << ":" << topic.getName();
            }

            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new mqtt::bridge::lib::Mqtt(subProtocolContext->getSocketConnection()->getConnectionName(), *broker));
        }

        return subProtocol;
    }

} // namespace mqtt::mqttbridge::websocket

extern "C" web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* mqttClientSubProtocolFactory() {
    return new mqtt::mqttbridge::websocket::SubProtocolFactory();
}

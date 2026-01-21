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

#include "SocketContextFactory.h"

#include "lib/Bridge.h"
#include "lib/BridgeStore.h"
#include "lib/Broker.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <log/Logger.h>

#endif

namespace mqtt::bridge {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        iot::mqtt::SocketContext* socketContext = nullptr;

        const mqtt::bridge::lib::Broker* broker = mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName());

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

            socketContext =
                new iot::mqtt::SocketContext(socketConnection, new mqtt::bridge::lib::Mqtt(socketConnection->getConnectionName(), *broker));
        }

        return socketContext;
    }

} // namespace mqtt::bridge

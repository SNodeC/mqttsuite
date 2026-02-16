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

#include "SocketContextFactory.h"

#include "lib/ConfigSections.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>
#include <log/Logger.h>
#include <net/config/ConfigInstanceAPI.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <string>

#endif

namespace mqtt::mqttcli {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        const lib::ConfigSession* configSession = socketConnection->getConfigInstance()->getSection<lib::ConfigSession>(true, true);
        const lib::ConfigSubscribe* configSubscribe = socketConnection->getConfigInstance()->getSection<lib::ConfigSubscribe>(true, true);
        const lib::ConfigPublish* configPublish = socketConnection->getConfigInstance()->getSection<lib::ConfigPublish>(true, true);

        configSubscribe = (configSubscribe != nullptr && configSubscribe->getOption("--topic")->count() > 0) ? configSubscribe : nullptr;
        configPublish = (configPublish != nullptr && configPublish->getOption("--topic")->count() > 0 &&
                         configPublish->getOption("--message")->count() > 0)
                            ? configPublish
                            : nullptr;

        core::socket::stream::SocketContext* socketContext = nullptr;

        if (configSubscribe != nullptr || configPublish != nullptr) {
            socketContext = new iot::mqtt::SocketContext(
                socketConnection,
                new ::mqtt::mqttcli::lib::Mqtt(socketConnection->getConnectionName(),
                                               configSession != nullptr ? configSession->getClientId() : "",
                                               configSession != nullptr ? configSession->getQoS() : 0,
                                               configSession != nullptr ? configSession->getKeepAlive() : 60,
                                               configSession != nullptr ? !configSession->getRetainSession() : true,
                                               configSession != nullptr ? configSession->getWillTopic() : "",
                                               configSession != nullptr ? configSession->getWillMessage() : "",
                                               configSession != nullptr ? configSession->getWillQoS() : 0,
                                               configSession != nullptr ? configSession->getWillRetain() : false,
                                               configSession != nullptr ? configSession->getUsername() : "",
                                               configSession != nullptr ? configSession->getPassword() : "",
                                               configSubscribe != nullptr ? configSubscribe->getTopic() : std::list<std::string>(),
                                               configPublish != nullptr ? configPublish->getTopic() : "",
                                               configPublish != nullptr ? configPublish->getMessage() : "",
                                               configPublish != nullptr ? configPublish->getRetain() : false));
        } else {
            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] " << socketConnection->getConnectionName()
                    << ": one of 'sub' or 'pub' is required";
        }

        return socketContext;
    }

} // namespace mqtt::mqttcli

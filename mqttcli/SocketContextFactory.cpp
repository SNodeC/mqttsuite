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

#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>
#include <log/Logger.h>
#include <net/config/ConfigInstance.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <utils/CLI11.hpp>

#endif

namespace mqtt::mqtt {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        const CLI::App* sessionApp = socketConnection->getConfig()->getSection("session", true, true);
        const CLI::App* subApp = socketConnection->getConfig()->getSection("sub", true, true);
        const CLI::App* pubApp = socketConnection->getConfig()->getSection("pub", true, true);

        subApp = (subApp != nullptr && (*subApp)["--topic"]->count() > 0) ? subApp : nullptr;
        pubApp = (pubApp != nullptr && (*pubApp)["--topic"]->count() > 0) ? pubApp : nullptr;

        core::socket::stream::SocketContext* socketContext = nullptr;

        if (subApp != nullptr || pubApp != nullptr) {
            socketContext = new iot::mqtt::SocketContext(
                socketConnection,
                new ::mqtt::mqtt::lib::Mqtt(socketConnection->getConnectionName(),
                                            sessionApp != nullptr ? sessionApp->get_option("--client-id")->as<std::string>() : "",
                                            sessionApp != nullptr ? sessionApp->get_option("--qos")->as<uint8_t>() : 0,
                                            sessionApp != nullptr ? sessionApp->get_option("--keep-alive")->as<uint16_t>() : 60,
                                            sessionApp != nullptr ? !sessionApp->get_option("--retain-session")->as<bool>() : true,
                                            sessionApp != nullptr ? sessionApp->get_option("--will-topic")->as<std::string>() : "",
                                            sessionApp != nullptr ? sessionApp->get_option("--will-message")->as<std::string>() : "",
                                            sessionApp != nullptr ? sessionApp->get_option("--will-qos")->as<uint8_t>() : 0,
                                            sessionApp != nullptr ? sessionApp->get_option("--will-retain")->as<bool>() : false,
                                            sessionApp != nullptr ? sessionApp->get_option("--username")->as<std::string>() : "",
                                            sessionApp != nullptr ? sessionApp->get_option("--password")->as<std::string>() : "",
                                            subApp != nullptr ? subApp->get_option("--topic")->as<std::list<std::string>>()
                                                              : std::list<std::string>(),
                                            pubApp != nullptr ? pubApp->get_option("--topic")->as<std::string>() : "",
                                            pubApp != nullptr ? pubApp->get_option("--message")->as<std::string>() : "",
                                            pubApp != nullptr ? pubApp->get_option("--retain")->as<bool>() : false));
        } else {
            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] " << socketConnection->getConnectionName()
                    << ": one of 'sub' or 'pub' is required";
        }

        return socketContext;
    }

} // namespace mqtt::mqtt

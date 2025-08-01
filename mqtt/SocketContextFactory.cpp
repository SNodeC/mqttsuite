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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>
#include <utils/CLI11.hpp>

#endif

namespace mqtt::mqtt {

    SocketContextFactory::SocketContextFactory(const CLI::App* sessionApp, const CLI::App* subApp, const CLI::App* pubApp)
        : sessionApp(sessionApp)
        , subApp(subApp)
        , pubApp(pubApp) {
    }

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        const std::string clientId = sessionApp->get_option("--client-id")->as<std::string>();
        const uint8_t qoS = sessionApp->get_option("--qos")->as<uint8_t>();
        const bool cleanSession = !sessionApp->get_option("--retain-session")->as<bool>();
        const uint16_t keepAlive = sessionApp->get_option("--keep-alive")->as<uint16_t>();
        const std::string willTopic = sessionApp->get_option("--will-topic")->as<std::string>();
        const std::string willMessage = sessionApp->get_option("--will-message")->as<std::string>();
        const uint8_t willQoS = sessionApp->get_option("--will-qos")->as<uint8_t>();
        const bool willRetain = sessionApp->get_option("--will-retain")->as<bool>();
        const std::string username = sessionApp->get_option("--username")->as<std::string>();
        const std::string password = sessionApp->get_option("--password")->as<std::string>();

        const std::list<std::string> subTopics =
            subApp->count() > 0 ? subApp->get_option("--topic")->as<std::list<std::string>>() : std::list<std::string>{};

        const std::string pubTopic = pubApp->count() > 0 ? pubApp->get_option("--topic")->as<std::string>() : "";
        const std::string pubMessage = pubApp->get_option("--message")->as<std::string>();
        const bool pubRetain = pubApp->get_option("--retain")->as<bool>();

        core::socket::stream::SocketContext* socketContext = nullptr;

        if (subApp->count() > 0 || pubApp->count() > 0) {
            socketContext = new iot::mqtt::SocketContext(socketConnection,
                                                         new ::mqtt::mqtt::lib::Mqtt(socketConnection->getConnectionName(),
                                                                                     clientId,
                                                                                     qoS,
                                                                                     keepAlive,
                                                                                     cleanSession,
                                                                                     willTopic,
                                                                                     willMessage,
                                                                                     willQoS,
                                                                                     willRetain,
                                                                                     username,
                                                                                     password,
                                                                                     subTopics,
                                                                                     pubTopic,
                                                                                     pubMessage,
                                                                                     pubRetain));
        } else {
            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] One of 'sub' or 'pub' is required";
        }

        return socketContext;
    }

} // namespace mqtt::mqtt

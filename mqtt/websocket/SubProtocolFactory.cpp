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

#include "SubProtocolFactory.h"

#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <log/Logger.h>
#include <net/config/ConfigInstance.h>
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <utils/CLI11.hpp>
#include <utils/Config.h>

#endif

namespace mqtt::mqtt::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        const CLI::App* sessionApp = subProtocolContext->getSocketConnection()->getConfig()->gotSection("session")
                                         ? subProtocolContext->getSocketConnection()->getConfig()->getSection("session")
                                         : utils::Config::getInstance("session");
        const CLI::App* subApp = subProtocolContext->getSocketConnection()->getConfig()->gotSection("sub")
                                     ? subProtocolContext->getSocketConnection()->getConfig()->getSection("sub")
                                     : utils::Config::getInstance("sub");
        const CLI::App* pubApp = subProtocolContext->getSocketConnection()->getConfig()->gotSection("pub")
                                     ? subProtocolContext->getSocketConnection()->getConfig()->getSection("pub")
                                     : utils::Config::getInstance("pub");

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

        iot::mqtt::client::SubProtocol* subProtocol = nullptr;
        if (subApp->count() > 0 || pubApp->count() > 0) {
            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new ::mqtt::mqtt::lib::Mqtt(subProtocolContext->getSocketConnection()->getConnectionName(),
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
            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] "
                    << subProtocolContext->getSocketConnection()->getConnectionName() << ": one of 'sub' or 'pub' is required";
        }

        return subProtocol;
    }

} // namespace mqtt::mqtt::websocket

extern "C" mqtt::mqtt::websocket::SubProtocolFactory* mqttClientSubProtocolFactory() {
    return new mqtt::mqtt::websocket::SubProtocolFactory();
}

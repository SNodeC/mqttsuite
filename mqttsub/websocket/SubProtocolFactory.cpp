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
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <utils/CLI11.hpp>
#include <utils/Config.h>

#endif

namespace mqtt::mqttsub::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        const CLI::App* subApp = utils::Config::getInstance("sub");

        if (subApp != nullptr) {
            const std::string clientId = subApp->get_option("--client-id")->as<std::string>();
            const std::list<std::string> topics = subApp->get_option("--topic")->as<std::list<std::string>>();
            const uint8_t qoS = subApp->get_option("--qos")->as<uint8_t>();
            const uint16_t keepAlive = subApp->get_option("--keep-alive")->as<uint16_t>();
            const bool cleanSession = subApp->get_option("--clean-session")->as<bool>();

            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new mqtt::mqttsub::lib::Mqtt(
                    subProtocolContext->getSocketConnection()->getConnectionName(), clientId, topics, qoS, keepAlive, cleanSession));
        }

        return subProtocol;
    }

} // namespace mqtt::mqttsub::websocket

extern "C" mqtt::mqttsub::websocket::SubProtocolFactory* mqttClientSubProtocolFactory() {
    return new mqtt::mqttsub::websocket::SubProtocolFactory();
}

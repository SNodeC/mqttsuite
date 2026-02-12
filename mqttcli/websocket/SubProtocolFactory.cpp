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

#include "lib/Mqtt.h"
#include "mqttcli/ConfigSections.h"

#include <core/socket/stream/SocketConnection.h>
#include <log/Logger.h>
#include <net/config/ConfigInstance.h>
#include <net/config/ConfigSectionAPI.hpp>
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <utils/CLI11.hpp>

#endif

namespace mqtt::mqttcli::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        const ConfigSession* configSection =
            subProtocolContext->getSocketConnection()->getConfigInstance()->getSection<ConfigSession>("session", true, true);
        const ConfigSubscribe* configSubecribe =
            subProtocolContext->getSocketConnection()->getConfigInstance()->getSection<ConfigSubscribe>("sub", true, true);
        const ConfigPublish* configPublish =
            subProtocolContext->getSocketConnection()->getConfigInstance()->getSection<ConfigPublish>("pub", true, true);

        configSubecribe = (configSubecribe != nullptr && configSubecribe->getOption("--topic")->count() > 0) ? configSubecribe : nullptr;
        configPublish = (configPublish != nullptr && configPublish->getOption("--topic")->count() > 0 &&
                         configPublish->getOption("--message")->count() > 0)
                            ? configPublish
                            : nullptr;

        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        if (configSubecribe != nullptr || configPublish != nullptr) {
            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new ::mqtt::mqttcli::lib::Mqtt(
                    subProtocolContext->getSocketConnection()->getConnectionName(),
                    configSection != nullptr ? configSection->getOption("--client-id")->as<std::string>() : "",
                    configSection != nullptr ? configSection->getOption("--qos")->as<uint8_t>() : 0,
                    configSection != nullptr ? configSection->getOption("--keep-alive")->as<uint16_t>() : 60,
                    configSection != nullptr ? !configSection->getOption("--retain-session")->as<bool>() : true,
                    configSection != nullptr ? configSection->getOption("--will-topic")->as<std::string>() : "",
                    configSection != nullptr ? configSection->getOption("--will-message")->as<std::string>() : "",
                    configSection != nullptr ? configSection->getOption("--will-qos")->as<uint8_t>() : 0,
                    configSection != nullptr ? configSection->getOption("--will-retain")->as<bool>() : false,
                    configSection != nullptr ? configSection->getOption("--username")->as<std::string>() : "",
                    configSection != nullptr ? configSection->getOption("--password")->as<std::string>() : "",
                    configSubecribe != nullptr ? configSubecribe->getOption("--topic")->as<std::list<std::string>>()
                                               : std::list<std::string>(),
                    configPublish != nullptr ? configPublish->getOption("--topic")->as<std::string>() : "",
                    configPublish != nullptr ? configPublish->getOption("--message")->as<std::string>() : "",
                    configPublish != nullptr ? configPublish->getOption("--retain")->as<bool>() : false));
        } else {
            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT << "] "
                    << subProtocolContext->getSocketConnection()->getConnectionName() << ": one of 'sub' or 'pub' is required";
        }

        return subProtocol;
    }

} // namespace mqtt::mqttcli::websocket

extern "C" web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* mqttClientSubProtocolFactory() {
    return new mqtt::mqttcli::websocket::SubProtocolFactory();
}

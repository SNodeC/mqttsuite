/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SubProtocolFactory.h"

#include "lib/JsonMappingReader.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json.hpp>
#include <utils/Config.h>

#endif

namespace mqtt::mqttintegrator::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        nlohmann::json& mappingJson =
            mqtt::lib::JsonMappingReader::readMappingFromFile(utils::Config::getStringOptionValue("--mqtt-mapping-file"));

        if (mappingJson.contains("connection")) {
            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new mqtt::mqttintegrator::lib::Mqtt(
                    subProtocolContext->getSocketConnection()->getConnectionName(), mappingJson["connection"], mappingJson["mapping"]));
        }

        return subProtocol;
    }

} // namespace mqtt::mqttintegrator::websocket

extern "C" mqtt::mqttintegrator::websocket::SubProtocolFactory* mqttClientSubProtocolFactory() {
    return new mqtt::mqttintegrator::websocket::SubProtocolFactory();
}

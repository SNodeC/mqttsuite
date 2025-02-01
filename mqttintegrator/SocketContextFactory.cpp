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

#include "SocketContextFactory.h"

#include "lib/JsonMappingReader.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <utils/Config.h>

#endif

namespace mqtt::mqttintegrator {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        iot::mqtt::SocketContext* socketContext = nullptr;

        nlohmann::json& mappingJson =
            mqtt::lib::JsonMappingReader::readMappingFromFile(utils::Config::getStringOptionValue("--mqtt-mapping-file"));

        if (mappingJson.contains("connection")) {
            socketContext =
                new iot::mqtt::SocketContext(socketConnection,
                                             new mqtt::mqttintegrator::lib::Mqtt(
                                                 socketConnection->getConnectionName(), mappingJson["connection"], mappingJson["mapping"]));
        }

        return socketContext;
    }

} // namespace mqtt::mqttintegrator

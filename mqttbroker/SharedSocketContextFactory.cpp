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

#include "SharedSocketContextFactory.h"

#include "lib/JsonMappingReader.h"
#include "mqttbroker/lib/Mqtt.h"

#include <iot/mqtt/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utils/Config.h>

#endif

namespace mqtt::mqttbroker {

    core::socket::stream::SocketContext* SharedSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection,
                                                                            std::shared_ptr<iot::mqtt::server::broker::Broker> broker) {
        return new iot::mqtt::SocketContext(
            socketConnection,
            new mqtt::mqttbroker::lib::Mqtt(
                broker,
                mqtt::lib::JsonMappingReader::readMappingFromFile(utils::Config::getStringOptionValue("--mqtt-mapping-file"))["mapping"]));
    }

} // namespace mqtt::mqttbroker

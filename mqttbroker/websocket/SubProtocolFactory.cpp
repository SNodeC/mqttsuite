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

#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <utils/Config.h>

#endif

namespace mqtt::mqttbroker::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::server::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::server::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        return new iot::mqtt::server::SubProtocol(
            subProtocolContext,
            getName(),
            new mqtt::mqttbroker::lib::Mqtt(
                iot::mqtt::server::broker::Broker::instance(SUBSCRIBTION_MAX_QOS),
                mqtt::lib::JsonMappingReader::readMappingFromFile(utils::Config::getStringOptionValue("--mqtt-mapping-file"))["mapping"]));
    }

} // namespace mqtt::mqttbroker::websocket

extern "C" mqtt::mqttbroker::websocket::SubProtocolFactory *subProtocolFactory() {
    return new mqtt::mqttbroker::websocket::SubProtocolFactory();
}

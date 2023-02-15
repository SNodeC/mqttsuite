/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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
#include "mqttintegrator/lib/Mqtt.h"

//

#include <cstdlib>
#include <map>

// IWYU pragma: no_include  <iot/mqtt/MqttSubProtocol.hpp>

namespace mqtt::mqttintegrator::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
        char* mappingFile = getenv("MQTT_MAPPING_FILE");

        if (mappingFile != nullptr) {
            mappingJson = mqtt::lib::JsonMappingReader::readMappingFromFile(mappingFile);
        }
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        if (mappingJson.contains("connection")) {
            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext, getName(), new mqtt::mqttintegrator::lib::Mqtt(mappingJson["connection"], mappingJson["mapping"]));
        }

        return subProtocol;
    }

} // namespace mqtt::mqttintegrator::websocket

extern "C" void* mqttClientSubProtocolFactory() {
    return new mqtt::mqttintegrator::websocket::SubProtocolFactory();
}

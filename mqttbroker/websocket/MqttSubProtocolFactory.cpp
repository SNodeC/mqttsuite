/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "MqttSubProtocolFactory.h"

#include "lib/JsonMappingReader.h"

#include <iot/mqtt/server/broker/Broker.h>

//

#include <cstdlib>

#define NAME "mqtt"

namespace mqtt::mqttbroker::websocket {

    MqttSubprotocolFactory::MqttSubprotocolFactory(const std::string& name)
        : web::websocket::SubProtocolFactory<MqttSubProtocol>::SubProtocolFactory(name) {
        char* mappingFile = getenv("MQTT_MAPPING_FILE");

        if (mappingFile != nullptr) {
            nlohmann::json mappingJson = mqtt::lib::JsonMappingReader::readMappingFromFile(mappingFile);

            if (!mappingJson.empty()) {
                jsonMapping = mappingJson["mapping"];
            }
        }
    }

    MqttSubProtocol* MqttSubprotocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        return new MqttSubProtocol(
            subProtocolContext, getName(), iot::mqtt::server::broker::Broker::instance(SUBSCRIBTION_MAX_QOS), jsonMapping);
    }

} // namespace mqtt::mqttbroker::websocket

extern "C" void* mqttServerSubProtocolFactory() {
    return new mqtt::mqttbroker::websocket::MqttSubprotocolFactory(NAME);
}
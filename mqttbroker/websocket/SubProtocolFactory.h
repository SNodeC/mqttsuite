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

#ifndef APPS_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H
#define APPS_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H

namespace web::websocket {
    class SubProtocolContext;
}

#include <iot/mqtt/server/SubProtocol.h>
#include <web/websocket/SubProtocolFactory.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json_fwd.hpp>

#endif

namespace mqtt::mqttbroker::websocket {

    class SubProtocolFactory : public web::websocket::SubProtocolFactory<iot::mqtt::server::SubProtocol> {
    public:
        SubProtocolFactory();

    private:
        iot::mqtt::server::SubProtocol* create(web::websocket::SubProtocolContext* subProtocolContext) override;

        nlohmann::json& mappingJson;
    };

} // namespace mqtt::mqttbroker::websocket

extern "C" void* mqttServerSubProtocolFactory();

#endif // WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H

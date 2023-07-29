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

#ifndef APPS_MQTTBROKER_MQTTINTEGRATOR_WEBSOCKET_SUBPROTOCOLFACTORY_H
#define APPS_MQTTBROKER_MQTTINTEGRATOR_WEBSOCKET_SUBPROTOCOLFACTORY_H

namespace web::websocket {
    class SubProtocolContext;
}

#include <iot/mqtt/client/SubProtocol.h>
#include <web/websocket/SubProtocolFactory.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif

namespace mqtt::mqttintegrator::websocket {

    class SubProtocolFactory : public web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol> {
    public:
        explicit SubProtocolFactory();

    private:
        iot::mqtt::client::SubProtocol* create(web::websocket::SubProtocolContext* subProtocolContext) override;
    };

} // namespace mqtt::mqttintegrator::websocket

extern "C" mqtt::mqttintegrator::websocket::SubProtocolFactory* mqttClientSubProtocolFactory();

#endif // APPS_MQTTBROKER_MQTTINTEGRATOR_WEBSOCKET_SUBPROTOCOLFACTORY_H

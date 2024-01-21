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

#ifndef APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXTFACTORY_H
#define APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXTFACTORY_H

namespace mqtt::bridge::lib {
    class Bridge;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/socket/stream/SocketContext.h>
#include <core/socket/stream/SocketContextFactory.h> // IWYU pragma: export
#include <iot/mqtt/Topic.h>
#include <list>

#endif

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

namespace mqtt::bridge {

    class SocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        SocketContextFactory(mqtt::bridge::lib::Bridge& bridge, const std::list<iot::mqtt::Topic>& topics);

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) final;

    private:
        mqtt::bridge::lib::Bridge& bridge;
        const std::list<iot::mqtt::Topic> topics;
    };

} // namespace mqtt::bridge

#endif // APPS_MQTTBROKER_MQTTBRIDGE_SOCKETCONTEXTFACTORY_H

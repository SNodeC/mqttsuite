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

#include "SocketContextFactory.h"

#include "lib/Mqtt.h"

#include <iot/mqtt/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif

namespace mqtt::bridge {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new iot::mqtt::SocketContext(socketConnection, new mqtt::bridge::lib::Mqtt(bridge, topics));
    }

    SocketContextFactory& SocketContextFactory::setBridge(lib::Bridge* bridge) {
        this->bridge = bridge;
        return *this;
    }

    SocketContextFactory& SocketContextFactory::setTopics(const std::list<iot::mqtt::Topic>& topics) {
        this->topics = topics;
        return *this;
    }

} // namespace mqtt::bridge

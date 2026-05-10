/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#ifndef MQTTSTORE_SOCKETCONTEXTFACTORY_H
#define MQTTSTORE_SOCKETCONTEXTFACTORY_H

#include <core/socket/stream/SocketContextFactory.h> // IWYU pragma: export

namespace mqtt::mqttstore {

    class SocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) final;
    };

} // namespace mqtt::mqttstore

#endif // MQTTSTORE_SOCKETCONTEXTFACTORY_H

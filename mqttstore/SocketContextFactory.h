/*
 * MQTTSuite - A lightweight MQTT Integration System
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#ifndef MQTTSTORE_SOCKETCONTEXTFACTORY_H
#define MQTTSTORE_SOCKETCONTEXTFACTORY_H

#include <core/socket/stream/SocketContextFactory.h>

namespace mqtt::mqttstore {

    class SocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) final;
    };

} // namespace mqtt::mqttstore

#endif // MQTTSTORE_SOCKETCONTEXTFACTORY_H

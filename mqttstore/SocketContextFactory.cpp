/*
 * MQTTSuite - A lightweight MQTT Integration System
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#include "SocketContextFactory.h"

#include "lib/ConfigSections.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>
#include <net/config/ConfigInstance.h>

namespace mqtt::mqttstore {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        const net::config::ConfigInstance* configInstance = socketConnection->getConfigInstance();

        const lib::ConfigSession* configSession = configInstance->getSubCommand<lib::ConfigSession>();
        const lib::ConfigSubscribe* configSubscribe = configInstance->getSubCommand<lib::ConfigSubscribe>();
        const lib::ConfigDatabase* configDatabase = configInstance->getSubCommand<lib::ConfigDatabase>();
        const lib::ConfigStorage* configStorage = configInstance->getSubCommand<lib::ConfigStorage>();

        return new iot::mqtt::SocketContext(socketConnection,
                                            new lib::Mqtt(socketConnection->getConnectionName(),
                                                          configSession->getClientId(),
                                                          configSession->getQoS(),
                                                          configSession->getKeepAlive(),
                                                          !configSession->getRetainSession(),
                                                          configSession->getWillTopic(),
                                                          configSession->getWillMessage(),
                                                          configSession->getWillQoS(),
                                                          configSession->getWillRetain(),
                                                          configSession->getUsername(),
                                                          configSession->getPassword(),
                                                          configSubscribe->getTopic(),
                                                          {.connectionName = socketConnection->getConnectionName(),
                                                           .database = configDatabase->getDatabase(),
                                                           .username = configDatabase->getUsername(),
                                                           .password = configDatabase->getPassword(),
                                                           .host = configDatabase->getHost(),
                                                           .port = configDatabase->getPort(),
                                                           .socket = configDatabase->getSocket(),
                                                           .flags = configDatabase->getFlags()},
                                                          {.table = configStorage->getTable(),
                                                           .autoCreate = configStorage->getAutoCreate(),
                                                           .storePayloadText = configStorage->getStorePayloadText(),
                                                           .storePayloadJson = configStorage->getStorePayloadJson()}));
    }

} // namespace mqtt::mqttstore

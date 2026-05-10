/*
 * MQTTSuite - A lightweight MQTT Integration System
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#include "SubProtocolFactory.h"

#include "lib/ConfigSections.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <net/config/ConfigInstance.h>
#include <web/websocket/SubProtocolContext.h>

namespace mqtt::mqttstore::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        const net::config::ConfigInstance* configInstance = subProtocolContext->getSocketConnection()->getConfigInstance();

        const lib::ConfigSession* configSession = configInstance->getSubCommand<lib::ConfigSession>();
        const lib::ConfigSubscribe* configSubscribe = configInstance->getSubCommand<lib::ConfigSubscribe>();
        const lib::ConfigDatabase* configDatabase = configInstance->getSubCommand<lib::ConfigDatabase>();
        const lib::ConfigStorage* configStorage = configInstance->getSubCommand<lib::ConfigStorage>();
        const std::string connectionName = subProtocolContext->getSocketConnection()->getConnectionName();

        return new iot::mqtt::client::SubProtocol(subProtocolContext,
                                                  getName(),
                                                  new lib::Mqtt(connectionName,
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
                                                                {.connectionName = connectionName,
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

} // namespace mqtt::mqttstore::websocket

extern "C" web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* mqttClientSubProtocolFactory() {
    return new mqtt::mqttstore::websocket::SubProtocolFactory();
}

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

        return new iot::mqtt::client::SubProtocol(
            subProtocolContext,
            getName(),
            new mqtt::mqttstore::lib::Mqtt(subProtocolContext->getSocketConnection()->getConnectionName(),
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
                                           configDatabase->getDatabase(),
                                           configDatabase->getUsername(),
                                           configDatabase->getPassword(),
                                           configDatabase->getHost(),
                                           configDatabase->getPort(),
                                           configDatabase->getSocket(),
                                           configDatabase->getFlags(),
                                           configStorage->getTable(),
                                           configStorage->getCreateSchema(),
                                           configStorage->getProjectJson()));
    }

} // namespace mqtt::mqttstore::websocket

/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "SubProtocolFactory.h"

#include "lib/BridgeStore.h"
#include "lib/Mqtt.h"

#include <core/socket/stream/SocketConnection.h>
#include <web/websocket/SubProtocolContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <cstdlib>
#include <list>
#include <log/Logger.h>

#endif

namespace mqtt::mqttbridge::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        const std::string& instanceName = subProtocolContext->getSocketConnection()->getInstanceName();

        const mqtt::bridge::lib::Broker& broker = mqtt::bridge::lib::BridgeStore::instance().getBroker(instanceName);

        VLOG(1) << "  Creating Broker instance '" << broker.getInstanceName() << "' of Bridge '" << broker.getBridge().getName() << "'";
        VLOG(1) << "    Bridge client id : " << broker.getClientId();
        VLOG(1) << "    Transport: " << broker.getTransport();
        VLOG(1) << "    Protocol: " << broker.getProtocol();
        VLOG(1) << "    Encryption: " << broker.getEncryption();

        VLOG(1) << "    Topics:";
        const std::list<iot::mqtt::Topic>& topics = broker.getTopics();
        for (const iot::mqtt::Topic& topic : topics) {
            VLOG(1) << "      " << static_cast<uint16_t>(topic.getQoS()) << ":" << topic.getName();
        }

        if (!topics.empty()) {
            subProtocol = new iot::mqtt::client::SubProtocol(
                subProtocolContext,
                getName(),
                new mqtt::bridge::lib::Mqtt(subProtocolContext->getSocketConnection()->getConnectionName(), broker));
        }

        return subProtocol;
    }

} // namespace mqtt::mqttbridge::websocket

extern "C" mqtt::mqttbridge::websocket::SubProtocolFactory* mqttClientSubProtocolFactory() {
    mqtt::mqttbridge::websocket::SubProtocolFactory* subProtocolFactory = nullptr;

    const char* bridgeConfigFileEnvPtr = std::getenv("BRIDGE_CONFIG");
    const std::string& bridgeConfigFile = (bridgeConfigFileEnvPtr != nullptr) ? bridgeConfigFileEnvPtr : "";

    const bool success = mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(bridgeConfigFile);

    if (success) {
        subProtocolFactory = new mqtt::mqttbridge::websocket::SubProtocolFactory();
    }

    return subProtocolFactory;
}

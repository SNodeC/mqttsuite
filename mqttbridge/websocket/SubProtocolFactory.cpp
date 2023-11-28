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

#include "SubProtocolFactory.h"

#include "lib/BridgeStore.h"
#include "lib/Mqtt.h"

namespace mqtt::bridge::lib {
    class Bridge;
}

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/Topic.h>
#include <web/websocket/SubProtocolContext.h>

// IWYU pragma: no_include "iot/mqtt/client/SubProtocol.h"
// IWYU pragma: no_include "web/websocket/SubProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <utils/Config.h>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif

namespace mqtt::mqttbridge::websocket {

#define NAME "mqtt"

    SubProtocolFactory::SubProtocolFactory()
        : web::websocket::SubProtocolFactory<iot::mqtt::client::SubProtocol>::SubProtocolFactory(NAME) {
    }

    iot::mqtt::client::SubProtocol* SubProtocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        iot::mqtt::client::SubProtocol* subProtocol = nullptr;

        const char* bridgeConfigFileEnvPtr = std::getenv("BRIDGE_CONFIG");
        const std::string& bridgeConfigFile = (bridgeConfigFileEnvPtr != nullptr) ? bridgeConfigFileEnvPtr : "";

        if (!bridgeConfigFile.empty()) {
            const bool success = mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(bridgeConfigFile);

            if (success) {
                const std::string& instanceName = subProtocolContext->getSocketConnection()->getInstanceName();

                nlohmann::json& brokerJsonConfig = mqtt::bridge::lib::BridgeStore::instance().getBrokerJsonConfig(instanceName);
                if (!brokerJsonConfig.empty()) {
                    VLOG(1) << "  Creating bridge instance: " << instanceName;
                    VLOG(1) << "    Protocol: " << brokerJsonConfig["protocol"];
                    VLOG(1) << "    Encryption: " << brokerJsonConfig["encryption"];

                    mqtt::bridge::lib::Bridge* bridge = mqtt::bridge::lib::BridgeStore::instance().getBridge(instanceName);

                    std::list<iot::mqtt::Topic> topics;
                    for (const nlohmann::json& topicJson : brokerJsonConfig["topics"]) {
                        VLOG(1) << "    Topic: " << topicJson["topic"];
                        VLOG(1) << "      Qos: " << topicJson["qos"];

                        topics.emplace_back(topicJson["topic"], topicJson["qos"]);
                    }

                    if (bridge != nullptr && !topics.empty()) {
                        subProtocol =
                            new iot::mqtt::client::SubProtocol(subProtocolContext, getName(), new mqtt::bridge::lib::Mqtt(bridge, topics));
                    }
                }
            }
        }

        return subProtocol;
    }

} // namespace mqtt::mqttbridge::websocket

extern "C" mqtt::mqttbridge::websocket::SubProtocolFactory* mqttClientSubProtocolFactory() {
    mqtt::mqttbridge::websocket::SubProtocolFactory* subProtocolFactory = nullptr;

    const bool success =
        mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(utils::Config::get_string_option_value("--bridge-config"));

    if (success) {
        subProtocolFactory = new mqtt::mqttbridge::websocket::SubProtocolFactory();
    }

    return subProtocolFactory;
}

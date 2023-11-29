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

#include "SocketContextFactory.h" // IWYU pragma: keep
#include "lib/Bridge.h"
#include "lib/BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <iot/mqtt/Topic.h>
#include <list>
#include <log/Logger.h>
#include <map>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/tls/SocketClient.h>
#include <net/in6/stream/legacy/SocketClient.h>
#include <net/in6/stream/tls/SocketClient.h>
#include <net/l2/stream/legacy/SocketClient.h>
#include <net/l2/stream/tls/SocketClient.h>
#include <net/rc/stream/legacy/SocketClient.h>
#include <net/rc/stream/tls/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <net/un/stream/tls/SocketClient.h>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>
#include <utils/Config.h>

// IWYU pragma: no_include <bits/utility.h>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif

template <typename SocketAddressT>
void reportState(const std::string& instanceName, const SocketAddressT& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "': " << state.what();
            break;
        case core::socket::State::DISABLED:
            VLOG(1) << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": error occurred";
            VLOG(1) << "    " << state.what();
            break;
        case core::socket::State::FATAL:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": fatal error occurred";
            VLOG(1) << "    " << state.what();
            break;
    }
}

template <template <typename> typename SocketClient, typename SocketContextFactory>
void startClient(const std::string& name, const std::function<void(SocketClient<SocketContextFactory>&)> configurator) {
    using Client = SocketClient<SocketContextFactory>;
    using SocketAddress = typename Client::SocketAddress;

    Client client(name);
    client.getConfig().setRetry();
    client.getConfig().setRetryBase(1);
    client.getConfig().setReconnect();
    configurator(client);
    client.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(name, socketAddress, state);
    });
}

template <template <typename> typename SocketClient, typename SocketContextFactory>
void startClient(const std::string& name) {
    startClient<SocketClient, SocketContextFactory>(name, []([[maybe_unused]] SocketClient<SocketContextFactory>& client) -> void {
    });
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--bridge-config", "MQTT bridge configuration file (JSON format)", "[path]");

    core::SNodeC::init(argc, argv);

    const bool success =
        mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(utils::Config::get_string_option_value("--bridge-config"));

    if (success) {
        for (const auto& [instanceName, brokerJsonConfig] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
            const std::string& name = brokerJsonConfig["name"];
            const std::string& protocol = brokerJsonConfig["protocol"];
            const std::string& encryption = brokerJsonConfig["encryption"];
            const std::string& transport = brokerJsonConfig["transport"];

            if (transport == "stream") {
                VLOG(1) << "  Creating bridge instance: " << name;
                VLOG(1) << "    Protocol: " << protocol;
                VLOG(1) << "    Encryption: " << encryption;

                const nlohmann::json& topicsJson = brokerJsonConfig["topics"];

                std::list<iot::mqtt::Topic> topics;
                for (const nlohmann::json& topicJson : topicsJson) {
                    VLOG(1) << "    Topic: " << topicJson["topic"];
                    VLOG(1) << "      Qos: " << topicJson["qos"];

                    topics.emplace_back(topicJson["topic"], topicJson["qos"]);
                }

                mqtt::bridge::lib::Bridge* bridge = mqtt::bridge::lib::BridgeStore::instance().getBridge(instanceName);

                if (protocol == "in") {
                    if (encryption == "legacy") {
                        startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    } else if (encryption == "tls") {
                        startClient<net::in::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    }
                } else if (protocol == "in6") {
                    if (encryption == "legacy") {
                        startClient<net::in6::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    } else if (encryption == "tls") {
                        startClient<net::in6::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    }
                } else if (protocol == "l2") {
                    if (encryption == "legacy") {
                        startClient<net::l2::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    } else if (encryption == "tls") {
                        startClient<net::l2::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    }
                } else if (protocol == "rc") {
                    if (encryption == "legacy") {
                        startClient<net::rc::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    } else if (encryption == "tls") {
                        startClient<net::rc::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    }
                } else if (protocol == "un") {
                    if (encryption == "legacy") {
                        startClient<net::un::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    } else if (encryption == "tls") {
                        startClient<net::un::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                            name, [bridge, &topics](auto& mqttBroker) -> void {
                                mqttBroker.getSocketContextFactory()->setBridge(bridge).setTopics(topics);
                            });
                    }
                }
            }
        }
    }

    return core::SNodeC::start();
}

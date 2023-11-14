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

#include "BridgeConfigLoader.h"
#include "SocketContextFactory.h" // IWYU pragma: keep
#include "mqttbridge/lib/Bridge.h"

#include <core/SNodeC.h>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <initializer_list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <type_traits>
#include <utils/Config.h>
#include <variant>
#include <vector>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

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
    configurator(client);
    client.connect([&name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(name, socketAddress, state);
    });
}

template <template <typename> typename SocketClient, typename SocketContextFactory>
void startClient(const std::string& name) {
    using Client = SocketClient<SocketContextFactory>;
    using SocketAddress = typename Client::SocketAddress;

    Client client(name);
    client.connect([&name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(name, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--bridge-config-file", "MQTT mapping file (json format) for integration", "[path]");

    core::SNodeC::init(argc, argv);

    nlohmann::json bridgesJsonConfig = BridgeConfigLoader::loadAndValidate(utils::Config::get_string_option_value("--bridge-config-file"));

    std::set<mqtt::bridge::lib::Bridge*> bridges;

    for (const nlohmann::json& bridgeJsonConfig : bridgesJsonConfig["bridges"]) {
        mqtt::bridge::lib::Bridge* bridge = new mqtt::bridge::lib::Bridge(bridgeJsonConfig["connection"]);
        bridges.insert(bridge);

        VLOG(1) << "Creating Bridge: " << bridgeJsonConfig["connection"]["client_id"];

        for (const nlohmann::json& brokerJsonConfig : bridgeJsonConfig["broker"]) {
            const std::string name = brokerJsonConfig["name"];
            const std::string protocol = brokerJsonConfig["protocol"];
            const std::string encryption = brokerJsonConfig["encryption"];
            const std::vector<std::string> topics = brokerJsonConfig["topics"];

            VLOG(1) << "Creating Instance: " << name;
            VLOG(1) << "         Protocol: " << protocol;
            VLOG(1) << "         Encryption: " << encryption;
            for (const std::string& topic : topics) {
                VLOG(1) << "         Topic: " << topic;
            }

            /*
            if (protocol == "in") {
                if (encryption == "legacy") {
                    startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                } else if (encryption == "tls") {
                    startClient<net::in::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                }
            } else if (protocol == "in6") {
                if (encryption == "legacy") {
                    startClient<net::in6::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                } else if (encryption == "tls") {
                    startClient<net::in6::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                }
            } else if (protocol == "l2") {
                if (encryption == "legacy") {
                    startClient<net::l2::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                } else if (encryption == "tls") {
                    startClient<net::l2::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                }
            } else if (protocol == "rc") {
                if (encryption == "legacy") {
                    startClient<net::rc::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                } else if (encryption == "tls") {
                    startClient<net::rc::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                }
            } else if (protocol == "un") {
                if (encryption == "legacy") {
                    startClient<net::un::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                } else if (encryption == "tls") {
                    startClient<net::un::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                        name, [&bridge](auto& mqttBridge) -> void {
                            mqttBridge.getSocketContextFactory()->setBridge(bridge);
                        });
                }
            }
            */
        }
    }

    mqtt::bridge::lib::Bridge bridge(nlohmann::json::parse(R"(
        {
            "keep_alive":60,
            "client_id":"MQTT-Bridge",
            "clean_session":true,
            "will_topic":"will/topic",
            "will_message":"Last Will",
            "will_qos":0,
            "will_retain":false,
            "username":"Username",
            "password":"Password"
        }
    )"));

    for (int i = 1; i < 4; i++) {
        startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
            "in-mqtt-" + std::to_string(i), [&bridge](auto& mqttBridge) -> void {
                mqttBridge.getSocketContextFactory()->setBridge(&bridge);
            });
    }
    /*
    startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
        "in-mqtt-1", [&bridge](auto& mqttBridge) -> void {
            mqttBridge.getSocketContextFactory()->setBridge(bridge);
        });

    startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
        "in-mqtt-2", [&bridge](auto& mqttBridge) -> void {
            mqttBridge.getSocketContextFactory()->setBridge(bridge);
        });

    startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
        "in-mqtt-3", [&bridge](auto& mqttBridge) -> void {
            mqttBridge.getSocketContextFactory()->setBridge(bridge);
        });
*/
    startClient<net::un::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
        "un-mqtt-1", [&bridge](auto& mqttBridge) -> void {
            mqttBridge.getSocketContextFactory()->setBridge(&bridge);
            mqttBridge.getConfig().setDisabled();
        });

    /*
        {
            using Bridge = net::in::stream::legacy::SocketClient<mqtt::bridge::SocketContextFactory>;
            using SocketAddress = Bridge::SocketAddress;

            Bridge integrator("in-mqtt-1");
            integrator.getSocketContextFactory()->setBridge(bridge);
            integrator.connect([](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                reportState("in-mqtt-1", socketAddress, state);
            });
        }

        {
            using Bridge = net::in::stream::legacy::SocketClient<mqtt::bridge::SocketContextFactory>;
            using SocketAddress = Bridge::SocketAddress;

            Bridge integrator("in-mqtt-2");
            integrator.getSocketContextFactory()->setBridge(bridge);
            integrator.connect([](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                reportState("in-mqtt-2", socketAddress, state);
            });
        }

        {
            using Bridge = net::un::stream::legacy::SocketClient<mqtt::bridge::SocketContextFactory>;
            using SocketAddress = Bridge::SocketAddress;

            Bridge integrator("un-mqtt-1");
            integrator.getSocketContextFactory()->setBridge(bridge);
            integrator.getConfig().setDisabled();
            integrator.connect([](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                reportState("un-mqtt-1", socketAddress, state);
            });
        }
    */
    return core::SNodeC::start();
}

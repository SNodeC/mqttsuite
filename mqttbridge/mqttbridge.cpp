/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SocketContextFactory.h" // IWYU pragma: keep
#include "config.h"
#include "lib/BridgeStore.h"
#include "lib/Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <core/eventreceiver/ConnectEventReceiver.h>
#include <express/legacy/in/Server.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/tls/in/Server.h>
#include <iot/mqtt/MqttContext.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <nlohmann/json_fwd.hpp>
#include <utils/CLI11.hpp>
//

// Select necessary include files
// ==============================
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4)
#include <net/in/stream/legacy/SocketClient.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4)
#include <net/in/stream/tls/SocketClient.h>
#endif
#endif

#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6)
#include <net/in6/stream/legacy/SocketClient.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6)
#include <net/in6/stream/tls/SocketClient.h>
#endif
#endif

#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX)
#include <net/un/stream/legacy/SocketClient.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS)
#include <net/un/stream/tls/SocketClient.h>
#endif
#endif

#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
#include <web/http/legacy/in/Client.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
#include <web/http/tls/in/Client.h>
#endif
#endif

#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
#include <web/http/legacy/in6/Client.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
#include <web/http/tls/in6/Client.h>
#endif
#endif

#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
#include <web/http/legacy/un/Client.h>
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
#include <web/http/tls/un/Client.h>
#endif
#endif

#include <list>
#include <nlohmann/json.hpp>
#include <set>
#include <string>

#endif

static std::map<std::string, std::map<std::string, core::socket::stream::SocketConnection*>> bridges;
static std::set<core::eventreceiver::ConnectEventReceiver*> activeConnectors;

static bool restart = false;
static std::string bridgeDefinitionFile = "<REQUIRED>";

static void startBridges();

static void tryRestartBridges() {
    if (bridges.empty() && activeConnectors.empty() && restart) {
        core::EventReceiver::atNextTick([]() {
            mqtt::bridge::lib::BridgeStore::instance().activateStaged();

            startBridges();

            utils::Config::parse2();

            restart = false;
        });
    }
}

static void handleConnector(core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
    if (connectEventReceiver->isEnabled()) {
        activeConnectors.insert(connectEventReceiver);
    } else {
        activeConnectors.erase(connectEventReceiver);

        tryRestartBridges();
    }
}

static void addBridgeBrokerConnection(const mqtt::bridge::lib::Broker& broker, core::socket::stream::SocketConnection* socketConnection) {
    const std::string& bridgeName(broker.getBridge().getName());

    VLOG(1) << "Add bridge broker: " << socketConnection->getInstanceName();

    bridges[bridgeName][socketConnection->getInstanceName()] = socketConnection;
}

static void delBridgeBrokerConnection(const mqtt::bridge::lib::Broker& broker, core::socket::stream::SocketConnection* socketConnection) {
    const std::string& bridgeName(broker.getBridge().getName());

    VLOG(1) << "Del bridge broker: " << socketConnection->getInstanceName();

    if (bridges.contains(bridgeName) && bridges[bridgeName].contains(socketConnection->getInstanceName())) {
        bridges[bridgeName].erase(socketConnection->getInstanceName());
        if (bridges[bridgeName].empty()) {
            bridges.erase(bridgeName);
        }
    }

    tryRestartBridges();
}

static void closeBridges() {
    for (const auto& bridge : mqtt::bridge::lib::BridgeStore::instance().getBridgeList()) {
        for (const auto& mqtt : bridge.getMqttList()) {
            mqtt->sendDisconnect();
            mqtt->getMqttContext()
                ->getSocketConnection()
                ->getConfigInstance()
                ->getSection("socket")
                ->get_option("--reconnect")
                ->default_str("false");
        }
    }

    for (auto connectEventReceiver : activeConnectors) {
        connectEventReceiver->stopConnect();
    }

    restart = true;
}

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
            break;
        case core::socket::State::DISABLED:
            VLOG(1) << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
        case core::socket::State::FATAL:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

template <typename HttpClient>
void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::MasterRequest>& req) {
            const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();

            req->set("Sec-WebSocket-Protocol", "mqtt");

            req->upgrade(
                "/ws",
                "websocket",
                [connectionName](bool success) {
                    VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket||"
                            << "mqtt" << ") start " << (success ? "success" : "failed");
                },
                []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req,
                   [[maybe_unused]] const std::shared_ptr<web::http::client::Response>& res,
                   [[maybe_unused]] bool success) {
                },
                [connectionName]([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req, const std::string& message) {
                    VLOG(1) << connectionName << ": Request parse error: " << message;
                });
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            VLOG(1) << "Session ended";
        });

    configurator(httpClient.getConfig());

    httpClient
        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
            addBridgeBrokerConnection(*mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                      socketConnection);
        })
        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
            delBridgeBrokerConnection(*mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                      socketConnection);
        })
        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
            handleConnector(connectEventReceiver);
        })
        .connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
            reportState(name, socketAddress, state);
        });
}

static void startBridges() {
    for (const auto& [fullInstanceName, broker] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
        VLOG(1) << "  Creating broker instance: " << fullInstanceName;
        VLOG(1) << "    Broker prefix: " << broker.getPrefix();
        VLOG(1) << "    Broker client id: " << broker.getClientId();
        VLOG(1) << "    Broker disabled: " << broker.getDisabled();
        VLOG(1) << "    Broker address: " << broker.getAddress();
        VLOG(1) << "    Broker prefix: " << broker.getPrefix();
        VLOG(1) << "    Broker username: " << broker.getUsername();
        VLOG(1) << "    Broker password: " << broker.getPassword();
        VLOG(1) << "    Broker client-id: " << broker.getClientId();
        VLOG(1) << "    Broker clean session: " << broker.getCleanSession();
        VLOG(1) << "    Broker will-topic: " << broker.getWillTopic();
        VLOG(1) << "    Broker will-message: " << broker.getWillMessage();
        VLOG(1) << "    Broker will-qos: " << static_cast<int>(broker.getWillQoS());
        VLOG(1) << "    Broker will-retain: " << broker.getWillRetain();
        VLOG(1) << "    Broker loop prevention: " << broker.getLoopPrevention();
        VLOG(1) << "    Bridge disabled: " << broker.getBridge().getDisabled();
        VLOG(1) << "    Bridge prefix: " << broker.getBridge().getPrefix();
        VLOG(1) << "    Bridge Transport: " << broker.getTransport();
        VLOG(1) << "    Bridge Protocol: " << broker.getProtocol();
        VLOG(1) << "    Bridge Encryption: " << broker.getEncryption();

        VLOG(1) << "    Topics:";
        const std::list<iot::mqtt::Topic>& topics = broker.getTopics();
        for (const iot::mqtt::Topic& topic : topics) {
            VLOG(1) << "      " << topic.getName() << ":" << static_cast<uint16_t>(topic.getQoS());
        }

        const std::string& transport = broker.getTransport();
        const std::string& protocol = broker.getProtocol();
        const std::string& encryption = broker.getEncryption();

        if (transport == "stream") {
            if (protocol == "in") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4)
                    net::in::stream::legacy::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();
                            config.setDisableNagleAlgorithm();

                            config.Remote::setHost(broker.getAddress()["host"]);
                            config.Remote::setPort(broker.getAddress()["port"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([&broker, fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(broker.getBridge().getName() + "+" + fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4)
                    net::in::stream::tls::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();
                            config.setDisableNagleAlgorithm();

                            config.Remote::setHost(broker.getAddress()["host"]);
                            config.Remote::setPort(broker.getAddress()["port"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4
                }
            } else if (protocol == "in6") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6)
                    net::in6::stream::legacy::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();
                            config.setDisableNagleAlgorithm();

                            config.Remote::setHost(broker.getAddress()["host"]);
                            config.Remote::setPort(broker.getAddress()["port"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6)
                    net::in6::stream::tls::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();
                            config.setDisableNagleAlgorithm();

                            config.Remote::setHost(broker.getAddress()["host"]);
                            config.Remote::setPort(broker.getAddress()["port"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6
                }
            } else if (protocol == "un") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX)
                    net::un::stream::legacy::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();

                            config.Remote::setSunPath(broker.getAddress()["host"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_UNIX
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_UNIX
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS)
                    net::un::stream::tls::Client<mqtt::bridge::SocketContextFactory>(
                        fullInstanceName,
                        [&broker](auto& config) {
                            config.setRetry();
                            config.setRetryBase(1);
                            config.setReconnect();

                            config.Remote::setSunPath(broker.getAddress()["host"]);

                            config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                        })
                        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
                            addBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
                            delBridgeBrokerConnection(
                                *mqtt::bridge::lib::BridgeStore::instance().getBroker(socketConnection->getInstanceName()),
                                socketConnection);
                        })
                        .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                            handleConnector(connectEventReceiver);
                        })
                        .connect([fullInstanceName](const auto& socketAddress, const core::socket::State& state) {
                            reportState(fullInstanceName, socketAddress, state);
                        });
#else  // CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS
                }
            }
        } else if (transport == "websocket") {
            if (protocol == "in") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
                    startClient<web::http::legacy::in::Client>(fullInstanceName, [&broker](auto& config) {
                        config.Remote::setPort(8080);

                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();
                        config.setDisableNagleAlgorithm();

                        config.Remote::setHost(broker.getAddress()["host"]);
                        config.Remote::setPort(broker.getAddress()["port"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4 && CONFIG_MQTTSUITE_BRIDGE_WS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4 && CONFIG_MQTTSUITE_BRIDGE_WS
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
                    startClient<web::http::tls::in::Client>(fullInstanceName, [&broker](auto& config) {
                        config.Remote::setPort(8088);

                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();
                        config.setDisableNagleAlgorithm();

                        config.Remote::setHost(broker.getAddress()["host"]);
                        config.Remote::setPort(broker.getAddress()["port"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4 && CONFIG_MQTTSUITE_BRIDGE_WSS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4 && CONFIG_MQTTSUITE_BRIDGE_WSS
                }
            } else if (protocol == "in6") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
                    startClient<web::http::legacy::in6::Client>(fullInstanceName, [&broker](auto& config) {
                        config.Remote::setPort(8080);

                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();
                        config.setDisableNagleAlgorithm();

                        config.Remote::setHost(broker.getAddress()["host"]);
                        config.Remote::setPort(broker.getAddress()["port"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6 && CONFIG_MQTTSUITE_BRIDGE_WS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6&&  CONFIG_MQTTSUITE_BRIDGE_WS
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
                    startClient<web::http::tls::in6::Client>(fullInstanceName, [&broker](auto& config) {
                        config.Remote::setPort(8088);

                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();
                        config.setDisableNagleAlgorithm();

                        config.Remote::setHost(broker.getAddress()["host"]);
                        config.Remote::setPort(broker.getAddress()["port"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6 && CONFIG_MQTTSUITE_BRIDGE_WSS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6 && CONFIG_MQTTSUITE_BRIDGE_WSS
                }
            } else if (protocol == "un") {
                if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX) && defined(CONFIG_MQTTSUITE_BRIDGE_WS)
                    startClient<web::http::legacy::un::Client>(fullInstanceName, [&broker](auto& config) {
                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();

                        config.Remote::setSunPath(broker.getAddress()["path"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_UNIX && CONFIG_MQTTSUITE_BRIDGE_WS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_UNIX &&  CONFIG_MQTTSUITE_BRIDGE_WS
                } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS) && defined(CONFIG_MQTTSUITE_BRIDGE_WSS)
                    startClient<web::http::tls::un::Client>(fullInstanceName, [&broker](auto& config) {
                        config.setRetry();
                        config.setRetryBase(1);
                        config.setReconnect();

                        config.Remote::setSunPath(broker.getAddress()["path"]);

                        config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                    });
#else  // CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS && CONFIG_MQTTSUITE_BRIDGE_WSS
                    VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                            << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS && CONFIG_MQTTSUITE_BRIDGE_WSS
                }
            }
        } else {
            VLOG(1) << "    Transport '" << transport << "' not supported.";
        }
    }
}

int main(int argc, char* argv[]) {
#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    CLI::App* bridgeApp = utils::Config::addInstance("bridge", "Configuration for Application mqttbridge", "MQTT-Bridge");
    utils::Config::required(bridgeApp);

    bridgeApp->needs(bridgeApp->add_option("--definition", bridgeDefinitionFile, "MQTT bridge definition file (JSON format)")
                         ->capture_default_str()
                         ->group(bridgeApp->get_formatter()->get_label("Persistent Options"))
                         ->type_name("path")
                         ->configurable()
                         ->required());

    core::SNodeC::init(argc, argv);

    const express::Router router(express::middleware::JsonMiddleware());

    router.get("/api/bridge/config", [] APPLICATION(req, res) { // cppcheck-suppress unknownMacro
        res->send(mqtt::bridge::lib::BridgeStore::instance().getBridgesConfigJson().dump(4));
    });

    router.patch("/api/bridge/config", [] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& jsonPatch) {
                std::string jsonString = jsonPatch.dump(4);

                VLOG(1) << jsonString;

                if (!restart) {
                    closeBridges();

                    if (mqtt::bridge::lib::BridgeStore::instance().patch(jsonPatch)) {
                        res->send(R"({"success": true, "message": "Bridge config patch applied"})"_json.dump());
                    } else {
                        res->status(404).send(R"({"success": false, "message": "Bridge config patch failed to applie"})"_json.dump());
                    }
                } else {
                    res->status(409).send(
                        R"({"success": false, "message": "Bridge is in restarting state. Patch not applied"})"_json.dump());
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    router.get("/api/bridge/sse", [] APPLICATION(req, res) {
    });

    express::legacy::in::Server("admin-legacy", router, reportState, [](auto& config) {
        config.setPort(8081);
        config.setRetry();
        config.setDisableNagleAlgorithm();
        config.setReuseAddress();
    });

    express::tls::in::Server("admin-tls", router, reportState, [](auto& config) {
        config.setPort(8082);
        config.setRetry();
        config.setDisableNagleAlgorithm();
        config.setReuseAddress();
    });

    if (mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(bridgeDefinitionFile)) {
        startBridges();
    } else {
        VLOG(1) << "Loading bridge definition file failed";
    }

    return core::SNodeC::start();
}

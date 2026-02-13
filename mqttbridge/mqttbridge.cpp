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
#include "lib/SSEDistributor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <core/eventreceiver/ConnectEventReceiver.h>
#include <core/socket/stream/AutoConnectControl.h>
#include <express/legacy/in/Server.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>
#include <express/tls/in/Server.h>
#include <iot/mqtt/MqttContext.h>
#include <net/config/ConfigInstanceAPI.hpp>
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

#include <concepts>
#include <list>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>

#endif

static std::set<core::eventreceiver::ConnectEventReceiver*> activeConnectors;
static std::set<std::shared_ptr<core::socket::stream::AutoConnectControl>> autoConnectControllers;

static bool restart = false;

static void startBridges();

static void tryRestartBridges() {
    bool empty = true;

    for (const auto& [bridgeName, bridge] : mqtt::bridge::lib::BridgeStore::instance().getBridgeMap()) {
        empty &= bridge.getMqttList().empty();
    }

    if (empty && activeConnectors.empty() && restart) {
        mqtt::bridge::lib::SSEDistributor::instance().bridgesStopped();

        core::EventReceiver::atNextTick([]() {
            mqtt::bridge::lib::BridgeStore::instance().activateStaged();

            startBridges();

            utils::Config::parse2();

            restart = false;
        });
    }
}

static void handleAutoConnectControllers(std::shared_ptr<core::socket::stream::AutoConnectControl>& autoConnectController) {
    autoConnectControllers.insert(autoConnectController);
}

static void handleConnector(core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
    if (connectEventReceiver->isEnabled()) {
        activeConnectors.insert(connectEventReceiver);
    } else {
        activeConnectors.erase(connectEventReceiver);

        tryRestartBridges();
    }
}

static void addBridgeBrokerConnection(core::socket::stream::SocketConnection* socketConnection) {
    VLOG(1) << "Connection established: " << socketConnection->getInstanceName();
}

static void delBridgeBrokerConnection(core::socket::stream::SocketConnection* socketConnection) {
    VLOG(1) << "Connection closed: " << socketConnection->getInstanceName();

    tryRestartBridges();
}

static void closeBridges() {
    mqtt::bridge::lib::SSEDistributor::instance().bridgesStopping();

    for (const auto& [bridgeName, bridge] : mqtt::bridge::lib::BridgeStore::instance().getBridgeMap()) {
        mqtt::bridge::lib::SSEDistributor::instance().bridgeStopping(bridgeName);
        for (const auto& mqtt : bridge.getMqttList()) {
            mqtt::bridge::lib::SSEDistributor::instance().brokerDisconnecting(
                bridgeName, mqtt->getMqttContext()->getSocketConnection()->getInstanceName());

            mqtt->sendDisconnect();
            mqtt->getMqttContext()
                ->getSocketConnection()
                ->getConfigInstance()
                ->getSection<net::config::ConfigPhysicalSocketClient>("socket")
                ->setReconnect(false);
        }
    }

    for (auto connectEventReceiver : activeConnectors) {
        connectEventReceiver->stopConnect();
    }

    for (auto autoConnectControler : autoConnectControllers) {
        autoConnectControler->stopReconnectAndRetry();
    }

    autoConnectControllers.clear();

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

template <typename T>
concept HasSocketClientConfig = requires {
    typename T::Config;
    typename T::SocketAddress;
};

template <typename T>
concept StreamSocketClient = HasSocketClientConfig<T> && std::derived_from<T, core::socket::Socket<typename T::Config>>;

template <template <typename SocketContextFactory, typename... Args> typename SocketClientT>
    requires StreamSocketClient<SocketClientT<mqtt::bridge::SocketContextFactory>>
SocketClientT<mqtt::bridge::SocketContextFactory>
startClient(const std::string& instanceName,
            const std::function<void(typename SocketClientT<mqtt::bridge::SocketContextFactory>::Config&)>& configurator) {
    using Client = SocketClientT<mqtt::bridge::SocketContextFactory>;
    using SocketAddress = typename Client::SocketAddress;

    Client socketClient = core::socket::stream::Client<Client>(instanceName, configurator);

    socketClient.getConfig().ConfigInstance::configurable(false);
    socketClient.getConfig().Remote::configurable(false);
    socketClient.getConfig().setRetry().setRetryBase(1);
    socketClient.getConfig().setReconnect();

    socketClient.getConfig().getDisabled();

    socketClient
        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
            addBridgeBrokerConnection(socketConnection);
        })
        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
            delBridgeBrokerConnection(socketConnection);
        })
        .setOnInitState([](core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
            handleConnector(connectEventReceiver);
        })
        .setOnAutoConnectControl([](std::shared_ptr<core::socket::stream::AutoConnectControl> autoConnectControl) {
            handleAutoConnectControllers(autoConnectControl);
        })
        .connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
            reportState(instanceName, socketAddress, state);
        });

    return socketClient;
}

template <typename HttpClient>
    requires requires {
        typename HttpClient::Config;
        typename HttpClient::SocketAddress;
    } &&
             std::constructible_from<HttpClient,
                                     const std::string&,
                                     std::function<void(const std::shared_ptr<web::http::client::MasterRequest>&)>&&,
                                     std::function<void(const std::shared_ptr<web::http::client::MasterRequest>&)>&&> &&
             requires(HttpClient& httpClient) {
                 { httpClient.getConfig() } -> std::same_as<typename HttpClient::Config&>;
                 httpClient.setOnConnected(std::declval<std::function<void(core::socket::stream::SocketConnection*)>>())
                     .setOnDisconnect(std::declval<std::function<void(core::socket::stream::SocketConnection*)>>())
                     .setOnInitState(std::declval<std::function<void(core::eventreceiver::ConnectEventReceiver*)>>())
                     .setOnAutoConnectControl(
                         std::declval<std::function<void(std::shared_ptr<core::socket::stream::AutoConnectControl>)>>())
                     .connect(std::declval<std::function<void(const typename HttpClient::SocketAddress&, const core::socket::State&)>>());
             }
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

    httpClient.getConfig().ConfigInstance::configurable(false);
    httpClient.getConfig().Remote::configurable(false);
    httpClient.getConfig().setRetry().setRetryBase(1);
    httpClient.getConfig().setReconnect();

    httpClient.getConfig().getDisabled();

    httpClient
        .setOnConnected([](core::socket::stream::SocketConnection* socketConnection) {
            addBridgeBrokerConnection(socketConnection);
        })
        .setOnDisconnect([](core::socket::stream::SocketConnection* socketConnection) {
            delBridgeBrokerConnection(socketConnection);
        })
        .setOnInitState([](core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
            handleConnector(connectEventReceiver);
        })
        .setOnAutoConnectControl([](std::shared_ptr<core::socket::stream::AutoConnectControl> autoConnectControl) {
            handleAutoConnectControllers(autoConnectControl);
        })
        .connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
            reportState(name, socketAddress, state);
        });
}

static void startBridges() {
    mqtt::bridge::lib::SSEDistributor::instance().bridgesStarting();

    for (const auto& [bridgeName, bridge] : mqtt::bridge::lib::BridgeStore::instance().getBridgeMap()) {
        VLOG(0) << "Starting bridge: " << bridgeName;

        if (!bridge.getDisabled()) {
            mqtt::bridge::lib::SSEDistributor::instance().bridgeStarting(bridgeName);

            for (const auto& [fullInstanceName, broker] : bridge.getBrokerMap()) {
                if (!broker.getDisabled()) {
                    mqtt::bridge::lib::SSEDistributor::instance().brokerConnecting(bridgeName, fullInstanceName);

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
                    VLOG(1) << "    Bridge disabled: " << bridge.getDisabled();
                    VLOG(1) << "    Bridge prefix: " << bridge.getPrefix();
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
                                startClient<net::in::stream::legacy::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.setDisableNagleAlgorithm();

                                    config.Remote::setHost(broker.getAddress()["host"]);
                                    config.Remote::setPort(broker.getAddress()["port"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                                });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV4
                            } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4)
                                startClient<net::in::stream::tls::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.setDisableNagleAlgorithm();

                                    config.Remote::setHost(broker.getAddress()["host"]);
                                    config.Remote::setPort(broker.getAddress()["port"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                                });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV4
                            }
                        } else if (protocol == "in6") {
                            if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6)
                                startClient<net::in6::stream::legacy::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.setDisableNagleAlgorithm();

                                    config.Remote::setHost(broker.getAddress()["host"]);
                                    config.Remote::setPort(broker.getAddress()["port"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                                });
#else  // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TCP_IPV6
                            } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6)
                                startClient<net::in6::stream::tls::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.setDisableNagleAlgorithm();

                                    config.Remote::setHost(broker.getAddress()["host"]);
                                    config.Remote::setPort(broker.getAddress()["port"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                                });
#else  // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_TLS_IPV6
                            }
                        } else if (protocol == "un") {
                            if (encryption == "legacy") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX)
                                startClient<net::un::stream::legacy::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.Remote::setSunPath(broker.getAddress()["host"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
                                });
#else  // CONFIG_MQTTSUITE_BRIDGE_UNIX
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // CONFIG_MQTTSUITE_BRIDGE_UNIX
                            } else if (encryption == "tls") {
#if defined(CONFIG_MQTTSUITE_BRIDGE_UNIX_TLS)
                                startClient<net::un::stream::tls::SocketClient>(fullInstanceName, [&broker](auto& config) {
                                    config.Remote::setSunPath(broker.getAddress()["host"]);

                                    config.setDisabled(broker.getDisabled() || broker.getBridge().getDisabled());
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
                } else {
                    mqtt::bridge::lib::SSEDistributor::instance().brokerDisabled(bridgeName, fullInstanceName);
                }
            }
        } else {
            mqtt::bridge::lib::SSEDistributor::instance().bridgeDisabled(bridgeName);
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

    int a = 3;

    CLI::App* bridgeApp =
        utils::Config::addInstance(net::config::Instance("bridge", "Configuration for Application mqttbridge", &a), "Applications");
    utils::Config::required(bridgeApp);

    std::string bridgeDefinitionFile; // = "<REQUIRED>";

    bridgeApp->needs(bridgeApp->add_option("--definition", bridgeDefinitionFile, "MQTT bridge definition file (JSON format)")
                         ->check(CLI::ExistingFile)
                         ->capture_default_str()
                         ->group(bridgeApp->get_formatter()->get_label("Persistent Options"))
                         ->type_name("path")
                         ->configurable()
                         ->required());

    std::string htmlDir = std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttbridge";

    bridgeApp->add_option("--html-dir", htmlDir, "Path to html source directory")
        ->check(CLI::ExistingDirectory)
        ->capture_default_str()
        ->group(bridgeApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("path")
        ->configurable();

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

                        tryRestartBridges();
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
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set("Content-Type", "text/event-stream") //
                .set("Cache-Control", "no-cache")
                .set("Connection", "keep-alive");
            res->sendHeader();

            std::string data{"data"};
            mqtt::bridge::lib::SSEDistributor::instance().addEventReceiver(res, req->get("Last-Event-ID"));
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/", [] APPLICATION(req, res) {
        res->redirect("/config");
    });

    router.get("/config", [] APPLICATION(req, res) {
        res->redirect("/config/index.html");
    });

    router.use("/config", express::middleware::StaticMiddleware(htmlDir));

    router.get("*", [] APPLICATION(req, res) {
        res->redirect("/config/index.html");
    });

    express::legacy::in::Server("admin-legacy", router, reportState, [](auto& config) {
        config.setPort(8081);
        config.setRetry();
        config.setReuseAddress();
    });

    express::tls::in::Server("admin-tls", router, reportState, [](auto& config) {
        config.setPort(8082);
        config.setRetry();
        config.setReuseAddress();
    });

    if (mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(bridgeDefinitionFile)) {
        startBridges();
    } else {
        VLOG(1) << "Loading bridge definition file failed";
    }

    return core::SNodeC::start();
}

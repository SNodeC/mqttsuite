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
#include "lib/BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <core/socket/State.h>                       // IWYU pragma: keep
#include <core/socket/stream/SocketContextFactory.h> // IWYU pragma: keep
#include <iot/mqtt/Topic.h>
#include <log/Logger.h>
#include <utils/Config.h>

namespace core::socket {
    class SocketAddress; // IWYU pragma: keep
}

// This can be and is done in CMakeLists.txt
// =========================================
#if !defined(MQTTBRIDGE_IN_STREAM_LEGACY)
#define MQTTBRIDGE_IN_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
#define MQTTBRIDGE_IN6_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_L2_STREAM_LEGACY)
#define MQTTBRIDGE_L2_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_RC_STREAM_LEGACY)
#define MQTTBRIDGE_RC_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_UN_STREAM_LEGACY)
#define MQTTBRIDGE_UN_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN_STREAM_TLS)
#define MQTTBRIDGE_IN_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_IN6_STREAM_TLS)
#define MQTTBRIDGE_IN6_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_L2_STREAM_TLS)
#define MQTTBRIDGE_L2_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_RC_STREAM_TLS)
#define MQTTBRIDGE_RC_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_UN_STREAM_TLS)
#define MQTTBRIDGE_UN_STREAM_TLS
#endif

// Select necessary include files
// ==============================
#if defined(MQTTBRIDGE_IN_STREAM_LEGACY)
#include <net/in/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN_STREAM_TLS)
#include <net/in/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
#include <net/in6/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN6_STREAM_TLS)
#include <net/in6/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_L2_STREAM_LEGACY)
#include <net/l2/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_L2_STREAM_TLS)
#include <net/l2/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_RC_STREAM_LEGACY)
#include <net/rc/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_RC_STREAM_TLS)
#include <net/rc/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_UN_STREAM_LEGACY)
#include <net/un/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_UN_STREAM_TLS)
#include <net/un/stream/tls/SocketClient.h>
#endif

#include <cstdint>
#include <functional> // IWYU pragma: keep
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

// IWYU pragma: no_include <bits/utility.h>

#endif

template <typename SocketAddress, typename = std::enable_if_t<std::is_base_of_v<core::socket::SocketAddress, SocketAddress>>>
void reportState(const std::string& instanceName, const SocketAddress& socketAddress, const core::socket::State& state) {
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

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startClient(const std::string& instanceName, const auto& configurator, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Client::SocketAddress;

    Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(client.getConfig());

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketClient<SocketContextFactory>::Config&>>>
void startClient(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Client::SocketAddress;

    Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--bridge-config", "MQTT bridge configuration file (JSON format)", "[path]");

    core::SNodeC::init(argc, argv);

    const bool success =
        mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(utils::Config::get_string_option_value("--bridge-config"));

    if (success) {
        for (const auto& [instanceName, broker] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
            if (!broker.getInstanceName().empty()) {
                VLOG(1) << "  Creating Broker instance: " << instanceName;
                VLOG(1) << "    Bridge client id : " << broker.getBridge().getClientId();
                VLOG(1) << "    Transport: " << broker.getTransport();
                VLOG(1) << "    Protocol: " << broker.getProtocol();
                VLOG(1) << "    Encryption: " << broker.getEncryption();

                VLOG(1) << "    Topics:";
                const std::list<iot::mqtt::Topic>& topics = broker.getTopics();
                for (const iot::mqtt::Topic& topic : topics) {
                    VLOG(1) << "      " << static_cast<uint16_t>(topic.getQoS()) << ":" << topic.getName();
                }

                const std::string& transport = broker.getTransport();
                const std::string& protocol = broker.getProtocol();
                const std::string& encryption = broker.getEncryption();

                if (transport == "stream") {
                    if (protocol == "in") {
                        if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN_STREAM_LEGACY)
                            startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_IN_STREAM_LEGACY
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_IN_STREAM_LEGACY
                        } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN_STREAM_TLS)
                            startClient<net::in::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_IN_STREAM_TLS
                            VLOG(1)
                            "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                              << "' not supported.";
#endif // MQTTBRIDGE_IN_STREAM_TLS
                        }
                    } else if (protocol == "in6") {
                        if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
                            startClient<net::in6::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_IN6_STREAM_LEGACY
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_IN6_STREAM_LEGACY
                        } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN6_STREAM_TLS)
                            startClient<net::in6::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_IN6_STREAM_TLS
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_IN6_STREAM_TLS
                        }
                    } else if (protocol == "l2") {
                        if (encryption == "legacy") {
#if defined(MQTTBRIDGE_L2_STREAM_LEGACY)
                            startClient<net::l2::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_L2_STREAM_LEGACY
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_L2_STREAM_LEGACY
                        } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_L2_STREAM_TLS)
                            startClient<net::l2::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_L2_STREAM_TLS
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_L2_STREAM_TLS
                        }
                    } else if (protocol == "rc") {
                        if (encryption == "legacy") {
#if defined(MQTTBRIDGE_RC_STREAM_LEGACY)
                            startClient<net::rc::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_RC_STREAM_LEGACY
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_RC_STREAM_LEGACY
                        } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_RC_STREAM_TLS)
                            startClient<net::rc::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_RC_STREAM_TLS
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_RC_STREAM_TLS
                        }
                    } else if (protocol == "un") {
                        if (encryption == "legacy") {
#if defined(MQTTBRIDGE_UN_STREAM_LEGACY)
                            startClient<net::un::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_UN_STREAM_LEGACY
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_UN_STREAM_LEGACY
                        } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_UN_STREAM_TLS)
                            startClient<net::un::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                instanceName,
                                [](auto& config) -> void {
                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                },
                                broker.getBridge(),
                                broker.getTopics());
#else  // MQTTBRIDGE_UN_STREAM_TLS
                            VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                    << "' not supported.";
#endif // MQTTBRIDGE_UN_STREAM_TLS
                        }
                    }
                } else {
                    VLOG(1) << "    Transport '" << transport << "' not supported.";
                }
            } else {
                VLOG(1) << "Instance not created: Instance name empty.";
            }
        }
    }

    return core::SNodeC::start();
}

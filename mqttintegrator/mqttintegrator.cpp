/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Fre
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

#include "SocketContextFactory.h"

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
//
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/tls/SocketClient.h>
#include <net/in6/stream/legacy/SocketClient.h>
#include <net/in6/stream/tls/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <net/un/stream/tls/SocketClient.h>
#include <web/http/legacy/in/Client.h>
#include <web/http/legacy/in6/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#endif

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

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
          typename SocketAddress = typename Client::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
static void startClient(const std::string& instanceName,
                        const std::function<void(typename Client::Config&)>& configurator,
                        SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    const Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(client.getConfig());

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>,
          typename SocketAddress = typename Client::SocketAddress,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketClient<SocketContextFactory>::Config&>>>
static void startClient(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    const Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

template <typename HttpClient>
static void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::Request>& req) {
            req->set("Sec-WebSocket-Protocol", "mqtt");

            if (!req->upgrade(
                    "/ws",
                    "websocket",
                    [](const std::shared_ptr<web::http::client::Request>& req, const std::shared_ptr<web::http::client::Response>& res) {
                        req->upgrade(res, [subProtocolsRequested = req->header("Upgrade")](const std::string& name) {
                            if (!name.empty()) {
                                VLOG(1) << "Successful upgrade to '" << name << "' requested: " << subProtocolsRequested;
                            } else {
                                VLOG(1) << "Can not upgrade to any of '" << subProtocolsRequested << "'";
                            }
                        });
                    },
                    [](const std::shared_ptr<web::http::client::Request>& req, const std::string& reason) {
                        VLOG(1) << "Upgrade to subprotocols '" << req->header("Upgrade")
                                << "' failed with response parse error: " << reason;
                    })) {
                VLOG(1) << "Initiating upgrade to any of 'upgradeprotocol, websocket' failed";
            }
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            VLOG(1) << "Session ended";
        });

    configurator(httpClient.getConfig());

    httpClient.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(name, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    utils::Config::addStringOption("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]");
    utils::Config::addStringOption("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    std::string sessionStoreFileName = utils::Config::getStringOptionValue("--mqtt-session-store");

    startClient<net::in::stream::legacy::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "in-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);

    startClient<net::in::stream::tls::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "in-mqtts",
        [](auto& config) {
            config.Remote::setPort(8883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);

    startClient<net::in6::stream::legacy::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "in6-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);

    startClient<net::in6::stream::tls::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "in6-mqtts",
        [](auto& config) {
            config.Remote::setPort(8883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);

    startClient<net::un::stream::legacy::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "un-mqtt",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtt");

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
        },
        sessionStoreFileName);

    startClient<net::un::stream::tls::SocketClient, mqtt::mqttintegrator::SocketContextFactory>(
        "un-mqtts",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtts");

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
        },
        sessionStoreFileName);

    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });

    startClient<web::http::tls::in::Client>("in-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });

    startClient<web::http::legacy::in6::Client>("in6-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });

    startClient<web::http::tls::in6::Client>("in6-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });

    return core::SNodeC::start();
}

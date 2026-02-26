/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
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
#include "lib/ConfigApplication.h"
#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

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
#include <web/http/legacy/un/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
#include <web/http/tls/un/Client.h>
//
#include <express/legacy/in/Server.h>
#include <express/tls/in/Server.h>
//
#include <net/config/ConfigInstanceAPI.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>
//
#include <string>
#include <utility>

#endif

// admin API
#include "lib/MappingAdminRouter.h"
#include "lib/Mqtt.h"

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

template <template <typename SocketContextFactoryT, typename... ArgsT> typename SocketClientT, typename... Args>
SocketClientT<mqtt::mqttintegrator::SocketContextFactory, Args...>
startClient(const std::string& instanceName,
            const std::function<void(typename SocketClientT<mqtt::mqttintegrator::SocketContextFactory>::Config&)>& configurator,
            Args&&... args) {
    using Client = SocketClientT<mqtt::mqttintegrator::SocketContextFactory, Args...>;
    using SocketAddress = typename Client::SocketAddress;

    Client socketClient = core::socket::stream::Client<Client>(instanceName, configurator, std::forward<Args>(args)...);

    socketClient.getConfig().setRetry();
    socketClient.getConfig().setRetryBase(1);
    socketClient.getConfig().setReconnect();

    socketClient.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });

    return socketClient;
}

template <typename HttpClient>
void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator = nullptr) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
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
                [connectionName](const std::shared_ptr<web::http::client::Request>&, const std::string& message) {
                    VLOG(1) << connectionName << ": Request parse error: " << message;
                });
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            VLOG(1) << "Session ended";
        });

    if (configurator != nullptr) {
        configurator(httpClient.getConfig());
    }

    httpClient.getConfig().setRetry();
    httpClient.getConfig().setRetryBase(1);
    httpClient.getConfig().setReconnect();

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

    // /home/voc/projects/mqttsuite/mqttsuite/mapfile.json
    utils::Config::addInstance<mqtt::lib::ConfigMqttIntegrator>();

    core::SNodeC::init(argc, argv);

    // Instanciate Admin Router for Mapping Management
    express::Router router = mqtt::lib::admin::makeMappingAdminRouter(
        utils::Config::getInstance<mqtt::lib::ConfigMqttIntegrator>()->getMappingFile(), mqtt::lib::admin::AdminOptions{}, []() {
            mqtt::mqttintegrator::lib::Mqtt::reloadAll();
        });

    express::legacy::in::Server("in-http", router, reportState, [](net::in::stream::legacy::config::ConfigSocketServer& config) {
        config.setPort(8085);
        config.setRetry();
    });

    express::tls::in::Server("in-https", router, reportState, [](net::in::stream::tls::config::ConfigSocketServer& config) {
        config.setPort(8086);
        config.setRetry();
    });

    std::string sessionStoreFileName = utils::Config::getInstance<mqtt::lib::ConfigMqttIntegrator>()->getSessionStore();

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4)
    startClient<net::in::stream::legacy::SocketClient>( //
        "in-mqtt",
        [](net::in::stream::legacy::config::ConfigSocketClient& config) {
            config.Remote::setPort(1883);

            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);
#endif // CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4)
    startClient<net::in::stream::tls::SocketClient>( //
        "in-mqtts",
        [](net::in::stream::tls::config::ConfigSocketClient& config) {
            config.Remote::setPort(1883);
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6)
    startClient<net::in6::stream::legacy::SocketClient>( //
        "in6-mqtt",
        [](net::in6::stream::legacy::config::ConfigSocketClient& config) {
            config.Remote::setPort(1883);
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6)
    startClient<net::in6::stream::tls::SocketClient>( //
        "in6-mqtts",
        [](net::in6::stream::tls::config::ConfigSocketClient& config) {
            config.Remote::setPort(1883);
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName);
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX)
    startClient<net::un::stream::legacy::SocketClient>( //
        "un-mqtt",
        []([[maybe_unused]] net::un::stream::legacy::config::ConfigSocketClient& config) {
        },
        sessionStoreFileName);
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS)
    startClient<net::un::stream::tls::SocketClient>( //
        "un-mqtts",
        []([[maybe_unused]] net::un::stream::tls::config::ConfigSocketClient& config) {
        },
        sessionStoreFileName);
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in::Client>( //
        "in-wsmqtt",
        [](net::in::stream::legacy::config::ConfigSocketClient& config) {
            config.Remote::setPort(8080);
            config.setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in::Client>( //
        "in-wsmqtts",
        [](net::in::stream::tls::config::ConfigSocketClient& config) {
            config.Remote::setPort(8088);
            config.setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in6::Client>( //
        "in6-wsmqtt",
        [](net::in6::stream::legacy::config::ConfigSocketClient& config) {
            config.Remote::setPort(8080);
            config.setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in6::Client>( //
        "in6-wsmqtts",
        [](net::in6::stream::tls::config::ConfigSocketClient& config) {
            config.Remote::setPort(8088);
            config.setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::un::Client>( //
        "un-wsmqtt",
        []([[maybe_unused]] net::un::stream::legacy::config::ConfigSocketClient& config) {
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::un::Client>( //
        "un-wsmqtts",
        []([[maybe_unused]] net::un::stream::tls::config::ConfigSocketClient& config) {
        });
#endif

    return core::SNodeC::start();
}

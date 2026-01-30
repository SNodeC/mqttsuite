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
#include <web/http/legacy/un/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
#include <web/http/tls/un/Client.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <string>

#endif

// admin API
#include "express/legacy/in/Server.h"
#include "lib/JsonMappingReader.h"
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

    const std::string mappingPath = utils::Config::getStringOptionValue("--mqtt-mapping-file");

    // Instanciate Admin Router for Mapping Management
    express::Router router = mqtt::lib::admin::makeMappingAdminRouter(mappingPath, mqtt::lib::admin::AdminOptions{}, []() {
        mqtt::mqttintegrator::lib::Mqtt::reloadAll();
    });

    express::legacy::in::Server("in-https", router, reportState, [](auto& config) {
        config.setPort(8085);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    std::string sessionStoreFileName = utils::Config::getStringOptionValue("--mqtt-session-store");

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4)
    net::in::stream::legacy::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "in-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in-mqtt", socketAddress, state);
        });
#endif // CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4)
    net::in::stream::tls::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "in-mqtts",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in-mqtts", socketAddress, state);
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6)
    net::in6::stream::legacy::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "in6-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in6-mqtt", socketAddress, state);
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6)
    net::in6::stream::tls::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "in6-mqtts",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
            config.setDisableNagleAlgorithm();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in6-mqtts", socketAddress, state);
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX)
    net::un::stream::legacy::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "un-mqtt",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtt");

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("un-mqtt", socketAddress, state);
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS)
    net::un::stream::tls::Client<mqtt::mqttintegrator::SocketContextFactory>(
        "un-mqtts",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtt");

            config.setRetry();
            config.setRetryBase(1);
            config.setReconnect();
        },
        sessionStoreFileName)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("un-mqtts", socketAddress, state);
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in::Client>("in-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in6::Client>("in6-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in6::Client>("in6-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::un::Client>("un-wsmqtt", [](auto& config) {
        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::un::Client>("un-wsmqtts", [](auto& config) {
        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });
#endif

    return core::SNodeC::start();
}

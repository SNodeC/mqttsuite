/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "SocketContextFactory.h"
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
#include <utils/Config.h>
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
#include <express/legacy/in/Server.h>
#include <express/tls/in/Server.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include "lib/SemanticLog.h"
#include <utility>
#endif

#include "lib/MappingAdminRouter.h"
#include "lib/Mqtt.h"

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            mqttsuite::semantic::integratorLog().debug() << instanceName << ": connected to '" << socketAddress.toString() << "'";
            break;
        case core::socket::State::DISABLED:
            mqttsuite::semantic::integratorLog().debug() << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
        case core::socket::State::FATAL:
            mqttsuite::semantic::integratorLog().debug() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

template <template <typename SocketContextFactoryT, typename... ArgsT> typename SocketClientT, typename... Args>
static SocketClientT<mqtt::mqttintegrator::SocketContextFactory, Args...>
startClient(const std::string& instanceName,
            const std::function<void(typename SocketClientT<mqtt::mqttintegrator::SocketContextFactory>::Config*)>& configurator,
            Args&&... args) {
    using Client = SocketClientT<mqtt::mqttintegrator::SocketContextFactory, Args...>;
    using SocketAddress = typename Client::SocketAddress;

    Client socketClient = core::socket::stream::Client<Client>(instanceName, configurator, std::forward<Args>(args)...);

    socketClient.getConfig()->setRetry();
    socketClient.getConfig()->setRetryBase(1);
    socketClient.getConfig()->setReconnect();

    socketClient.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });

    return socketClient;
}

template <typename HttpClient>
HttpClient startClient(const std::string& name, const std::function<void(typename HttpClient::Config*)>& configurator = nullptr) {
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
                    mqttsuite::semantic::integratorLog().debug() << connectionName << ": HTTP Upgrade (http -> websocket||mqtt) start "
                                                                 << (success ? "success" : "failed");
                },
                []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req,
                   [[maybe_unused]] const std::shared_ptr<web::http::client::Response>& res,
                   [[maybe_unused]] bool success) {
                },
                [connectionName](const std::shared_ptr<web::http::client::Request>&, const std::string& message) {
                    mqttsuite::semantic::integratorLog().debug() << connectionName << ": Request parse error: " << message;
                });
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            mqttsuite::semantic::integratorLog().debug() << "Session ended";
        });

    if (configurator != nullptr) {
        configurator(httpClient.getConfig());
    }

    httpClient.getConfig()->setRetry();
    httpClient.getConfig()->setRetryBase(1);
    httpClient.getConfig()->setReconnect();

    httpClient.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(name, socketAddress, state);
    });

    return httpClient;
}

int main(int argc, char* argv[]) {
    mqtt::lib::ConfigMqttIntegrator* configMqttIntegrator = utils::Config::configRoot.newSubCommand<mqtt::lib::ConfigMqttIntegrator>();

    // Mapping has exactly one production source: --mqtt-mapping-file (directly or
    // through the SNode.C configuration file). With no selection the mapper
    // remains in its explicit empty/no-op state; no demo mapping is substituted.
    core::SNodeC::init(argc, argv);

    if (configMqttIntegrator->hasAdminAuthentication() && !configMqttIntegrator->getMappingFilename().empty()) {
        const mqtt::lib::admin::AdminOptions adminOptions{configMqttIntegrator->getAdminUser(),
                                                          configMqttIntegrator->getAdminPassword(),
                                                          configMqttIntegrator->getAdminRealm()};

        express::Router router = mqtt::lib::admin::makeMappingAdminRouter(configMqttIntegrator, adminOptions, [](bool mustReconnect) {
            return mqtt::mqttintegrator::lib::Mqtt::updateSubscriptions(mustReconnect);
        });

        express::legacy::in::Server("in-http", router, reportState, [](net::in::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(8085);
            config->setRetry();
        });

        express::tls::in::Server("in-https", router, reportState, [](net::in::stream::tls::config::ConfigSocketServer* config) {
            config->setPort(8086);
            config->setRetry();
        });
    } else {
        mqttsuite::semantic::integratorLog().info()
            << "Mapping administration disabled: configure --admin-user, --admin-password-file, and --mqtt-mapping-file to enable it";
    }

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4)
    startClient<net::in::stream::legacy::SocketClient>(
        "in-mqtt",
        [](net::in::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4)
    startClient<net::in::stream::tls::SocketClient>(
        "in-mqtts",
        [](net::in::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6)
    startClient<net::in6::stream::legacy::SocketClient>(
        "in6-mqtt",
        [](net::in6::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6)
    startClient<net::in6::stream::tls::SocketClient>(
        "in6-mqtts",
        [](net::in6::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();
        });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX)
    startClient<net::un::stream::legacy::SocketClient>("un-mqtt", []([[maybe_unused]] const net::un::stream::legacy::config::ConfigSocketClient*) {});
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS)
    startClient<net::un::stream::tls::SocketClient>("un-mqtts", []([[maybe_unused]] const net::un::stream::tls::config::ConfigSocketClient*) {});
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](net::in::stream::legacy::config::ConfigSocketClient* config) {
        config->Remote::setPort(8080);
        config->setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in::Client>("in-wsmqtts", [](net::in::stream::tls::config::ConfigSocketClient* config) {
        config->Remote::setPort(8088);
        config->setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::in6::Client>("in6-wsmqtt", [](net::in6::stream::legacy::config::ConfigSocketClient* config) {
        config->Remote::setPort(8080);
        config->setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::in6::Client>("in6-wsmqtts", [](net::in6::stream::tls::config::ConfigSocketClient* config) {
        config->Remote::setPort(8088);
        config->setDisableNagleAlgorithm();
    });
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WS)
    startClient<web::http::legacy::un::Client>("un-wsmqtt", []([[maybe_unused]] const net::un::stream::legacy::config::ConfigSocketClient*) {});
#endif

#if defined(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS) && defined(CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    startClient<web::http::tls::un::Client>("un-wsmqtts", []([[maybe_unused]] const net::un::stream::tls::config::ConfigSocketClient*) {});
#endif

    return core::SNodeC::start();
}

/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#include "SocketContextFactory.h"
#include "config.h"
#include "lib/ConfigSections.h"
#include "lib/DashboardModel.h"

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

#include <core/SNodeC.h>
#include <express/middleware/StaticMiddleware.h>
#include <net/config/ConfigInstance.h>
//
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/tls/SocketClient.h>
#include <net/in6/stream/legacy/SocketClient.h>
#include <net/in6/stream/tls/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <net/un/stream/tls/SocketClient.h>
#include <web/http/client/ConfigHTTP.h>
#include <web/http/http_utils.h>
#include <web/http/legacy/in/Client.h>
#include <web/http/legacy/in6/Client.h>
#include <web/http/legacy/un/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
#include <web/http/tls/un/Client.h>

#ifdef CONFIG_MQTTSUITE_CLI_TCP_IPV4
#include <express/legacy/in/Server.h>
#endif
#ifdef CONFIG_MQTTSUITE_CLI_TCP_IPV6
#include <express/legacy/in6/Server.h>
#endif
#ifdef CONFIG_MQTTSUITE_CLI_UNIX
#include <express/legacy/un/Server.h>
#endif
#ifdef CONFIG_MQTTSUITE_CLI_TLS_IPV4
#include <express/tls/in/Server.h>
#endif
#ifdef CONFIG_MQTTSUITE_CLI_TLS_IPV6
#include <express/tls/in6/Server.h>
#endif
#ifdef CONFIG_MQTTSUITE_CLI_UNIX_TLS
#include <express/tls/un/Server.h>
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>
//
#include <list>
#include <string>

#endif

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected/listening on '" << socketAddress.toString() << "'";
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

static express::Router getDashboardRouter(const std::string& webRoot) {
    const express::Router router;

    router.get("/api/water-buoy/events", [] APPLICATION(req, res) {
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set({{"Content-Type", "text/event-stream"},
                      {"Cache-Control", "no-cache"},
                      {"Connection", "keep-alive"},
                      {"Access-Control-Allow-Origin", "*"}});

            res->sendHeader();

            mqtt::mqttcli::lib::DashboardModel::instance().addEventReceiver(res, req->get("Last-Event-ID"));
        } else {
            res->redirect("/dashboard/index.html");
        }
    });

    router.get("/sse", [] APPLICATION(req, res) {
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set("Content-Type", "text/event-stream")
                .set("Cache-Control", "no-cache")
                .set("Connection", "keep-alive");
            res->sendHeader();

            mqtt::mqttcli::lib::DashboardModel::instance().addEventReceiver(res, req->get("Last-Event-ID"));
        } else {
            res->redirect("/dashboard/index.html");
        }
    });

    router.get("/", [] APPLICATION(req, res) {
        res->redirect("/dashboard/index.html");
    });

    router.use("/dashboard", express::middleware::StaticMiddleware(webRoot));

    router.get("*", [] APPLICATION(req, res) {
        res->redirect("/dashboard/index.html");
    });

    return router;
}

static void logResponse(const std::shared_ptr<web::http::client::Request>& req, const std::shared_ptr<web::http::client::Response>& res) {
    VLOG(1) << req->getConnectionName() << " HTTP response for: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "."
            << req->httpMinor << "\n"
            << httputils::toString(req->method,
                                   req->url,
                                   "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                   req->getQueries(),
                                   req->getHeaders(),
                                   req->getTrailer(),
                                   req->getCookies(),
                                   {})
            << "\n"
            << httputils::toString(res->httpVersion, res->statusCode, res->reason, res->headers, res->cookies, res->body);
}

template <template <typename SocketContextFactoryT, typename... ArgsT> typename SocketClient>
static SocketClient<mqtt::mqttcli::SocketContextFactory>
startClient(const std::string& instanceName,
            const std::function<void(typename SocketClient<mqtt::mqttcli::SocketContextFactory>::Config*)>& configurator) {
    using Client = SocketClient<mqtt::mqttcli::SocketContextFactory>;
    using SocketAddress = typename Client::SocketAddress;

    Client socketClient = core::socket::stream::Client<Client>(instanceName, configurator);

    socketClient.getConfig()->setRetry();
    socketClient.getConfig()->setRetryBase(1);
    socketClient.getConfig()->setReconnect();
    socketClient.getConfig()->setDisabled();

    socketClient.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });

    return socketClient;
}

template <typename HttpClient>
static HttpClient startClient(const std::string& name, const std::function<void(typename HttpClient::Config*)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::MasterRequest>& req) {
            const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();
            const std::string target = req->getSocketContext()
                                           ->getSocketConnection()
                                           ->getConfigInstance()
                                           ->getSubCommand<web::http::client::ConfigHTTP>()
                                           ->getOption("--target")
                                           ->as<std::string>();

            req->set("Sec-WebSocket-Protocol", "mqtt");

            req->upgrade(
                target,
                "websocket",
                [connectionName](bool success) {
                    VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket||"
                            << "mqtt" << ") start " << (success ? "success" : "failed");
                },
                [connectionName](const std::shared_ptr<web::http::client::Request>& req,
                                 const std::shared_ptr<web::http::client::Response>& res,
                                 bool success) {
                    logResponse(req, res);

                    VLOG(1) << connectionName << ": HTTP Upgrade " << (success ? "success" : "failed");
                },
                [connectionName](const std::shared_ptr<web::http::client::Request>& req, const std::string& message) {
                    VLOG(1) << connectionName << ": Response parse error: " << message;
                    VLOG(1) << "  Request was: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "." << req->httpMinor
                            << "\n"
                            << httputils::toString(req->method,
                                                   req->url,
                                                   "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                                   req->getQueries(),
                                                   req->getHeaders(),
                                                   req->getTrailer(),
                                                   req->getCookies(),
                                                   {})
                            << "\n";
                });
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            VLOG(1) << "Session ended";
        });

    configurator(httpClient.getConfig());

    httpClient.getConfig()->setRetry();
    httpClient.getConfig()->setRetryBase(1);
    httpClient.getConfig()->setReconnect();
    httpClient.getConfig()->setDisabled();

    httpClient.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(name, socketAddress, state);
    });

    return httpClient;
}

static void createConfig(net::config::ConfigInstance* config) {
    config->newSubCommand<mqtt::mqttcli::lib::ConfigSession>();
    config->newSubCommand<mqtt::mqttcli::lib::ConfigSubscribe>();
    config->newSubCommand<mqtt::mqttcli::lib::ConfigPublish>();
    config->newSubCommand<mqtt::mqttcli::lib::ConfigDatabase>();

    config->setRequireCallback([config]() {
        if (!config->getDisabled() && config->getShowConfigTriggerApp() == nullptr &&
            config->getParent()->getOption("--write-config")->count() == 0) {
            const mqtt::mqttcli::lib::ConfigPublish* pubApp = config->getSubCommand<mqtt::mqttcli::lib::ConfigPublish>();
            const mqtt::mqttcli::lib::ConfigSubscribe* subApp = config->getSubCommand<mqtt::mqttcli::lib::ConfigSubscribe>();

            if (pubApp->getTopic().empty() && subApp->getTopic().empty()) {
                throw CLI::RequiresError(config->getParent()->getName() + ":" + config->getInstanceName() +
                                             " requires at least one of {sub | pub}",
                                         CLI::ExitCodes::RequiresError);
            }

            if (!pubApp->getTopic().empty()) {
                VLOG(0) << "[" << Color::Code::FG_LIGHT_GREEN << "Success" << Color::Code::FG_DEFAULT << "] " << "Bootstrap of "
                        << config->getInstanceName() << ":pub";
            }

            if (!subApp->getTopic().empty()) {
                VLOG(0) << "[" << Color::Code::FG_LIGHT_GREEN << "Success" << Color::Code::FG_DEFAULT << "] " << "Bootstrap of "
                        << config->getInstanceName() << ":sub";
            }
        }
    });
}

static void createWSConfig(net::config::ConfigInstance* config) {
    createConfig(config);

    config->getSubCommand<web::http::client::ConfigHTTP>()
        ->addOption("--target", "Websocket endpoint", "string", "/ws", CLI::TypeValidator<std::string>())
        ->configurable();
}

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV4)
    startClient<net::in::stream::legacy::SocketClient>(
        "in-mqtt",
        [](net::in::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();

            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV4)
    startClient<net::in::stream::tls::SocketClient>(
        "in-mqtts",
        [](net::in::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();

            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV6)
    startClient<net::in6::stream::legacy::SocketClient>(
        "in6-mqtt",
        [](net::in6::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();

            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV6)
    startClient<net::in6::stream::tls::SocketClient>(
        "in6-mqtts",
        [](net::in6::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(1883);
            config->setDisableNagleAlgorithm();

            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX)
    startClient<net::un::stream::legacy::SocketClient>(
        "un-mqtt",
        [](net::un::stream::legacy::config::ConfigSocketClient* config) {
            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX_TLS)
    startClient<net::un::stream::tls::SocketClient>(
        "un-mqtts",
        [](net::un::stream::tls::config::ConfigSocketClient* config) {
            createConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV4) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::in::Client>(
        "in-wsmqtt",
        [](net::in::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(8080);
            config->setDisableNagleAlgorithm();

            createWSConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV4) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::in::Client>(
        "in-wsmqtts",
        [](net::in::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(8088);
            config->setDisableNagleAlgorithm();

            createWSConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV6) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::in6::Client>(
        "in6-wsmqtt",
        [](net::in6::stream::legacy::config::ConfigSocketClient* config) {
            config->Remote::setPort(8080);
            config->setDisableNagleAlgorithm();

            createWSConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV6) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::in6::Client>(
        "in6-wsmqtts",
        [](net::in6::stream::tls::config::ConfigSocketClient* config) {
            config->Remote::setPort(8088);
            config->setDisableNagleAlgorithm();

            createWSConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::un::Client>(
        "un-wsmqtt",
        [](net::un::stream::legacy::config::ConfigSocketClient* config) {
            createWSConfig(config);
        });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX_TLS) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::un::Client>(
        "un-wsmqtts",
        [](net::un::stream::tls::config::ConfigSocketClient* config) {
            createWSConfig(config);
        });
#endif

    const std::string htmlRoot = std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttcli";
    const express::Router router = getDashboardRouter(htmlRoot);

#ifdef CONFIG_MQTTSUITE_CLI_TCP_IPV4
    express::legacy::in::Server(
        "in-http",
        router,
        reportState,
        [](net::in::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(8080);
            config->setRetry();
            config->setDisableNagleAlgorithm();
        });
#endif

#ifdef CONFIG_MQTTSUITE_CLI_TCP_IPV6
    express::legacy::in6::Server(
        "in6-http",
        router,
        reportState,
        [](net::in6::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(8080);
            config->setRetry();
            config->setDisableNagleAlgorithm();
            config->setIPv6Only();
        });
#endif

#ifdef CONFIG_MQTTSUITE_CLI_UNIX
    express::legacy::un::Server(
        "un-http",
        router,
        reportState,
        [](net::un::stream::legacy::config::ConfigSocketServer* config) {
            config->setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config->getInstanceName());
        });
#endif

    return core::SNodeC::start();
}

/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "SocketContextFactory.h" // IWYU pragma: keep
#include "config.h"
#include "lib/ConfigApplication.h"
#include "lib/Mqtt.h"
#include "lib/MqttModel.h"

#include <core/SNodeC.h>
#include <utils/Config.h>
#include <express/middleware/BasicAuthentication.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/server/broker/Broker.h>

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
#include <express/legacy/in/Server.h>
#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
#include <express/tls/in/Server.h>
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV6
#include <express/legacy/in6/Server.h>
#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
#include <express/tls/in6/Server.h>
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX
#include <express/legacy/un/Server.h>
#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
#include <express/tls/un/Server.h>
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
#include <net/in/stream/legacy/SocketServer.h>
#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
#include <net/in/stream/tls/SocketServer.h>
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV6
#include <net/in6/stream/legacy/SocketServer.h>
#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
#include <net/in6/stream/tls/SocketServer.h>
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX
#include <net/un/stream/legacy/SocketServer.h>
#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
#include <net/un/stream/tls/SocketServer.h>
#endif
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <web/http/http_utils.h>
#include <nlohmann/json.hpp>
#include "lib/SemanticLog.h"
#include <utility>

#endif

static void upgrade APPLICATION(req, res) {
    const std::string connectionName = res->getSocketContext()->getSocketConnection()->getConnectionName();

    mqttsuite::semantic::brokerLog().trace() << connectionName << " HTTP: Upgrade request:\n"
                                             << httputils::toString(req->method,
                                                                    req->url,
                                                                    "HTTP/" + std::to_string(req->httpMajor) + "." +
                                                                        std::to_string(req->httpMinor),
                                                                    req->queries,
                                                                    req->headers,
                                                                    req->trailer,
                                                                    req->cookies,
                                                                    std::vector<char>());

    if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
        res->upgrade(req, [req, res, connectionName](const std::string& name) {
            if (!name.empty()) {
                mqttsuite::semantic::brokerLog().debug() << connectionName << ": Successful upgrade:";
                mqttsuite::semantic::brokerLog().debug() << connectionName << ":    Selected: " << name;
                mqttsuite::semantic::brokerLog().debug() << connectionName << ":   Requested: " << req->get("sec-websocket-protocol");
                res->end();
            } else {
                mqttsuite::semantic::brokerLog().debug() << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";
                res->sendStatus(404);
            }
        });
    } else {
        mqttsuite::semantic::brokerLog().debug() << connectionName << ": Unsupported subprotocol(s):";
        mqttsuite::semantic::brokerLog().debug() << "    Expected: mqtt";
        mqttsuite::semantic::brokerLog().debug() << "   Requested: " << req->get("sec-websocket-protocol");
        res->sendStatus(404);
    }
}

static express::Router getRouter(std::shared_ptr<iot::mqtt::server::broker::Broker> broker, mqtt::lib::ConfigMqttBroker* config) {
    const express::Router& jsonRouter = express::middleware::JsonMiddleware();
    const express::Router router;

    if (config->hasAdminAuthentication()) {
        const std::string user = config->getAdminUser();
        const std::string password = config->getAdminPassword();
        const std::string realm = config->getAdminRealm();

        jsonRouter.use("/api/mqtt", express::middleware::BasicAuthentication(user, password, realm));
        router.use("/sse", express::middleware::BasicAuthentication(user, password, realm));
        router.use("/clients", express::middleware::BasicAuthentication(user, password, realm));
    } else {
        auto unavailable = [] MIDDLEWARE(req, res, next) {
            static_cast<void>(req);
            static_cast<void>(next);
            res->sendStatus(401);
        };
        jsonRouter.use("/api/mqtt", unavailable);
        router.use("/sse", unavailable);
        router.use("/clients", unavailable);
    }

    // Same-origin administration is the default: no CORS response headers are
    // emitted here. Cross-origin administration is therefore not enabled by
    // the shipped configuration.
    jsonRouter.post("/api/mqtt/disconnect", [] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string clientId = json["clientId"].get<std::string>();
                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->getMqttContext()->getSocketConnection()->close();
                    res->send(R"({"success": true, "message": "Client disconnected successfully"})"_json.dump());
                } else {
                    res->status(404).send(R"({"success": false, "error": "Client not found"})"_json.dump());
                }
            },
            [&res](const std::string& key) {
                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/api/mqtt/unsubscribe", [] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string clientId = json["clientId"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();

                mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);
                if (mqtt != nullptr) {
                    mqtt->unsubscribe(topic);
                    res->send(R"({"success": true, "message": "Client unsubscribed successfully"})"_json.dump());
                } else {
                    res->status(404).send(R"({"success": false, "error": "Client not found"})"_json.dump());
                }
            },
            [&res](const std::string& key) {
                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/api/mqtt/release", [broker] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res, broker](nlohmann::json& json) {
                std::string topic = json["topic"].get<std::string>();

                broker->publish("", topic, "", 0, true);
                mqtt::mqttbroker::lib::MqttModel::instance().publishMessage(topic, "", 0, true);

                res->send(R"({"success": true, "message": "Retained message released successfully"})"_json.dump());
            },
            [&res](const std::string& key) {
                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/api/mqtt/subscribe", [] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string clientId = json["clientId"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();
                uint8_t qoS = json["qos"].get<uint8_t>();

                mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);
                if (mqtt != nullptr) {
                    mqtt->subscribe(topic, qoS);
                    res->send(R"({"success": true, "message": "Client subscribed successfully"})"_json.dump());
                } else {
                    res->status(404).send(R"({"success": false, "error": "Client not found"})"_json.dump());
                }
            },
            [&res](const std::string& key) {
                res->status(400).send("Attribute type not found: " + key);
            });
    });

    router.use(jsonRouter);

    router.get("/api/mqtt/events", [broker] APPLICATION(req, res) {
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set({{"Content-Type", "text/event-stream"}, {"Cache-Control", "no-cache"}, {"Connection", "keep-alive"}});
            res->sendHeader();
            mqtt::mqttbroker::lib::MqttModel::instance().addEventReceiver(res, req->get("Last-Event-ID"), broker);
        } else {
            res->redirect("/clients");
        }
    });

    // MQTT-over-WebSocket protocol routes intentionally remain outside the
    // HTTP administration authentication boundary.
    router.get("/ws", [] APPLICATION(req, res) {
        if (req->headers.contains("upgrade")) {
            upgrade(req, res);
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/mqtt", [] APPLICATION(req, res) {
        if (req->headers.contains("upgrade")) {
            upgrade(req, res);
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/", [] APPLICATION(req, res) {
        if (req->headers.contains("upgrade")) {
            upgrade(req, res);
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/sse", [broker] APPLICATION(req, res) {
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set("Content-Type", "text/event-stream").set("Cache-Control", "no-cache").set("Connection", "keep-alive");
            res->sendHeader();
            mqtt::mqttbroker::lib::MqttModel::instance().addEventReceiver(res, req->get("Last-Event-ID"), broker);
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/clients", [] APPLICATION(req, res) {
        res->redirect("/clients/index.html");
    });

    router.use("/clients", express::middleware::StaticMiddleware(config->getHtmlRoot()));

    router.get("*", [] APPLICATION(req, res) {
        res->redirect("/clients/index.html");
    });

    return router;
}

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            mqttsuite::semantic::brokerLog().debug() << instanceName << ": listening on '" << socketAddress.toString() << "'";
            break;
        case core::socket::State::DISABLED:
            mqttsuite::semantic::brokerLog().debug() << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
        case core::socket::State::FATAL:
            mqttsuite::semantic::brokerLog().debug() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

int main(int argc, char* argv[]) {
    mqtt::lib::ConfigMqttBroker* configMqttBroker =
        utils::Config::configRoot.newSubCommand<mqtt::lib::ConfigMqttBroker>();
    configMqttBroker->setHtmlRoot(std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttbroker");

    core::SNodeC::init(argc, argv);

    std::shared_ptr<iot::mqtt::server::broker::Broker> broker =
        iot::mqtt::server::broker::Broker::instance(SUBSCRIPTION_MAX_QOS, configMqttBroker->getSessionStore());

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
    net::in::stream::legacy::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in-mqtt",
        [](net::in::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(1883);
            config->setRetry();
            config->setDisableNagleAlgorithm();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
    net::in::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in-mqtts",
        [](net::in::stream::tls::config::ConfigSocketServer* config) {
            config->setPort(8883);
            config->setRetry();
            config->setDisableNagleAlgorithm();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in-mqtts", socketAddress, state);
        });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV6
    net::in6::stream::legacy::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in6-mqtt",
        [](net::in6::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(1883);
            config->setRetry();
            config->setDisableNagleAlgorithm();
            config->setIPv6Only();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in6-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
    net::in6::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in6-mqtts",
        [](net::in6::stream::tls::config::ConfigSocketServer* config) {
            config->setPort(8883);
            config->setRetry();
            config->setDisableNagleAlgorithm();
            config->setIPv6Only();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in6-mqtts", socketAddress, state);
        });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX
    net::un::stream::legacy::Server<mqtt::mqttbroker::SocketContextFactory>(
        "un-mqtt",
        [](net::un::stream::legacy::config::ConfigSocketServer* config) {
            config->setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config->getInstanceName());
            config->setRetry();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("un-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
    net::un::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "un-mqtts",
        [](net::un::stream::tls::config::ConfigSocketServer* config) {
            config->setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config->getInstanceName());
            config->setRetry();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("un-mqtts", socketAddress, state);
        });
#endif
#endif

    express::Router router = getRouter(broker, configMqttBroker);

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
    express::legacy::in::Server(
        "in-http",
        router,
        reportState,
        [](net::in::stream::legacy::config::ConfigSocketServer* config) {
            config->setPort(8080);
            config->setRetry();
            config->setDisableNagleAlgorithm();
        });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
    express::tls::in::Server(
        "in-https",
        router,
        reportState,
        [](net::in::stream::tls::config::ConfigSocketServer* config) {
            config->setPort(8088);
            config->setRetry();
            config->setDisableNagleAlgorithm();
        });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV6
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

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
    express::tls::in6::Server(
        "in6-https",
        router,
        reportState,
        [](net::in6::stream::tls::config::ConfigSocketServer* config) {
            config->setPort(8088);
            config->setRetry();
            config->setDisableNagleAlgorithm();
            config->setIPv6Only();
        });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX
    express::legacy::un::Server(
        "un-http",
        router,
        reportState,
        [](net::un::stream::legacy::config::ConfigSocketServer* config) {
            config->setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config->getInstanceName());
        });

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
    express::tls::un::Server(
        "un-https",
        router,
        reportState,
        [](net::un::stream::tls::config::ConfigSocketServer* config) {
            config->setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config->getInstanceName());
        });
#endif
#endif

    return core::SNodeC::start();
}

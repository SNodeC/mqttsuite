/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
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
#include "lib/Mqtt.h"
#include "lib/MqttModel.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#endif
#endif
#include "lib/inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <iot/mqtt/MqttContext.h>
#include <utils/Config.h>
//

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

//
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

#include <web/http/http_utils.h>
//
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>
#include <iot/mqtt/server/broker/Broker.h>
//
#include <log/Logger.h>
//
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
//
#include <string>
#include <utility>
//
#endif

static void upgrade APPLICATION(req, res) {
    const std::string connectionName = res->getSocketContext()->getSocketConnection()->getConnectionName();

    VLOG(2) << connectionName << " HTTP: Upgrade request:\n"
            << httputils::toString(req->method,
                                   req->url,
                                   "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                   req->queries,
                                   req->headers,
                                   req->trailer,
                                   req->cookies,
                                   std::vector<char>());

    if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
        res->upgrade(req, [req, res, connectionName](const std::string& name) {
            if (!name.empty()) {
                VLOG(1) << connectionName << ": Successful upgrade:";
                VLOG(1) << connectionName << ":    Selected: " << name;
                VLOG(1) << connectionName << ":   Requested: " << req->get("sec-websocket-protocol");

                res->end();
            } else {
                VLOG(1) << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";

                res->sendStatus(404);
            }
        });
    } else {
        VLOG(1) << connectionName << ": Unsupported subprotocol(s):";
        VLOG(1) << "    Expected: mqtt";
        VLOG(1) << "   Requested: " << req->get("sec-websocket-protocol");

        res->sendStatus(404);
    }
}

static express::Router getRouter([[maybe_unused]] const inja::Environment& environment,
                                 std::shared_ptr<iot::mqtt::server::broker::Broker> broker) {
    const express::Router& jsonRouter = express::middleware::JsonMiddleware();

    /*
     * /api/mqtt/disconnect
     * JSON.stringify({ clientId })
     */
    jsonRouter.use("/api/mqtt", [] MIDDLEWARE(req, res, next) { // cppcheck-suppress unknownMacro
        res->set({{"Access-Control-Allow-Origin", "*"},
                  {"Access-Control-Allow-Headers", "Content-Type"},
                  {"Access-Control-Allow-Methods", "GET, OPTIONS, POST"},
                  {"Access-Control-Allow-Private-Network", "true"}});
        next();
    });

    jsonRouter.post("/api/mqtt/disconnect", [] APPLICATION(req, res) {
        VLOG(1) << "POST /disconnect";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["clientId"].get<std::string>();
                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->getMqttContext()->getSocketConnection()->close();
                    res->send(R"({"success": true, "message": "Client disconnected successfully"})"_json.dump());
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    /*
     * /api/mqtt/unsubscribe
     * JSON.stringify({clientId, topic})
     */
    jsonRouter.post("/api/mqtt/unsubscribe", [] APPLICATION(req, res) {
        VLOG(1) << "POST /unsubscribe";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["clientId"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();

                mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->unsubscribe(topic);
                    res->send(R"({"success": true, "message": "Client unsubscribed successfully"})"_json.dump());
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    /*
     * /api/mqtt/release
     * JSON.stringify({ topic })
     */
    jsonRouter.post("/api/mqtt/release", [broker] APPLICATION(req, res) {
        VLOG(1) << "POST /release";

        req->getAttribute<nlohmann::json>(
            [&res, broker](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string topic = json["topic"].get<std::string>();

                broker->publish("", topic, "", 0, true);
                mqtt::mqttbroker::lib::MqttModel::instance().publishMessage(topic, "", 0, true);

                res->send(R"({"success": true, "message": "Retained message released successfully"})"_json.dump());
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    /*
     * /api/mqtt/subscribe
     * JSON.stringify({ clientId, topic, qos })
     */
    jsonRouter.post("/api/mqtt/subscribe", [] APPLICATION(req, res) {
        VLOG(1) << "POST /subscribe";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["clientId"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();
                uint8_t qoS = json["qos"].get<uint8_t>();

                mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->subscribe(topic, qoS);

                    res->send(R"({"success": true, "message": "Client subscribed successfully"})"_json.dump());
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });
    const express::Router router;

    router.use(jsonRouter);

    router.get("/api/mqtt/events", [broker] APPLICATION(req, res) {
        if (web::http::ciContains(req->get("Accept"), "text/event-stream")) {
            res->set({{"Content-Type", "text/event-stream"},
                      {"Cache-Control", "no-cache"},
                      {"Connection", "keep-alive"},
                      {"Access-Control-Allow-Origin", "*"}});

            res->sendHeader();

            mqtt::mqttbroker::lib::MqttModel::instance().addEventReceiver(res, req->get("Last-Event-ID"), broker);
        } else {
            res->redirect("/clients");
        }
    });

    router.setStrictRouting();
    router.get("/clients", [] APPLICATION(req, res) {
        res->redirect("/clients/index.html");
    });

    router.get("/clients", express::middleware::StaticMiddleware(utils::Config::getStringOptionValue("--html-dir")));

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
            res->set("Content-Type", "text/event-stream") //
                .set("Cache-Control", "no-cache")
                .set("Connection", "keep-alive");
            res->sendHeader();

            mqtt::mqttbroker::lib::MqttModel::instance().addEventReceiver(res, req->get("Last-Event-ID"), broker);
        } else {
            res->redirect("/clients");
        }
    });

    return router;
}

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
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

int main(int argc, char* argv[]) {
    utils::Config::addStringOption("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::addStringOption("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");
    utils::Config::addStringOption(
        "--html-dir", "Path to html source directory", "[path]", std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttbroker");

    core::SNodeC::init(argc, argv);

    std::shared_ptr<iot::mqtt::server::broker::Broker> broker =
        iot::mqtt::server::broker::Broker::instance(SUBSCRIPTION_MAX_QOS, utils::Config::getStringOptionValue("--mqtt-session-store"));

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
    net::in::stream::legacy::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in-mqtt",
        [](auto& config) {
            config.setPort(1883);
            config.setRetry();
            config.setDisableNagleAlgorithm();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
    net::in::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in-mqtts",
        [](auto& config) {
            config.setPort(8883);
            config.setRetry();
            config.setDisableNagleAlgorithm();
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
        [](auto& config) {
            config.setPort(1883);
            config.setRetry();
            config.setDisableNagleAlgorithm();

            config.setIPv6Only();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("in6-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
    net::in6::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "in6-mqtts",
        [](auto& config) {
            config.setPort(8883);
            config.setRetry();
            config.setDisableNagleAlgorithm();

            config.setIPv6Only();
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
        [](auto& config) {
            config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
            config.setRetry();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("un-mqtt", socketAddress, state);
        });

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
    net::un::stream::tls::Server<mqtt::mqttbroker::SocketContextFactory>(
        "un-mqtts",
        [](auto& config) {
            config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
            config.setRetry();
        },
        broker)
        .listen([](const auto& socketAddress, core::socket::State state) {
            reportState("un-mqtts", socketAddress, state);
        });
#endif
#endif

    inja::Environment environment{utils::Config::getStringOptionValue("--html-dir") + "/"};
    express::Router router = getRouter(environment, broker);

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV4
    express::legacy::in::Server("in-http", router, reportState, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV4
    express::tls::in::Server("in-https", router, reportState, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_TCP_IPV6
    express::legacy::in6::Server("in6-http", router, reportState, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

#ifdef CONFIG_MQTTSUITE_BROKER_TLS_IPV6
    express::tls::in6::Server("in6-https", router, reportState, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });
#endif
#endif

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX
    express::legacy::un::Server("un-http", router, reportState, [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
    });

#ifdef CONFIG_MQTTSUITE_BROKER_UNIX_TLS
    express::tls::un::Server("un-https", router, reportState, [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
    });
#endif
#endif

    return core::SNodeC::start();
}

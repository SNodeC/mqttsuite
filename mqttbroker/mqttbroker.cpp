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
#include "lib/Mqtt.h"
#include "lib/MqttModel.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
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
#include <express/legacy/in/Server.h>
#include <express/legacy/in6/Server.h>
#include <express/legacy/un/Server.h>
#include <express/tls/in/Server.h>
#include <express/tls/in6/Server.h>
#include <express/tls/un/Server.h>
//
#include <net/in/stream/legacy/SocketServer.h>
#include <net/in/stream/tls/SocketServer.h>
#include <net/in6/stream/legacy/SocketServer.h>
#include <net/in6/stream/tls/SocketServer.h>
#include <net/un/stream/legacy/SocketServer.h>
#include <net/un/stream/tls/SocketServer.h>
//
#include <express/middleware/JsonMiddleware.h>
#include <iot/mqtt/server/broker/Broker.h>
//
#include <log/Logger.h>
//
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
//
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include <utility>

#endif

static void upgrade APPLICATION(req, res) {
    if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
        res->upgrade(req, [req, res](const std::string& name) {
            if (!name.empty()) {
                VLOG(1) << "Successful upgrade to '" << name << "'  requested: " << req->get("upgrade");
            } else {
                VLOG(1) << "Can not upgrade to any of '" << req->get("upgrade") << "'";
            }
            res->end();
        });
    } else {
        VLOG(1) << "Not supporting any of: " << req->get("sec-websocket-protocol");

        res->sendStatus(404);
    }
}

static std::string href(const std::string& text, const std::string& link) {
    return "<a href=\"" + link + "\" style=\"color:inherit;\">" + text + "</a>";
}

static std::string href(const std::string& text, const std::string& url, const std::string& windowId, uint16_t width, uint16_t height) {
    return "<a href=\"#\" onClick=\""
           "let key = '" +
           windowId +
           "'; "
           "if (!localStorage.getItem(key)) "
           "  localStorage.setItem(key, key + '-' + Math.random().toString(36).substr(2, 6)); "
           "let uniqueId = localStorage.getItem(key); "
           "if (!window._openWindows) window._openWindows = {}; "
           "if (!window._openWindows[uniqueId] || window._openWindows[uniqueId].closed) { "
           "  window._openWindows[uniqueId] = window.open('" +
           url + "', uniqueId, 'width=" + std::to_string(width) + ", height=" + std::to_string(height) +
           ",location=no, menubar=no, status=no, toolbar=no'); "
           "} else { "
           "  window._openWindows[uniqueId].focus(); "
           "} return false;\" "
           "style=\"color:inherit;\">" +
           text + "</a>";
}

static std::string getOverviewPage(inja::Environment environment,
                                   std::shared_ptr<iot::mqtt::server::broker::Broker> broker,
                                   mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    inja::json json;

    json["title"] = "MQTTBroker | Active Clients";
    json["voc"] = href("Volker Christian", "https://github.com/VolkerChristian/");
    json["broker"] = href("MQTTBroker", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker");
    json["suite"] = href("MQTTSuite", "https://github.com/SNodeC/mqttsuite");
    json["snodec"] = "Powered by " + href("SNode.C", "https://github.com/SNodeC/snode.c");
    json["since"] = mqttModel.onlineSince();
    json["duration"] = mqttModel.onlineDuration();
    json["header_row"] = {"Client ID", "Online Since", "Duration", "Connection", "Local Address", "Remote Address", "Action"};
    json["data_rows"] = inja::json::array();
    json["subscribed_topics"] = inja::json::array();
    json["retained_topics"] = inja::json::array();

    inja::json& jsonDataRows = json["data_rows"];
    for (const auto& [connectionName, mqttModelEntry] : mqttModel.getClients()) {
        const mqtt::mqttbroker::lib::Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        std::ostringstream windowId("window");
        for (char ch : mqtt->getClientId()) {
            if (std::isalnum(static_cast<unsigned char>(ch))) {
                windowId << ch;
            } else {
                windowId << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                         << static_cast<int>(static_cast<unsigned char>(ch));
            }
        }

        jsonDataRows.push_back(
            {href(mqtt->getClientId(), "/client?" + mqtt->getClientId(), windowId.str(), 450, 900),
             mqttModelEntry.onlineSince(),
             "<duration>" + mqttModelEntry.onlineDuration() + "</duration>",
             mqtt->getConnectionName(),
             socketConnection->getLocalAddress().toString(),
             socketConnection->getRemoteAddress().toString(),
             "<button class=\"red-btn\" onClick=\"disconnectClient('" + mqtt->getClientId() + "')\">Disconnect</button>"});
    }

    std::map<std::string, std::list<std::pair<std::string, uint8_t>>> subscribedTopics = broker->getSubscriptionTree();

    inja::json& subscribedTopicsJson = json["subscribed_topics"];
    for (const auto& [topic, clients] : subscribedTopics) {
        inja::json topicJson;

        topicJson["key"] = topic;
        for (const auto& client : clients) {
            std::ostringstream windowId("window");
            for (char ch : client.first) {
                if (std::isalnum(static_cast<unsigned char>(ch))) {
                    windowId << ch;
                } else {
                    windowId << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                             << static_cast<int>(static_cast<unsigned char>(ch));
                }
            }

            topicJson["values"].push_back({{"client_id", client.first},
                                           {"link", href(client.first, "/client?" + client.first, windowId.str(), 450, 900)},
                                           {"topic", topic},
                                           {"qos", std::to_string(static_cast<int>(client.second))}});
        }

        subscribedTopicsJson.push_back(topicJson);
    }

    std::list<std::pair<std::string, std::string>> retainTree = broker->getRetainTree();

    inja::json& retainedTopicsJson = json["retained_topics"];
    for (const auto& [topic, message] : retainTree) {
        inja::json topicJson;

        topicJson["key"] = topic;
        topicJson["values"].push_back(message);

        retainedTopicsJson.push_back(topicJson);
    }

    return environment.render_file("OverviewPage.html", json);
}

static std::string getDetailedPage(inja::Environment environment, const mqtt::mqttbroker::lib::Mqtt* mqtt) {
    return environment.render_file("DetailPage.html",
                                   {{"client_id", mqtt->getClientId()},
                                    {"title", mqtt->getClientId()},
                                    {"header_row", {"Attribute", "Value"}},
                                    {"data_rows",
                                     inja::json::array({{"Client ID", mqtt->getClientId()},
                                                        {"Connection", mqtt->getConnectionName()},
                                                        {"Clean Session", mqtt->getCleanSession() ? "true" : "false"},
                                                        {"Connect Flags", std::to_string(mqtt->getConnectFlags())},
                                                        {"Username", mqtt->getUsername()},
                                                        {"Username Flag", mqtt->getUsernameFlag() ? "true" : "false"},
                                                        {"Password", mqtt->getPassword()},
                                                        {"Password Flag", mqtt->getPasswordFlag() ? "true" : "false"},
                                                        {"Keep Alive", std::to_string(mqtt->getKeepAlive())},
                                                        {"Protocol", mqtt->getProtocol()},
                                                        {"Protocol Level", std::to_string(mqtt->getLevel())},
                                                        {"Loop Prevention", !mqtt->getReflect() ? "true" : "false"},
                                                        {"Will Message", mqtt->getWillMessage()},
                                                        {"Will Topic", mqtt->getWillTopic()},
                                                        {"Will QoS", std::to_string(mqtt->getWillQoS())},
                                                        {"Will Flag", mqtt->getWillFlag() ? "true" : "false"},
                                                        {"Will Retain", mqtt->getWillRetain() ? "true" : "false"}})},
                                    {"client_id", mqtt->getClientId()},
                                    {"topics", mqtt->getSubscriptions()}});
}

static std::string getRedirectSpinnerPage(inja::Environment environment, const std::string& location) {
    return environment.render_file("Spinner.html", {{"location", location}});
}

static std::string urlDecode(const std::string& encoded) {
    std::string decoded;
    size_t i = 0;

    while (i < encoded.length()) {
        char ch = encoded[i];
        if (ch == '%') {
            // Make sure there are at least two characters after '%'
            if (i + 2 < encoded.length() && std::isxdigit(encoded[i + 1]) && std::isxdigit(encoded[i + 2])) {
                // Convert the two hex digits to a character
                std::string hexValue = encoded.substr(i + 1, 2);
                char decodedChar = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                decoded.push_back(decodedChar);
                i += 3; // Skip over the % and the two hex digits
            } else {
                // Malformed encoding, just add the '%' as is.
                decoded.push_back(ch);
                ++i;
            }
        } else if (ch == '+') {
            // Convert '+' to space (common in URL encoding)
            decoded.push_back(' ');
            ++i;
        } else {
            // Regular character, just append it.
            decoded.push_back(ch);
            ++i;
        }
    }

    return decoded;
}

static express::Router getRouter(const inja::Environment& environment, std::shared_ptr<iot::mqtt::server::broker::Broker> broker) {
    const express::Router router;

    const express::Router& jsonRouter = express::middleware::JsonMiddleware();
    jsonRouter.setStrictRouting();

    jsonRouter.post("/disconnect", [] APPLICATION(req, res) {
        VLOG(1) << "POST /disconnect";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["client_id"].get<std::string>();
                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->getMqttContext()->getSocketConnection()->close();
                    res->send(jsonString);
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/unsubscribe", [] APPLICATION(req, res) {
        VLOG(1) << "POST /unsubscribe";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["client_id"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();

                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->unsubscribe(topic);
                    res->send(jsonString);
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/release", [broker] APPLICATION(req, res) {
        VLOG(1) << "POST /release";

        req->getAttribute<nlohmann::json>(
            [&res, broker](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string topic = json["topic"].get<std::string>();

                broker->release(topic);

                res->send(jsonString);
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    jsonRouter.post("/subscribe", [] APPLICATION(req, res) {
        VLOG(1) << "POST /subscribe";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);

                VLOG(1) << jsonString;

                std::string clientId = json["client_id"].get<std::string>();
                std::string topic = json["topic"].get<std::string>();
                uint8_t qoS = json["qos"].get<uint8_t>();

                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(clientId);

                if (mqtt != nullptr) {
                    mqtt->subscribe(topic, qoS);
                    res->send(jsonString);
                } else {
                    res->status(404).send("MQTT client has never existed or already gone away: '" + clientId + "'");
                }
            },
            [&res](const std::string& key) {
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    router.use(jsonRouter);

    router.get("/clients", [environment, broker] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        try {
            responseString = getOverviewPage(environment, broker, mqtt::mqttbroker::lib::MqttModel::instance());
        } catch (const inja::InjaError& error) {
            responseStatus = 500;
            responseString = "Internal Server Error\n";
            responseString += std::string(error.what()) + " " + error.type + " " + error.message + " " +
                              std::to_string(error.location.line) + ":" + std::to_string(error.location.column);
        }

        res->status(responseStatus).send(responseString);
    });

    router.get("/client", [environment] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        if (req->queries.size() == 1) {
            const mqtt::mqttbroker::lib::Mqtt* mqtt =
                mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(urlDecode(req->queries.begin()->first));

            if (mqtt != nullptr) {
                VLOG(1) << "Subscriptions for client " << mqtt->getClientId();
                for (const std::string& subscription : mqtt->getSubscriptions()) {
                    VLOG(1) << "  " << subscription;
                }

                try {
                    responseString = getDetailedPage(environment, mqtt);
                } catch (const inja::InjaError& error) {
                    responseStatus = 500;
                    responseString = "Internal Server Error\n";
                    responseString += std::string(error.what()) + " " + error.type + " " + error.message + " " +
                                      std::to_string(error.location.line) + ":" + std::to_string(error.location.column);
                }
            } else {
                responseStatus = 404;
                responseString = "Not Found: " + urlDecode(req->queries.begin()->first);
            }
        } else {
            responseStatus = 400;
            responseString = "Bad Request: No Client requested";
        }

        res->status(responseStatus).send(responseString);
    });

    router.get("/spinner", [environment] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        if (req->queries.size() == 1) {
            responseString = getRedirectSpinnerPage(environment, req->queries.begin()->first);
        } else {
            responseStatus = 400;
            responseString = "Bad Request: No Client requested";
        }
        res->status(responseStatus).send(responseString);
    });

    router
        .get("/ws",
             [] APPLICATION(req, res) {
                 if (req->headers.contains("upgrade")) {
                     upgrade(req, res);
                 } else {
                     res->redirect("/spinner?/clients");
                 }
             })
        .setStrictRouting();

    router
        .get("/mqtt",
             [] APPLICATION(req, res) {
                 if (req->headers.contains("upgrade")) {
                     upgrade(req, res);
                 } else {
                     res->redirect("/spinner?/clients");
                 }
             })

        .setStrictRouting();

    router
        .get("/",
             [] APPLICATION(req, res) {
                 if (req->headers.contains("upgrade")) {
                     upgrade(req, res);
                 } else {
                     res->redirect("/spinner?/clients");
                 }
             })
        .setStrictRouting();

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

    inja::Environment environment{utils::Config::getStringOptionValue("--html-dir") + "/"};
    express::Router router = getRouter(environment, broker);

    express::legacy::in::Server("in-http", router, reportState, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    express::tls::in::Server("in-https", router, reportState, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    express::legacy::in6::Server("in6-http", router, reportState, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

    express::tls::in6::Server("in6-https", router, reportState, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

    express::legacy::un::Server("un-http", router, reportState, [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
    });

    express::tls::un::Server("un-https", router, reportState, [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
    });

    return core::SNodeC::start();
}

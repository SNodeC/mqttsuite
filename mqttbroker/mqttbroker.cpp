/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "SharedSocketContextFactory.h"
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
#include <express/legacy/in/WebApp.h>
#include <express/legacy/in6/WebApp.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/tls/in/WebApp.h>
#include <express/tls/in6/WebApp.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/in/stream/tls/SocketServer.h>
#include <net/un/stream/legacy/SocketServer.h>
#include <net/un/stream/tls/SocketServer.h>
//
#include <log/Logger.h>
#include <utils/CLI11.hpp>
//
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
//
#include <cstdlib>
#include <fmt/core.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

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
    return "<a href=\\\"#\\\" onClick=\\\"" + windowId + "=window.open('" + url + "', '" + windowId + "', 'width=" + std::to_string(width) +
           ", height=" + std::to_string(height) +
           ",location=no, menubar=no, status=no, toolbar=no'); return false;\\\"  \\\" style=\\\"color:inherit;\\\">" + text + "</a>";
}

static std::string getHTMLPageClientTable(mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    inja::json json{{"title", "MQTTBroker | Active Clients"},
                    {"header_row", {"Client ID", "Online Since", "Duration", "Connection", "Locale Address", "Remote Address", "Action"}}};

    std::string jsonString = R"(
    {
        "data_rows": [
    )";

    for (const auto& [connectionName, mqttModelEntry] : mqttModel.getClients()) {
        const mqtt::mqttbroker::lib::Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        const std::string windowId = "window" + std::to_string(reinterpret_cast<unsigned long long>(mqtt));

        jsonString += std::string("[") + //                                                                          //
                      "\"" +
                      href(mqtt->getClientId(),
                           "/client/?" + //
                               mqtt->getConnectionName(),
                           windowId,
                           450,
                           900) +
                      "\" ," +                                                              //
                      "\"" + mqttModelEntry.onlineSince() + "\" ," +                        //
                      "\"<duration>" + mqttModelEntry.onlineDuration() + "<duration>\" ," + //
                      "\"" + mqtt->getConnectionName() + "\" ," +                           //
                      "\"" + socketConnection->getLocalAddress().toString() + "\" ," +      //
                      "\"" + socketConnection->getRemoteAddress().toString() + "\" , " +    //
                      "\"<button onclick=\\\"disconnectClient('" + mqtt->getConnectionName() +
                      "')\\\">Disconnect</button>\"" //
                      "],";
    }
    jsonString.pop_back();

    jsonString += "]}";

    json.merge_patch(inja::json::parse(jsonString));

    json["voc"] = href("Volker Christian", "https://github.com/VolkerChristian/");
    json["broker"] = href("MQTTBroker", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker");
    json["suite"] = href("MQTTSuite", "https://github.com/SNodeC/mqttsuite");
    json["snodec"] = "Powered by " + href("SNode.C", "https://github.com/SNodeC/snode.c");
    json["since"] = mqttModel.onlineSince();
    json["duration"] = mqttModel.onlineDuration();

    inja::Environment env;

    const std::string& htmlPath = utils::Config::getStringOptionValue("--html-dir");

    return env.render_file(htmlPath + "/OverviewPage.html", json);
}

static std::string getDetailedPage(const mqtt::mqttbroker::lib::Mqtt* mqtt) {
    inja::Environment env;

    const std::string& htmlPath = utils::Config::getStringOptionValue("--html-dir");

    return env.render_file(htmlPath + "/DetailPage.html",
                           {{"title", mqtt->getClientId()},
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
                                                {"Will Retain", mqtt->getWillRetain() ? "true" : "false"}})}});
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

static express::Router getRouter() {
    const express::Router router;

    router.get("/clients", [] APPLICATION(req, res) {
        res->send(getHTMLPageClientTable(mqtt::mqttbroker::lib::MqttModel::instance()));
    });

    router.get("/client", [] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        if (req->queries.size() == 1) {
            const mqtt::mqttbroker::lib::Mqtt* mqtt =
                mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(urlDecode(req->queries.begin()->first));

            if (mqtt != nullptr) {
                responseString = getDetailedPage(mqtt);
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

    const express::Router& jsonRouter = express::middleware::JsonMiddleware();

    jsonRouter.post([] APPLICATION(req, res) {
        VLOG(2) << "+++++++++++++++++++++++++++++\n" //
                << std::string(req->body.begin(), req->body.end());
        VLOG(2) << "-----------------------------";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);
                VLOG(2) << "Application received JSON body\n" << jsonString;

                std::string connectionName = json["connection_name"].get<std::string>();
                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(connectionName);

                if (mqtt != nullptr) {
                    mqtt->getMqttContext()->getSocketConnection()->close();
                    res->send(jsonString);
                } else {
                    res->status(404).send("MQTT client has already gone away: " + json["connection_name"].get<std::string>());
                }
            },
            [&res](const std::string& key) {
                VLOG(0) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
        VLOG(2) << "+++++++++++++++++++++++++++++";
    });

    router.use("/clients", jsonRouter);

    router.get("/ws/", [] APPLICATION(req, res) {
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

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>,
          typename SocketAddress = typename Server::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startServer(const std::string& instanceName,
                 const std::function<void(typename Server::Config&)>& configurator,
                 SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    const Server server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(server.getConfig());

    server.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>,
          typename SocketAddress = typename Server::SocketAddress,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketServer<SocketContextFactory>::Config&>>

          >
void startServer(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    Server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...)
        .listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
            reportState(instanceName, socketAddress, state);
        });
}

template <typename HttpExpressServer>
void startServer(const std::string& instanceName, const std::function<void(typename HttpExpressServer::Config&)>& configurator = nullptr) {
    using SocketAddress = typename HttpExpressServer::SocketAddress;

    const HttpExpressServer httpExpressServer(instanceName, getRouter());

    if (configurator != nullptr) {
        configurator(httpExpressServer.getConfig());
    }
    httpExpressServer.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    utils::Config::addStringOption("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::addStringOption("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");
    utils::Config::addStringOption("--html-dir", "Path to html source directory", "[path]", "")->required();

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::getStringOptionValue("--mqtt-session-store").data(), 0);

    startServer<net::in::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();
    });

    startServer<net::in::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();
    });

    startServer<net::in6::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<net::in6::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<net::un::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("un-mqtt", [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
        config.setRetry();
    });

    startServer<net::un::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("un-mqtts", [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
        config.setRetry();
    });

    startServer<express::legacy::in::WebApp>("in-http", [](auto& config) {
        config.setPort(8080);
        config.setRetry();
    });

    startServer<express::tls::in::WebApp>("in-https", [](auto& config) {
        config.setPort(8088);
        config.setRetry();
    });

    startServer<express::legacy::in6::WebApp>("in6-http", [](auto& config) {
        config.setPort(8080);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<express::tls::in6::WebApp>("in6-https", [](auto& config) {
        config.setPort(8088);
        config.setRetry();

        config.setIPv6Only();
    });

    return core::SNodeC::start();
}

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
//
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
//
#include <cctype>
#include <cstdlib>
#include <list>
#include <sstream>
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
    return "<a href=\"#\" onClick=\"" + windowId + "=window.open('" + url + "', '" + windowId + "', 'width=" + std::to_string(width) +
           ", height=" + std::to_string(height) +
           ",location=no, menubar=no, status=no, toolbar=no'); return false;\" style=\"color:inherit;\">" + text + "</a>";
}

// --- Tree structure for topics ---
struct Node {
    std::map<std::string, Node> children;
};

// Insert a topic (a slash‑separated string) into the tree.
static void insertTopic(Node& root, const std::string& topic) {
    std::istringstream iss(topic);
    std::string part;
    Node* current = &root;
    while (std::getline(iss, part, '/')) {
        current = &current->children[part]; // Create node if it doesn't exist.
    }
}

// Recursively render table rows as an HTML string.
// Compresses chains with exactly one child (i.e. concatenates keys)
// until a branch or leaf is reached.
// For expandable nodes, child rows are wrapped in a <tbody> (hidden by default) with a unique id.
static std::string renderRows(const Node& node, const std::string& prefix, int level, int& groupCounter) {
    std::ostringstream oss;
    for (const auto& pair : node.children) {
        std::string key = pair.first;
        const Node& child = pair.second;
        std::string fullPath = prefix.empty() ? key : prefix + "/" + key;
        const Node* current = &child;
        // Compress the chain while there is exactly one child.
        while (current->children.size() == 1) {
            auto it = current->children.begin();
            fullPath += "/" + it->first;
            current = &it->second;
        }
        bool expandable = !current->children.empty();
        if (expandable) {
            // Generate a unique group id for the child rows.
            std::string groupId = "group" + std::to_string(groupCounter++);
            oss << "<tr>";
            // First column: fold button with onclick to toggle the group.
            oss << "<td><button class=\"fold-btn\" onclick=\"toggleGroup('" << groupId << "', this)\">►</button></td>";
            // Second column: topic text with left padding proportional to the level.
            oss << "<td style=\"padding-left:" << (20 * level) << "px;\">" << fullPath << "</td>";
            // Third column: empty for expandable nodes.
            oss << "<td></td>";
            oss << "</tr>\n";
            // Render child rows inside a tbody that is hidden by default.
            oss << "<tbody id=\"" << groupId << "\" style=\"display:none;\">";
            oss << renderRows(*current, fullPath, level + 1, groupCounter);
            oss << "</tbody>\n";
        } else {
            // Leaf node: output a single row.
            oss << "<tr>";
            // First column: placeholder (to align with fold buttons).
            oss << "<td><span style=\"display:inline-block; width:1em;\"></span></td>";
            // Second column: topic text with left padding.
            oss << "<td style=\"padding-left:" << (20 * level) << "px;\">" << fullPath << "</td>";
            // Third column: Unsubscribe button triggering a JavaScript function.
            oss << "<td><button onclick=\"trigger('" << fullPath << "')\">Unsubscribe</button></td>";
            oss << "</tr>\n";
        }
    }
    return oss.str();
}

// Render the complete HTML page using an Inja template.
[[maybe_unused]] static std::string renderHTML(const Node& root) {
    int groupCounter = 1;
    std::string tableRows = renderRows(root, "", 0, groupCounter);

    inja::Environment env;
    std::string templateStr = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>MQTT Topics</title>
  <style>
    table { width: 100%; font-family: sans-serif; border-collapse: collapse; }
    td { padding: 4px; vertical-align: middle; text-align: left; border: none; }
    .fold-btn { background: none; border: none; cursor: pointer; font-size: 1em; }
  </style>
  <script>
    function toggleGroup(id, btn) {
      var group = document.getElementById(id);
      if (group.style.display === 'none') {
        group.style.display = '';
        btn.textContent = '▼';
      } else {
        group.style.display = 'none';
        btn.textContent = '►';
      }
    }
    function trigger(topic) {
      alert('Unsubscribed from topic: ' + topic);
    }
  </script>
</head>
<body>
  <table>
    <colgroup>
      <col style='width:2em;'>
      <col>
      <col style='width:8em;'>
    </colgroup>
    <tbody>
      {{ table_rows | safe }}
    </tbody>
  </table>
</body>
</html>
    )";

    nlohmann::json data;
    data["table_rows"] = tableRows;
    return env.render(templateStr, data);
}

static std::string getOverviewPage(inja::Environment& environment, mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    inja::json json;

    json["title"] = "MQTTBroker | Active Clients";
    json["header_row"] = {"Client ID", "Online Since", "Duration", "Connection", "Local Address", "Remote Address", "Action"};
    json["voc"] = href("Volker Christian", "https://github.com/VolkerChristian/");
    json["broker"] = href("MQTTBroker", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker");
    json["suite"] = href("MQTTSuite", "https://github.com/SNodeC/mqttsuite");
    json["snodec"] = "Powered by " + href("SNode.C", "https://github.com/SNodeC/snode.c");
    json["since"] = mqttModel.onlineSince();
    json["duration"] = mqttModel.onlineDuration();
    json["data_rows"] = inja::json::array();

    inja::json& jsonDataRows = json["data_rows"];
    for (const auto& [connectionName, mqttModelEntry] : mqttModel.getClients()) {
        const mqtt::mqttbroker::lib::Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        const std::string windowId = "window" + std::to_string(reinterpret_cast<unsigned long long>(mqtt));

        jsonDataRows.push_back({href(mqtt->getClientId(), "/client?" + mqtt->getConnectionName(), windowId, 450, 900),
                                mqttModelEntry.onlineSince(),
                                "<duration>" + mqttModelEntry.onlineDuration() + "</duration>",
                                mqtt->getConnectionName(),
                                socketConnection->getLocalAddress().toString(),
                                socketConnection->getRemoteAddress().toString(),
                                "<button onClick=\"disconnectClient('" + mqtt->getConnectionName() + "')\">Disconnect</button>"});
    }

    return environment.render_file("OverviewPage.html", json);
}

static std::string getDetailedPage(inja::Environment& environment, const mqtt::mqttbroker::lib::Mqtt* mqtt) {
    std::list<std::string> topicsList = mqtt->getSubscriptions();

    Node root;
    for (const auto& topic : topicsList) {
        insertTopic(root, topic);
    }

    int groupCounter = 1;

    return environment.render_file("DetailPage.html",
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
                                                        {"Will Retain", mqtt->getWillRetain() ? "true" : "false"}})},
                                    {"table_rows", renderRows(root, "", 0, groupCounter)}});
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

static express::Router getRouter(inja::Environment& environment) {
    const express::Router router;

    const express::Router& jsonRouter = express::middleware::JsonMiddleware();

    jsonRouter.post([] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);
                VLOG(1) << "Application received JSON body\n" << jsonString;

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
                VLOG(1) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
    });

    router.use("/clients", jsonRouter);

    router.get("/clients", [&environment] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        try {
            responseString = getOverviewPage(environment, mqtt::mqttbroker::lib::MqttModel::instance());
        } catch (...) {
            responseStatus = 500;
            responseString = "Internal Server Error";
        }

        res->status(responseStatus).send(responseString);
    });

    router.get("/client", [&environment] APPLICATION(req, res) {
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
                } catch (...) {
                    responseStatus = 500;
                    responseString = "Internal Server Error";
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

    router.get("/ws", [] APPLICATION(req, res) {
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
void startServer(const std::string& instanceName,
                 inja::Environment& environment,
                 const std::function<void(typename HttpExpressServer::Config&)>& configurator = nullptr) {
    using SocketAddress = typename HttpExpressServer::SocketAddress;

    const HttpExpressServer httpExpressServer(instanceName, getRouter(environment));

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
    utils::Config::addStringOption(
        "--html-dir", "Path to html source directory", "[path]", std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttbroker");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::getStringOptionValue("--mqtt-session-store").data(), 0);

    startServer<net::in::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    startServer<net::in::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    startServer<net::in6::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

    startServer<net::in6::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();
        config.setDisableNagleAlgorithm();

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

    inja::Environment environment{utils::Config::getStringOptionValue("--html-dir") + "/"};

    startServer<express::legacy::in::WebApp>("in-http", environment, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    startServer<express::tls::in::WebApp>("in-https", environment, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();
    });

    startServer<express::legacy::in6::WebApp>("in6-http", environment, [](auto& config) {
        config.setPort(8080);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

    startServer<express::tls::in6::WebApp>("in6-https", environment, [](auto& config) {
        config.setPort(8088);
        config.setRetry();
        config.setDisableNagleAlgorithm();

        config.setIPv6Only();
    });

    return core::SNodeC::start();
}

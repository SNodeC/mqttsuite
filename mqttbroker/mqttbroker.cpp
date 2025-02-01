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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <iot/mqtt/MqttContext.h>
//
#include <express/legacy/in/WebApp.h>
#include <express/legacy/in6/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <express/tls/in6/WebApp.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/in/stream/tls/SocketServer.h>
#include <net/un/stream/legacy/SocketServer.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <cstdlib>
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

/*
fetch(url , {
method: "POST",
        body: JSON.stringify({
            row: rowNumber
        }),
        headers: {
            "Content-type": "application/json; charset=UTF-8"
        }
)
*/

static express::Router getRouter() {
    const express::Router router;

    router.get("/clients", [] APPLICATION(req, res) {
        std::string responseString =
            "<html>"
            "  <head>"
            "    <title>Mqtt Broker</title>"
            "    <style>"
            "      table {"
            "        width: 100%;"
            "        border-collapse: collapse;"
            "        margin: 20px 0;"
            "        font-family: Arial, sans-serif;"
            "      }"
            "      th, td {"
            "        padding: 12px;"
            "        border: 1px solid #ccc;"
            "        text-align: left;"
            "      }"
            "      th {"
            "        background-color: #f4f4f4;"
            "      }"
            "      tr:nth-child(even) {"
            "        background-color: #f9f9f9;"
            "      }"
            "      tr:hover {"
            "        background-color: #e0e0e0;"
            "      }"
            "    </style>"
            "    <script>"
            "      // This function performs an HTTP GET request when a button in any row is clicked.\n"
            "      function executeCode(rowNumber) {"
            "        // const url = \"https://api.example.com/data?row=\" + rowNumber + \";\"\n"
            "        // Example URL endpoint (update with your own URL as needed)\n"
            "        const url = \"http://localhost:8080/clients\""
            "        // Using the Fetch API to perform the HTTP GET request.\n"
            "       fetch(url , {"
            "           method: \"POST\","
            "           body: JSON.stringify({"
            "               row: rowNumber"
            "               }),"
            "           headers: {"
            "               \"Content-type\": \"application/json; charset=UTF-8\""
            "           }})"
            "          .then(response => {"
            "            if (!response.ok) {"
            "              throw new Error(\"Network response was not ok\");"
            "            }"
            "            // Assuming the response is in JSON format.\n"
            "            return response.text();"
            "          })"
            "          .then(data => {"
            "            // Process the returned data. Here we log it and display an alert.\n"
            "            console.log(\"Data received:\", data);"
            "            alert(\"HTTP Request successful: \" + JSON.stringify(data));"
            "          })"
            "          .catch(error => {"
            "            console.error(\"There was a problem with the fetch operation:\", error);"
            "            alert(\"HTTP Request failed: \" + error.message);"
            "          });"
            "      }"
            "    </script>"
            "  </head>"
            "  <body>"
            "    <h1>List of all Connected Clients</h1>"
            "    <table>"
            "      <thead>"
            "        <tr><th>Client ID</th><th>Connection</th><th>Locale Address</th><th>Remote Address</th><th>Action</th></tr>"
            "      </thead>"
            "      <tbody>";

        for (const auto& [mqtt, connectPacket] : mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClients()) {
            core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();
            mqtt->getConnectionName();

            responseString += "<tr>"
                              "  <td>" +
                              mqtt->getClientId() +
                              "</td>"
                              "  <td>" +
                              socketConnection->getConnectionName() +
                              "  </td>"
                              "  <td>" +
                              socketConnection->getLocalAddress().toString() +
                              "  </td>"
                              "  <td>" +
                              socketConnection->getRemoteAddress().toString() +
                              "  </td>"
                              "  <td>"
                              "    <button onclick=\"executeCode('" +
                              mqtt->getConnectionName() +
                              "')"
                              "\">"
                              "Click Me"
                              "</button>"
                              "  </td>"
                              "</tr>";
        }
        responseString += "      </tbody>"
                          "    </table>"
                          "  </body>"
                          "</html>";

        res->set("Access-Control-Allow-Origin", "*");

        res->send(responseString);
    });

    router.post("/clients", [] APPLICATION(req, res) {
        VLOG(0) << "-----------------------------";
        VLOG(0) << std::string(req->body.begin(), req->body.end());
        VLOG(0) << "-----------------------------";
    });

    router.get("/ws/", [] APPLICATION(req, res) {
        upgrade(req, res);
    });

    router.get("/", [] APPLICATION(req, res) {
        upgrade(req, res);
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
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
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
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
          typename SocketAddress = typename Server::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<
              std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>, typename Server::Config&>>,
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
        config.setSunPath("/tmp/" + utils::Config::getApplicationName());
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

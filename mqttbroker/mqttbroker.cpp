/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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
//
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
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

static express::Router getRouter() {
    const express::Router router;
    router.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClients();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Mqtt Broker</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Locale Address</th><th>Remote Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString += "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getLocalAddress().toString() +
                              "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
        }

        responseString += "    </table>"
                          "  </body>"
                          "</html>";

        res->send(responseString);
    });

    router.get("/ws/", [] APPLICATION(req, res) -> void {
        if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
            res->upgrade(req, [req](bool success) -> void {
                if (success) {
                    VLOG(1) << "Successful upgrade to '" << req->get("upgrade") << "'";
                } else {
                    VLOG(1) << "Can not upgrade to '" << req->get("upgrade") << "'";
                }
            });
        } else {
            res->sendStatus(404);
        }
    });

    router.get("/", [] APPLICATION(req, res) -> void {
        if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
            res->upgrade(req, [req](bool success) -> void {
                if (success) {
                    VLOG(1) << "Successful upgrade to '" << req->get("upgrade") << "'";
                } else {
                    VLOG(1) << "Can not upgrade to '" << req->get("upgrade") << "'";
                }
            });
        } else {
            res->sendStatus(404);
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
            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
        case core::socket::State::FATAL:
            LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startServer(const std::string& instanceName, const auto& configurator, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Server::SocketAddress;

    const Server server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(server.getConfig());

    server.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketServer<SocketContextFactory>::Config&>>>
void startServer(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Server::SocketAddress;

    const Server server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    server.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <typename HttpServer>
void startServer(const std::string& instanceName, const std::function<void(typename HttpServer::Config&)>& configurator) {
    using SocketAddress = typename HttpServer::SocketAddress;

    const HttpServer httpServer(instanceName, getRouter());

    configurator(httpServer.getConfig());

    httpServer.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    startServer<net::in::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtt", [](auto& config) -> void {
        config.setPort(1883);
        config.setRetry();
    });

    startServer<net::in::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtts", [](auto& config) -> void {
        config.setPort(8883);
        config.setRetry();
    });

    startServer<net::un::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("un-mqtt", [](auto& config) -> void {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName());
        config.setRetry();
    });

    startServer<express::legacy::in::WebApp>("in-http", [](auto& config) -> void {
        config.setPort(8080);
        config.setRetry();
    });

    startServer<express::tls::in::WebApp>("in-https", [](auto& config) -> void {
        config.setPort(8088);
        config.setRetry();
    });

    return core::SNodeC::start();
}

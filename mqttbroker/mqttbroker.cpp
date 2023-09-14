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

#include "SharedSocketContextFactory.h" // IWYU pragma: keep
#include "lib/Mqtt.h"
#include "lib/MqttModel.h"

#include <core/SNodeC.h>
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <net/in/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <net/in/stream/tls/SocketServer.h>    // IWYU pragma: keep
#include <net/un/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <utils/Config.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <log/Logger.h>

#endif

namespace iot::mqtt::packets {
    class Connect;
}

template <typename Server>
void doListen(Server server, bool reconnect = false) {
    if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
        server.listen([server, reconnect](const typename Server::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum == 0) {
                VLOG(0) << "Server instance '" << server.getConfig().getInstanceName() << "' listening on " << socketAddress.toString();
            } else {
                PLOG(ERROR) << "Server instance '" << server.getConfig().getInstanceName() << "' listening on " << socketAddress.toString();
                if (reconnect) {
                    LOG(INFO) << "  ... retrying";
                    core::timer::Timer::singleshotTimer(
                        [server, reconnect]() mutable -> void {
                            doListen(server, reconnect);
                        },
                        1);
                }
            }
        });
    }
}

express::Router getRouter();
express::Router getRouter() {
    express::Router router;
    router.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
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

        res.send(responseString);
    });

    router.get("/ws/", [] APPLICATION(req, res) -> void {
        if (!res.upgrade(req)) {
            res.end();
        }
    });

    router.get("/", [] APPLICATION(req, res) -> void {
        if (!res.upgrade(req)) {
            res.end();
        }
    });

    return router;
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    {
        using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        MQTTLegacyInServer mqttLegacyInServer("legacyin");
        doListen(mqttLegacyInServer, true);

        using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        MQTTTLSInServer mqttTLSInServer("tlsin");
        doListen(mqttTLSInServer, true);

        using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        MQTTLegacyUnServer mqttLegacyUnServer("legacyun");
        doListen(mqttLegacyUnServer, true);

        using MQTTTLSWebView = express::tls::in::WebApp;
        MQTTTLSWebView mqttTLSWebView("mqtttlswebview", getRouter());
        doListen(mqttTLSWebView, true);

        using MQTTLegacyWebView = express::legacy::in::WebApp;
        MQTTLegacyWebView mqttLegacyWebView("mqttlegacywebview", mqttTLSWebView);
        doListen(mqttLegacyWebView, true);
    }

    return core::SNodeC::start();
}

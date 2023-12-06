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
#include <type_traits>

#endif

namespace iot::mqtt::packets {
    class Connect;
}

express::Router getRouter();
express::Router getRouter() {
    express::Router router;
    router.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClinets();

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

template <typename SocketAddress, typename = std::enable_if_t<std::is_base_of_v<core::socket::SocketAddress, SocketAddress>>>
void reportState(const std::string& instanceName, const SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "': " << state.what();
            break;
        case core::socket::State::DISABLED:
            VLOG(1) << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": error occurred";
            VLOG(1) << "    " << state.what();
            break;
        case core::socket::State::FATAL:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": fatal error occurred";
            VLOG(1) << "    " << state.what();
            break;
    }
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);
    {
        using MqttBroker = net::in::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        using SocketAddress = MqttBroker::SocketAddress;

        const MqttBroker mqttBroker("in-mqtt");
        mqttBroker.getConfig().setRetry();
        mqttBroker.listen(1883, [](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
            reportState("in-mqtt", socketAddress, state);
        });
    }

    {
        using MqttBroker = net::in::stream::tls::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        using SocketAddress = MqttBroker::SocketAddress;

        const MqttBroker mqttBroker("in-mqtts");
        mqttBroker.getConfig().setRetry();
        mqttBroker.listen(8883, [](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
            reportState("in-mqtts", socketAddress, state);
        });
    }

    {
        using MqttBroker = net::un::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;
        using SocketAddress = MqttBroker::SocketAddress;

        const MqttBroker mqttBroker("un-mqtt");
        mqttBroker.getConfig().setRetry();
        mqttBroker.listen("/tmp/" + utils::Config::getApplicationName(),
                          [](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                              reportState("un-mqtt", socketAddress, state);
                          });
    }

    {
        using MqttBroker = express::legacy::in::WebApp;
        using SocketAddress = MqttBroker::SocketAddress;

        const MqttBroker mqttBroker("in-http", getRouter());
        mqttBroker.getConfig().setRetry();
        mqttBroker.listen(8080, [](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
            reportState("in-http", socketAddress, state);
        });
    }

    {
        using MqttBroker = express::tls::in::WebApp;
        using SocketAddress = MqttBroker::SocketAddress;

        const MqttBroker mqttBroker("in-https", getRouter());
        mqttBroker.getConfig().setRetry();
        mqttBroker.listen(8088, [](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
            reportState("in-https", socketAddress, state);
        });
    }

    return core::SNodeC::start();
}

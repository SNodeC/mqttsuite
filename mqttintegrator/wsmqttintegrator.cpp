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

#include <core/SNodeC.h>
#include <core/timer/Timer.h>
#include <web/http/client/Request.h>
#include <web/http/client/Response.h>
#include <web/http/legacy/in/Client.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <log/Logger.h>
#include <utils/Config.h>

#endif

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

template <typename Client>
void doConnect(Client& client, bool reconnect = false) {
    if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
        client.connect([&client, reconnect](const typename Client::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum == 0) {
                VLOG(0) << "Client Instance '" << client.getConfig().getInstanceName() << "' connected to " << socketAddress.toString();
            } else {
                PLOG(ERROR) << "Client Instance '" << client.getConfig().getInstanceName() << "' connecting to "
                            << socketAddress.toString();
                if (reconnect) {
                    LOG(INFO) << "  ... retrying";
                    core::timer::Timer::singleshotTimer(
                        [&client]() -> void {
                            doConnect(client, true);
                        },
                        1);
                }
            }
        });
    }
}

int main(int argc, char* argv[]) {
#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    using WsMqttLegacyIntegrator = web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>;
    WsMqttLegacyIntegrator wsMqttLegacyIntegrator(
        "legacy",
        [](web::http::client::Request& request) -> void {
            request.set("Sec-WebSocket-Protocol", "mqtt");

            request.upgrade("/ws/", "websocket");
        },
        [](web::http::client::Request& request, web::http::client::Response& response) -> void {
            response.upgrade(request);
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "OnResponseError";
            VLOG(0) << "     Status: " << status;
            VLOG(0) << "     Reason: " << reason;
        });

    wsMqttLegacyIntegrator.onDisconnect([&wsMqttLegacyIntegrator](WsMqttLegacyIntegrator::SocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect";

        VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

        doConnect(wsMqttLegacyIntegrator, true);
    });

    doConnect(wsMqttLegacyIntegrator, true);

    return core::SNodeC::start();
}

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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
//
#include <web/http/legacy/in/Client.h>
#include <web/http/tls/in/Client.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <cstdlib>
#include <string>
#include <type_traits>

#endif

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

template <typename SocketAddress, typename = std::enable_if_t<std::is_base_of_v<core::socket::SocketAddress, SocketAddress>>>
void reportState(const std::string& instanceName, const SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
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

template <typename HttpClient>
void startClient(const std::string& name, const auto& configurator) {
    using Client = HttpClient;
    using SocketAddress = typename Client::SocketAddress;

    const Client client(
        name,
        [](const std::shared_ptr<web::http::client::Request>& req) -> void {
            req->set("Sec-WebSocket-Protocol", "mqtt");

            req->upgrade("/ws/",
                         "websocket",
                         [](const std::shared_ptr<web::http::client::Request>& req,
                            const std::shared_ptr<web::http::client::Response>& res) -> void {
                             req->upgrade(res, [&subProtocol = res->headers["upgrade"]](bool success) -> void {
                                 if (success) {
                                     VLOG(1) << "Successful upgrade to '" << subProtocol << "'";
                                 } else {
                                     VLOG(1) << "Can not upgrade to '" << subProtocol << "'";
                                 }
                             });
                         });
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "OnResponseError";
            VLOG(0) << "     Status: " << status;
            VLOG(0) << "     Reason: " << reason;
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) -> void {
            VLOG(0) << "Session ended";
        });

    configurator(client.getConfig());

    client.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(name, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](auto& config) -> void {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });

    startClient<web::http::tls::in::Client>("in-wsmqtts", [](auto& config) -> void {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });

    return core::SNodeC::start();
}

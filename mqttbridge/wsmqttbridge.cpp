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

#include "lib/BridgeStore.h"

#include <core/SNodeC.h>
#include <web/http/client/Request.h>
#include <web/http/client/Response.h>
#include <web/http/legacy/in/Client.h>
#include <web/http/tls/in/Client.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <utils/Config.h>

// IWYU pragma: no_include <bits/utility.h>

#endif

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

template <typename SocketAddressT, typename = std::enable_if_t<std::is_base_of_v<core::socket::SocketAddress, SocketAddressT>>>
void reportState(const std::string& instanceName, const SocketAddressT& socketAddress, const core::socket::State& state) {
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
#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    utils::Config::add_string_option("--bridge-config", "MQTT mapping file (json format) for integration", "[path]");

    core::SNodeC::init(argc, argv);

    const bool success =
        mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(utils::Config::get_string_option_value("--bridge-config"));

    if (success) {
        for (const auto& [instanceName, brokerJsonConfig] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
            const std::string& name = brokerJsonConfig["name"];
            const std::string& protocol = brokerJsonConfig["protocol"];
            const std::string& encryption = brokerJsonConfig["encryption"];
            const std::string& transport = brokerJsonConfig["transport"];

            if (transport == "websocket") {
                if (protocol == "in") {
                    if (encryption == "legacy") {
                        {
                            using WsIntegrator = web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>;
                            using SocketAddress = WsIntegrator::SocketAddress;

                            WsIntegrator wsIntegrator(
                                name,
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

                            wsIntegrator.getConfig().Remote::setPort(8080);
                            wsIntegrator.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                                reportState(name, socketAddress, state);
                            });
                        }
                    } else if (encryption == "tls") {
                        using WsIntegrator = web::http::tls::in::Client<web::http::client::Request, web::http::client::Response>;
                        using SocketAddress = WsIntegrator::SocketAddress;

                        WsIntegrator wsIntegrator(
                            name,
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

                        wsIntegrator.getConfig().setDisabled();
                        wsIntegrator.getConfig().Remote::setPort(8088);
                        wsIntegrator.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
                            reportState(name, socketAddress, state);
                        });
                    } else {
                        VLOG(0) << "Ignoring: " << transport << "::" << protocol << "::" << encryption;
                    }
                } else {
                    VLOG(0) << "Ignoring: " << transport << "::" << protocol;
                }
            } else {
                VLOG(0) << "Ignoring: " << transport;
            }
        }
    }

    return core::SNodeC::start();
}

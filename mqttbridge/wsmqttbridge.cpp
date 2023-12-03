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

#include <cstdlib>
#include <log/Logger.h>
#include <type_traits>
#include <utils/CLI11.hpp>
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
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "': " << state.what();
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

template <template <typename, typename> typename HttpClient>
void startClient(const std::string& name,
                 const std::function<void(HttpClient<web::http::client::Request, web::http::client::Response>&)>& configurator) {
    using WsIntegrator = HttpClient<web::http::client::Request, web::http::client::Response>;
    using SocketAddress = typename WsIntegrator::SocketAddress;

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
    wsIntegrator.getConfig().setRetry();
    wsIntegrator.getConfig().setRetryBase(1);
    wsIntegrator.getConfig().setReconnect();
    configurator(wsIntegrator);
    wsIntegrator.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
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

    utils::Config::add_string_option("--bridge-config", "MQTT bridge configuration file (JSON format)", "[path]")->envname("BRIDGE_CONFIG");

    core::SNodeC::init(argc, argv);

    setenv("BRIDGE_CONFIG", utils::Config::get_string_option_value("--bridge-config").data(), 1);

    bool success = mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(utils::Config::get_string_option_value("--bridge-config"));

    if (success) {
        success = setenv("BRIDGE_CONFIG", utils::Config::get_string_option_value("--bridge-config").data(), 1) == 0;

        if (success) {
            for (const auto& [instanceName, broker] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
                if (!broker.getInstanceName().empty()) {
                    if (broker.getTransport() == "websocket") {
                        if (broker.getProtocol() == "in") {
                            if (broker.getEncryption() == "legacy") {
                                startClient<web::http::legacy::in::Client>(broker.getInstanceName(), [](auto& mqttBridge) -> void {
                                    mqttBridge.getConfig().Remote::setPort(8080);
                                });
                            } else if (broker.getEncryption() == "tls") {
                                startClient<web::http::tls::in::Client>(broker.getInstanceName(), [](auto& mqttBridge) -> void {
                                    mqttBridge.getConfig().Remote::setPort(8088);
                                });
                            } else {
                                VLOG(2) << "Ignoring: " << broker.getTransport() << "::" << broker.getProtocol()
                                        << "::" << broker.getEncryption();
                            }
                        } else {
                            VLOG(2) << "Ignoring: " << broker.getTransport() << "::" << broker.getProtocol();
                        }
                    } else {
                        VLOG(2) << "Ignoring: " << broker.getTransport();
                    }
                }
            }
        }
    }

    return core::SNodeC::start();
}

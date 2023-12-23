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

#include "SocketContextFactory.h" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <core/socket/stream/SocketContextFactory.h> // IWYU pragma: keep
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/tls/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
//
#include <cstdlib>
#include <log/Logger.h>
#include <string>
#include <type_traits>
#include <utility>
#include <utils/Config.h>
#include <variant>

// IWYU pragma: no_include <bits/utility.h>

#endif

template <typename SocketAddress>
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

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startClient(const std::string& instanceName, const auto& configurator, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Client::SocketAddress;

    const Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(client.getConfig());

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketClient<SocketContextFactory>::Config&>>>
void startClient(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    using Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>;
    using SocketAddress = typename Client::SocketAddress;

    const Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    startClient<net::in::stream::legacy::SocketClient, mqtt::mqttintegrator::SocketContextFactory>("in-mqtt", [](auto& config) -> void {
        config.Remote::setPort(1883);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });

    startClient<net::in::stream::tls::SocketClient, mqtt::mqttintegrator::SocketContextFactory>("in-mqtts", [](auto& config) -> void {
        config.setDisabled();
        config.Remote::setPort(8883);

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });

    startClient<net::un::stream::legacy::SocketClient, mqtt::mqttintegrator::SocketContextFactory>("un-mqtt", [](auto& config) -> void {
        config.setDisabled();
        config.Remote::setSunPath("/tmp/mqttbroker");

        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
    });

    return core::SNodeC::start();
}

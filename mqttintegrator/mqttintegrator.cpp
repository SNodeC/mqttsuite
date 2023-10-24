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

#include <core/SNodeC.h>
#include <net/in/stream/tls/SocketClient.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <log/Logger.h>
#include <string>
#include <utils/Config.h>

#endif

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    {
        using InMqttTlsIntegrator = net::in::stream::tls::SocketClient<mqtt::mqttintegrator::SocketContextFactory>;
        using SocketAddress = InMqttTlsIntegrator::SocketAddress;

        InMqttTlsIntegrator inMqttTlsIntegrator("mqtttlsintegrator");

        inMqttTlsIntegrator.connect([](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "mqtttlsintegrator: connected to '" << socketAddress.toString() << "': " << state.what();
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "mqtttlsintegrator: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "mqtttlsintegrator: " << socketAddress.toString() << ": non critical error occurred";
                    VLOG(1) << "    " << state.what();
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "mqtttlsintegrator: " << socketAddress.toString() << ": critical error occurred";
                    VLOG(1) << "    " << state.what();
                    break;
            }
        });
    }

    return core::SNodeC::start();
}

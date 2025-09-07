/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SocketContextFactory.h" // IWYU pragma: keep

#ifdef LINK_SUBPROTOCOL_STATIC

#include "websocket/SubProtocolFactory.h"

#include <web/websocket/client/SubProtocolFactorySelector.h>

#endif

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)

#include <web/websocket/client/SocketContextUpgradeFactory.h>

#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
//
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/tls/SocketClient.h>
#include <net/in6/stream/legacy/SocketClient.h>
#include <net/in6/stream/tls/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <net/un/stream/tls/SocketClient.h>
#include <web/http/legacy/in/Client.h>
#include <web/http/legacy/in6/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <utils/CLI11.hpp>
//
#include <string>

#endif

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
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

template <typename HttpClient>
void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::Request>& req) {
            req->set("Sec-WebSocket-Protocol", "mqtt");

            if (!req->upgrade(
                    "/ws",
                    "websocket",
                    [](const std::shared_ptr<web::http::client::Request>& req, const std::shared_ptr<web::http::client::Response>& res) {
                        req->upgrade(res, [subProtocolsRequested = req->header("Upgrade")](const std::string& name) {
                            if (!name.empty()) {
                                VLOG(1) << "Successful upgrade to '" << name << "' requested: " << subProtocolsRequested;
                            } else {
                                VLOG(1) << "Can not upgrade to any of '" << subProtocolsRequested << "'";
                            }
                        });
                    },
                    [](const std::shared_ptr<web::http::client::Request>& req, const std::string& reason) {
                        VLOG(1) << "Upgrade to subprotocols '" << req->header("Upgrade")
                                << "' failed with response parse error: " << reason;
                    })) {
                VLOG(1) << "Initiating upgrade to any of 'upgradeprotocol, websocket' failed";
            }
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
            VLOG(1) << "Session ended";
        });

    configurator(httpClient.getConfig());

    httpClient.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(name, socketAddress, state);
    });
}

static void createConfig(CLI::App* sessionApp, CLI::App* subApp, CLI::App* pubApp) {
    sessionApp->configurable(false);
    subApp->configurable(false);
    pubApp->configurable(false);

    CLI::Option* clientIdOpt = sessionApp->add_option("--client-id", "MQTT Client-ID")
                                   ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
                                   ->type_name("[string]");

    sessionApp->add_option("--qos", "Quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[uint8_t]")
        ->default_val(0)
        ->configurable();

    sessionApp->add_flag("--retain-session{true},-r{true}", "Clean session")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[bool]")
        ->default_str("false")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable()
        ->needs(clientIdOpt);

    sessionApp->add_option("--keep-alive", "Quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[uint8_t]")
        ->default_val(60)
        ->configurable();

    sessionApp->add_option("--will-topic", "MQTT will topic")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[string]")
        ->configurable();

    sessionApp->add_option("--will-message", "MQTT will message")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[string]")
        ->configurable();

    sessionApp->add_option("--will-qos", "MQTT will quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[uint8_t]")
        ->default_val(0)
        ->configurable();

    sessionApp->add_flag("--will-retain{true}", "MQTT will message retain")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->type_name("[bool]")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable();

    sessionApp->add_option("--username", "MQTT username")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[string]")
        ->configurable();

    sessionApp->add_option("--password", "MQTT password")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("[string]")
        ->configurable();

    subApp->add_option("--topic", "List of topics subscribing to")
        ->group(subApp->get_formatter()->get_label("Persistent Options"))
        ->default_str("#")
        ->type_name("[string list]")
        ->take_all()
        ->allow_extra_args()
        ->configurable();

    pubApp->needs(pubApp->add_option("--topic", "Topic publishing to")
                      ->group(pubApp->get_formatter()->get_label("Persistent Options"))
                      ->type_name("[string]")
                      ->required()
                      ->configurable());

    pubApp->needs(pubApp->add_option("--message", "Message to publish")
                      ->group(pubApp->get_formatter()->get_label("Persistent Options"))
                      ->type_name("[string]")
                      ->required()
                      ->configurable());

    pubApp->add_flag("--retain{true},-r{true}", "Retain message")
        ->group(pubApp->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->type_name("[bool]")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable();
}

static void createConfig(net::config::ConfigInstance& config) {
    createConfig(config.addSection("session", "MQTT session behavior", "Connection"),
                 config.addSection("sub", "Configuration for application mqttsub", "Applications"),
                 config.addSection("pub", "Configuration for application mqttpub", "Applications"));
}

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    utils::Config::app->get_formatter()->label("SUBCOMMAND", "APPLICATION | CONNECTION | INSTANCE");
    utils::Config::app->get_formatter()->label("SUBCOMMANDS", "APPLICATION | CONNECTION | INSTANCES");

    CLI::App* sessionApp = utils::Config::addInstance("session", "MQTT session behavior", "Connection");
    CLI::App* subApp = utils::Config::addInstance("sub", "Configuration for application mqttsub", "Applications");
    CLI::App* pubApp = utils::Config::addInstance("pub", "Configuration for application mqttpub", "Applications");

    createConfig(sessionApp, subApp, pubApp);

    // Start of application

    net::in::stream::legacy::Client<mqtt::mqtt::SocketContextFactory>(
        "in-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setDisableNagleAlgorithm();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in-mqtt", socketAddress, state);
        });

    net::in::stream::tls::Client<mqtt::mqtt::SocketContextFactory>(
        "in-mqtts",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setDisableNagleAlgorithm();
            config.setDisabled();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in-mqtts", socketAddress, state);
        });

    net::in6::stream::legacy::Client<mqtt::mqtt::SocketContextFactory>(
        "in6-mqtt",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setDisableNagleAlgorithm();
            config.setDisabled();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in6-mqtt", socketAddress, state);
        });

    net::in6::stream::tls::Client<mqtt::mqtt::SocketContextFactory>(
        "in6-mqtts",
        [](auto& config) {
            config.Remote::setPort(1883);

            config.setRetry();
            config.setRetryBase(1);
            config.setDisableNagleAlgorithm();
            config.setDisabled();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("in6-mqtts", socketAddress, state);
        });

    net::un::stream::legacy::Client<mqtt::mqtt::SocketContextFactory>(
        "un-mqtt",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtt");

            config.setRetry();
            config.setRetryBase(1);
            config.setDisabled();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("un-mqtt", socketAddress, state);
        });

    net::un::stream::tls::Client<mqtt::mqtt::SocketContextFactory>(
        "un-mqtts",
        [](auto& config) {
            config.Remote::setSunPath("/var/mqttbroker-un-mqtts");

            config.setRetry();
            config.setRetryBase(1);
            config.setDisabled();

            createConfig(config);
        },
        sessionApp,
        subApp,
        pubApp)
        .connect([](const auto& socketAddress, const core::socket::State& state) {
            reportState("un-mqtts", socketAddress, state);
        });

    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    });

    startClient<web::http::tls::in::Client>("in-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    });

    startClient<web::http::legacy::in6::Client>("in6-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    });

    startClient<web::http::tls::in6::Client>("in6-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setReconnect();
        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    });

    return core::SNodeC::start();
}

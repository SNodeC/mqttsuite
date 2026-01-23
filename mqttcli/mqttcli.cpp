/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
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
#include "config.h"

#include <web/http/http_utils.h>

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
#include <web/http/legacy/un/Client.h>
#include <web/http/tls/in/Client.h>
#include <web/http/tls/in6/Client.h>
#include <web/http/tls/un/Client.h>
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

static void logResponse(const std::shared_ptr<web::http::client::Request>& req, const std::shared_ptr<web::http::client::Response>& res) {
    VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " HTTP response for: " << req->method << " "
            << req->url << " HTTP/" << req->httpMajor << "." << req->httpMinor << "\n"
            << httputils::toString(req->method,
                                   req->url,
                                   "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                   req->getQueries(),
                                   req->getHeaders(),
                                   req->getTrailer(),
                                   req->getCookies(),
                                   {})
            << "\n"
            << httputils::toString(res->httpVersion, res->statusCode, res->reason, res->headers, res->cookies, res->body);
}

template <typename HttpClient>
void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::MasterRequest>& req) {
            const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();
            const std::string target = req->getSocketContext()
                                           ->getSocketConnection()
                                           ->getConfigInstance()
                                           ->getSection("http")
                                           ->get_option("--target")
                                           ->as<std::string>();

            req->set("Sec-WebSocket-Protocol", "mqtt");

            req->upgrade(
                target,
                "websocket",
                [connectionName](bool success) {
                    VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket||"
                            << "mqtt" << ") start " << (success ? "success" : "failed");
                },
                [connectionName](const std::shared_ptr<web::http::client::Request>& req,
                                 const std::shared_ptr<web::http::client::Response>& res,
                                 bool success) {
                    logResponse(req, res);

                    VLOG(1) << connectionName << ": HTTP Upgrade " << (success ? "success" : "failed");
                },
                [connectionName]([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req, const std::string& message) {
                    VLOG(1) << connectionName << ": Response parse error: " << message;
                    VLOG(1) << "  Request was: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "." << req->httpMinor
                            << "\n"
                            << httputils::toString(req->method,
                                                   req->url,
                                                   "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                                   req->getQueries(),
                                                   req->getHeaders(),
                                                   req->getTrailer(),
                                                   req->getCookies(),
                                                   {})
                            << "\n";
                });
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
                                   ->type_name("string");

    sessionApp->add_option("--qos", "Quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(0)
        ->configurable();

    sessionApp->add_flag("--retain-session{true},-r{true}", "Clean session")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("bool")
        ->default_str("false")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable()
        ->needs(clientIdOpt);

    sessionApp->add_option("--keep-alive", "Quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(60)
        ->configurable();

    sessionApp->add_option("--will-topic", "MQTT will topic")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    sessionApp->add_option("--will-message", "MQTT will message")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    sessionApp->add_option("--will-qos", "MQTT will quality of service")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(0)
        ->configurable();

    sessionApp->add_flag("--will-retain{true}", "MQTT will message retain")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->type_name("bool")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable();

    sessionApp->add_option("--username", "MQTT username")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    sessionApp->add_option("--password", "MQTT password")
        ->group(sessionApp->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    subApp->needs(subApp
                      ->add_option_function<std::string>(
                          "--topic",
                          [subApp](const std::string& value) {
                              if (value == "") {
                                  subApp->get_option("--topic")->required(false)->clear();
                                  subApp->remove_needs(subApp->get_option("--topic"));
                              }
                          },
                          "List of topics subscribing to")
                      ->group(subApp->get_formatter()->get_label("Persistent Options"))
                      ->type_name("string list")
                      ->take_all()
                      ->required()
                      ->allow_extra_args()
                      ->configurable());

    pubApp->needs(pubApp
                      ->add_option_function<std::string>(
                          "--topic",
                          [pubApp](const std::string& value) {
                              if (value == "") {
                                  pubApp->get_option("--topic")->required(false)->clear();
                                  pubApp->remove_needs(pubApp->get_option("--topic"));

                                  pubApp->get_option("--message")->required(false)->clear();
                                  pubApp->remove_needs(pubApp->get_option("--message"));
                              }
                          },
                          "Topic publishing to")
                      ->group(pubApp->get_formatter()->get_label("Persistent Options"))
                      ->type_name("string")
                      ->required()
                      ->configurable());

    pubApp->needs(pubApp
                      ->add_option_function<std::string>(
                          "--message",
                          [pubApp](const std::string& value) {
                              if (value == "") {
                                  pubApp->get_option("--topic")->required(false)->clear();
                                  pubApp->remove_needs(pubApp->get_option("--topic"));

                                  pubApp->get_option("--message")->required(false)->clear();
                                  pubApp->remove_needs(pubApp->get_option("--message"));
                              }
                          },
                          "Message to publish")
                      ->group(pubApp->get_formatter()->get_label("Persistent Options"))
                      ->type_name("string")
                      ->required()
                      ->configurable());

    pubApp->add_flag("--retain{true},-r{true}", "Retain message")
        ->group(pubApp->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->type_name("bool")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable();
}

static void createConfig(net::config::ConfigInstance& config) {
    createConfig(config.addSection("session", "MQTT session behavior", "Connection"),
                 config.addSection("sub", "Configuration for application mqttsub", "Applications"),
                 config.addSection("pub", "Configuration for application mqttpub", "Applications"));

    config.get()->require_callback([config = &config]() {
        if (!config->getDisabled() && utils::Config::showConfigTriggerApp == nullptr &&
            utils::Config::app->get_option("--write-config")->count() == 0) {
            CLI::App* pubApp = config->getSection("pub", true, true);
            CLI::App* subApp = config->getSection("sub", true, true);

            if ((pubApp == nullptr || (*pubApp)["--topic"]->count() == 0 || (*pubApp)["--message"]->count() == 0) &&
                (subApp == nullptr || (*subApp)["--topic"]->count() == 0)) {
                throw CLI::RequiresError(config->get()->get_parent()->get_name() + ":" + config->getInstanceName() +
                                             " requires at least one of {sub | pub}",
                                         CLI::ExitCodes::RequiresError);
            }

            if (pubApp != nullptr) {
                VLOG(0) << "[" << Color::Code::FG_LIGHT_GREEN << "Success" << Color::Code::FG_DEFAULT << "] " << "Bootstrap of "
                        << config->getInstanceName() << ":pub";
            }

            if (subApp != nullptr) {
                VLOG(0) << "[" << Color::Code::FG_LIGHT_GREEN << "Success" << Color::Code::FG_DEFAULT << "] " << "Bootstrap of "
                        << config->getInstanceName() << ":sub";
            }
        }
    });
}

static void createWSConfig(net::config::ConfigInstance& config) {
    createConfig(config);

    CLI::App* http = config.getSection("http");
    http->add_option("--target", "Websocket endpoint")
        ->group(http->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->default_str("/ws")
        ->configurable();
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

    createConfig(utils::Config::addInstance("session", "MQTT session behavior", "Connection", true),
                 utils::Config::addInstance("sub", "Configuration for application mqttsub", "Applications", true),
                 utils::Config::addInstance("pub", "Configuration for application mqttpub", "Applications", true));

    // Start of application

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV4)
    net::in::stream::legacy::Client<mqtt::mqttcli::SocketContextFactory>("in-mqtt", [](auto& config) {
        config.Remote::setPort(1883);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("in-mqtt", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV4)
    net::in::stream::tls::Client<mqtt::mqttcli::SocketContextFactory>("in-mqtts", [](auto& config) {
        config.Remote::setPort(1883);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("in-mqtts", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV6)
    net::in6::stream::legacy::Client<mqtt::mqttcli::SocketContextFactory>("in6-mqtt", [](auto& config) {
        config.Remote::setPort(1883);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("in6-mqtt", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV6)
    net::in6::stream::tls::Client<mqtt::mqttcli::SocketContextFactory>("in6-mqtts", [](auto& config) {
        config.Remote::setPort(1883);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("in6-mqtts", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX)
    net::un::stream::legacy::Client<mqtt::mqttcli::SocketContextFactory>("un-mqtt", [](auto& config) {
        config.Remote::setSunPath("/var/mqttbroker-un-mqtt");

        config.setRetry();
        config.setRetryBase(1);
        config.setDisabled();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("un-mqtt", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX_TLS)
    net::un::stream::tls::Client<mqtt::mqttcli::SocketContextFactory>("un-mqtts", [](auto& config) {
        config.Remote::setSunPath("/var/mqttbroker-un-mqtts");

        config.setRetry();
        config.setRetryBase(1);
        config.setDisabled();

        createConfig(config);
    }).connect([](const auto& socketAddress, const core::socket::State& state) {
        reportState("un-mqtts", socketAddress, state);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV4) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::in::Client>("in-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV4) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::in::Client>("in-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TCP_IPV6) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::in6::Client>("in6-wsmqtt", [](auto& config) {
        config.Remote::setPort(8080);

        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_TLS_IPV6) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::in6::Client>("in6-wsmqtts", [](auto& config) {
        config.Remote::setPort(8088);

        config.setReconnect();
        config.setRetry();
        config.setRetryBase(1);
        config.setDisableNagleAlgorithm();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX) && defined(CONFIG_MQTTSUITE_CLI_WS)
    startClient<web::http::legacy::un::Client>("un-wsmqtt", [](auto& config) {
        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

#if defined(CONFIG_MQTTSUITE_CLI_UNIX_TLS) && defined(CONFIG_MQTTSUITE_CLI_WSS)
    startClient<web::http::tls::un::Client>("un-wsmqtts", [](auto& config) {
        config.setRetry();
        config.setRetryBase(1);
        config.setReconnect();
        config.setDisabled();

        createWSConfig(config);
    });
#endif

    return core::SNodeC::start();
}

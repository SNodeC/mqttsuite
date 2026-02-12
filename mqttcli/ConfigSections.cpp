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

#include "ConfigSections.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
//
#include <net/config/ConfigSectionAPI.hpp>
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

ConfigSubscribe::ConfigSubscribe(net::config::ConfigInstance* instance)
    : net::config::ConfigSection(instance, net::config::Section("sub", "Configuration for application mqttsub", this)) {
    sectionSc->needs(addOptionFunction<std::string>(
                         "--topic",
                         [subApp = this](const std::string& value) {
                             if (value == "") {
                                 subApp->getOption("--topic")->required(false)->clear();
                                 subApp->sectionSc->remove_needs(subApp->getOption("--topic"));
                             }
                         },
                         "List of topics subscribing to")
                         ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("string list")
                         ->take_all()
                         ->required()
                         ->allow_extra_args()
                         ->configurable());
}

ConfigPublish::ConfigPublish(net::config::ConfigInstance* instance)
    : net::config::ConfigSection(instance, net::config::Section("pub", "Configuration for application mqttpub", this)) {
    sectionSc->needs(addOptionFunction<std::string>(
                         "--topic",
                         [pubApp = this](const std::string& value) {
                             if (value == "") {
                                 pubApp->getOption("--topic")->required(false)->clear();
                                 pubApp->sectionSc->remove_needs(pubApp->getOption("--topic"));

                                 pubApp->getOption("--message")->required(false)->clear();
                                 pubApp->sectionSc->remove_needs(pubApp->getOption("--message"));
                             }
                         },
                         "List of topics subscribing to")
                         ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("string list")
                         ->take_all()
                         ->required()
                         ->configurable());

    sectionSc->needs(addOptionFunction<std::string>(
                         "--message",
                         [pubApp = this](const std::string& value) {
                             if (value == "") {
                                 pubApp->getOption("--topic")->required(false)->clear();
                                 pubApp->sectionSc->remove_needs(pubApp->getOption("--topic"));

                                 pubApp->getOption("--message")->required(false)->clear();
                                 pubApp->sectionSc->remove_needs(pubApp->getOption("--message"));
                             }
                         },
                         "List of topics subscribing to")
                         ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("string list")
                         ->take_all()
                         ->required()
                         ->configurable());
}

ConfigSession::ConfigSession(net::config::ConfigInstance* instance)
    : net::config::ConfigSection(instance, net::config::Section("session", "MQTT session behavior", this)) {
    CLI::Option* clientIdOpt =
        addOption("--client-id", "MQTT Client-ID")->group(sectionSc->get_formatter()->get_label("Persistent Options"))->type_name("string");

    addOption("--qos", "Quality of service")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(0)
        ->configurable();

    addFlag("--retain-session{true},-r{true}", "Clean session", "bool")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable()
        ->needs(clientIdOpt);

    addOption("--keep-alive", "Quality of service")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(60)
        ->configurable();

    addOption("--will-topic", "MQTT will topic")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    addOption("--will-message", "MQTT will message")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    addOption("--will-qos", "MQTT will quality of service")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("uint8_t")
        ->default_val(0)
        ->configurable();

    addFlag("--will-retain{true}", "MQTT will message retain", "bool")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->default_str("false")
        ->check(CLI::IsMember({"true", "false"}))
        ->configurable();

    addOption("--username", "MQTT username")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();

    addOption("--password", "MQTT password")
        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
        ->type_name("string")
        ->configurable();
}

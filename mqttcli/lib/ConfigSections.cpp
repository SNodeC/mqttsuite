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

//
#include <net/config/ConfigSection.hpp>
//
#include <functional>
#include <memory>

#endif

namespace mqtt::mqttcli::lib {
    ConfigSubscribe::ConfigSubscribe(net::config::ConfigInstance* instance)
        : net::config::ConfigSection(instance, net::config::Section(ConfigSubscribe::name, "Configuration for application mqttsub", this)) {
        topicOpt = addOptionFunction<std::string>(
                       "--topic",
                       [configSubscribe = this](const std::string& value) {
                           if (value == "") {
                               configSubscribe->topicOpt->required(false)->clear();
                               configSubscribe->sectionSc->remove_needs(configSubscribe->topicOpt);
                           }
                       },
                       "List of topics subscribing to")
                       ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                       ->type_name("string list")
                       ->take_all()
                       ->required()
                       ->allow_extra_args()
                       ->configurable();
        sectionSc->needs(topicOpt);
    }

    std::list<std::string> ConfigSubscribe::getTopic() const {
        return topicOpt->as<std::list<std::string>>();
    }

    const ConfigSubscribe* ConfigSubscribe::setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return this;
    }

    ConfigPublish::ConfigPublish(net::config::ConfigInstance* instance)
        : net::config::ConfigSection(instance, net::config::Section(ConfigPublish::name, "Configuration for application mqttpub", this)) {
        topicOpt = addOptionFunction<std::string>(
                       "--topic",
                       [configPublish = this](const std::string& value) {
                           if (value == "") {
                               configPublish->topicOpt->required(false)->clear();
                               configPublish->sectionSc->remove_needs(configPublish->topicOpt);

                               configPublish->messageOpt->required(false)->clear();
                               configPublish->sectionSc->remove_needs(configPublish->messageOpt);
                           }
                       },
                       "List of topics subscribing to")
                       ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                       ->type_name("string list")
                       ->take_all()
                       ->required()
                       ->configurable();
        sectionSc->needs(topicOpt);

        messageOpt = addOptionFunction<std::string>(
                         "--message",
                         [configPublish = this](const std::string& value) {
                             if (value == "") {
                                 configPublish->topicOpt->required(false)->clear();
                                 configPublish->sectionSc->remove_needs(configPublish->topicOpt);

                                 configPublish->messageOpt->required(false)->clear();
                                 configPublish->sectionSc->remove_needs(configPublish->messageOpt);
                             }
                         },
                         "List of topics subscribing to")
                         ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("string list")
                         ->take_all()
                         ->required()
                         ->configurable();
        sectionSc->needs(messageOpt);

        retainOpt = addFlag("--retain{true}", "Message retain", "bool")
                        ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                        ->default_str("false")
                        ->check(CLI::IsMember({"true", "false"}))
                        ->configurable();
    }

    std::string ConfigPublish::getTopic() const {
        return topicOpt->as<std::string>();
    }

    const ConfigPublish* ConfigPublish::setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return this;
    }

    std::string ConfigPublish::getMessage() const {
        return messageOpt->as<std::string>();
    }

    const ConfigPublish* ConfigPublish::setMessage(const std::string& message) {
        messageOpt->default_val(message);

        return this;
    }

    bool ConfigPublish::getRetain() const {
        return retainOpt->as<bool>();
    }

    const ConfigPublish* ConfigPublish::setRetain(bool retain) {
        retainOpt->default_val(retain);

        return this;
    }

    ConfigSession::ConfigSession(net::config::ConfigInstance* instance)
        : net::config::ConfigSection(instance, net::config::Section(ConfigSession::name, "MQTT session behavior", this)) {
        clientIdOpt = addOption("--client-id", "MQTT Client-ID")
                          ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                          ->type_name("string");

        qoSOpt = addOption("--qos", "Quality of service")
                     ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                     ->type_name("uint8_t")
                     ->default_val(0)
                     ->configurable();

        retainSessionOpt = addFlag("--retain-session{true},-r{true}", "Clean session", "bool")
                               ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                               ->default_str("false")
                               ->check(CLI::IsMember({"true", "false"}))
                               ->configurable()
                               ->needs(clientIdOpt);

        keepAliveOpt = addOption("--keep-alive", "Quality of service")
                           ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                           ->type_name("uint16_t")
                           ->default_val(60)
                           ->configurable();

        willTopicOpt = addOption("--will-topic", "MQTT will topic")
                           ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                           ->type_name("string")
                           ->configurable();

        willMessageOpt = addOption("--will-message", "MQTT will message")
                             ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                             ->type_name("string")
                             ->configurable();

        willQoSOpt = addOption("--will-qos", "MQTT will quality of service")
                         ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("uint8_t")
                         ->default_val(0)
                         ->configurable();

        willRetainOpt = addFlag("--will-retain{true}", "MQTT will message retain", "bool")
                            ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                            ->default_str("false")
                            ->check(CLI::IsMember({"true", "false"}))
                            ->configurable();

        usernameOpt = addOption("--username", "MQTT username")
                          ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                          ->type_name("string")
                          ->configurable();

        passwordOpt = addOption("--password", "MQTT password")
                          ->group(sectionSc->get_formatter()->get_label("Persistent Options"))
                          ->type_name("string")
                          ->configurable();
    }

    std::string ConfigSession::getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    const ConfigSession* ConfigSession::setClientId(const std::string& clientId) const {
        clientIdOpt->default_val(clientId)->clear();

        return this;
    }

    uint8_t ConfigSession::getQoS() const {
        return qoSOpt->as<uint8_t>();
    }

    const ConfigSession* ConfigSession::setQos(uint8_t qoS) const {
        qoSOpt->default_val(qoS)->clear();

        return this;
    }

    bool ConfigSession::getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    const ConfigSession* ConfigSession::setRetainSession(bool retainSession) const {
        retainSessionOpt->default_val(retainSession)->clear();

        return this;
    }

    uint16_t ConfigSession::getKeepAlive() const {
        return keepAliveOpt->as<uint16_t>();
    }

    const ConfigSession* ConfigSession::setKeepAlive(uint16_t keepAlive) const {
        keepAliveOpt->default_val(keepAlive)->clear();

        return this;
    }

    std::string ConfigSession::getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    const ConfigSession* ConfigSession::setWillTopic(const std::string& willTopic) const {
        willTopicOpt->default_val(willTopic)->clear();

        return this;
    }

    std::string ConfigSession::getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    const ConfigSession* ConfigSession::getWillMessage(const std::string& willMessage) const {
        willMessageOpt->default_val(willMessage)->clear();

        return this;
    }

    uint8_t ConfigSession::getWillQoS() const {
        return willQoSOpt->as<uint8_t>();
    }

    const ConfigSession* ConfigSession::settWillQoS(uint8_t willQoS) const {
        willQoSOpt->default_val(willQoS)->clear();

        return this;
    }

    bool ConfigSession::getWillRetain() const {
        return willRetainOpt->as<bool>();
    }

    const ConfigSession* ConfigSession::setWillRetain(bool willRetain) const {
        willRetainOpt->default_val(willRetain)->clear();

        return this;
    }

    std::string ConfigSession::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    const ConfigSession* ConfigSession::settUsername(const std::string& username) const {
        usernameOpt->default_val(username)->clear();

        return this;
    }

    std::string ConfigSession::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    const ConfigSession* ConfigSession::setPassword(const std::string& password) const {
        passwordOpt->default_val(password)->clear();

        return this;
    }

} // namespace mqtt::mqttcli::lib

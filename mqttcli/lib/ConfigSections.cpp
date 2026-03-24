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

#endif

namespace mqtt::mqttcli::lib {
    ConfigSubscribe::ConfigSubscribe(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications (at least one required)")
        , topicOpt( //
              setConfigurable(addOption("--topic", "List of topics subscribing to", "string", CLI::TypeValidator<std::string>()), true)
                  ->take_all()) {
        required(topicOpt);

        required(false, true);
    }

    ConfigSubscribe::~ConfigSubscribe() = default;

    std::list<std::string> ConfigSubscribe::getTopic() const {
        std::list<std::string> topicList = topicOpt->as<std::list<std::string>>();

        if (topicList.front().empty()) {
            topicList.pop_front();
        }

        return topicList;
    }

    const ConfigSubscribe& ConfigSubscribe::setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return *this;
    }

    ConfigPublish::ConfigPublish(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications (at least one required)")
        , topicOpt( //
              setConfigurable(addOption("--topic", "List of topics subscribing to", "string", CLI::TypeValidator<std::string>()), true))
        , messageOpt( //
              setConfigurable(addOption("--message", "Message to be published", "string", CLI::TypeValidator<std::string>()), true))
        , retainOpt( //
              setConfigurable(addFlag("--retain{true}", "Message retain", "bool", "false", CLI::IsMember({"true", "false"})), true)) {
        required(messageOpt);
        required(topicOpt);

        required(false, true);
    }

    ConfigPublish::~ConfigPublish() = default;

    std::string ConfigPublish::getTopic() const {
        return topicOpt->as<std::string>();
    }

    const ConfigPublish& ConfigPublish::setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return *this;
    }

    std::string ConfigPublish::getMessage() const {
        return messageOpt->as<std::string>();
    }

    const ConfigPublish& ConfigPublish::setMessage(const std::string& message) {
        messageOpt->default_val(message);

        return *this;
    }

    bool ConfigPublish::getRetain() const {
        return retainOpt->as<bool>();
    }

    const ConfigPublish& ConfigPublish::setRetain(bool retain) {
        retainOpt->default_val(retain);

        return *this;
    }

    ConfigSession::ConfigSession(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications")
        , clientIdOpt( //
              setConfigurable(addOption("--client-id", "MQTT Client-ID", "string", CLI::TypeValidator<std::string>()), true))
        , qoSOpt( //
              setConfigurable(addOption("--qos", "Quality of service", "uint8_t", "0", CLI::Range(0, 2)), true))
        , retainSessionOpt( //
              setConfigurable(
                  addFlag("--retain-session{true},-r{true}", "Clean session", "bool", "false", CLI::IsMember({"true", "false"})), true)
                  ->needs(clientIdOpt))
        , keepAliveOpt( //
              setConfigurable(addOption("--keep-alive", "Quality of service", "uint16_t", "60", CLI::TypeValidator<uint16_t>()), true))
        , willTopicOpt( //
              setConfigurable(addOption("--will-topic", "MQTT will topic", "string", CLI::TypeValidator<std::string>()), true))
        , willMessageOpt( //
              setConfigurable(addOption("--will-message", "MQTT will message", "string", CLI::TypeValidator<std::string>()), true))
        , willQoSOpt( //
              setConfigurable(addOption("--will-qos", "MQTT will quality of service", "uint8_t", "0", CLI::TypeValidator<uint8_t>()), true))
        , willRetainOpt( //
              setConfigurable(addFlag("--will-retain{true}", "MQTT will message retain", "bool", "false", CLI::IsMember({"true", "false"})),
                              true))
        , usernameOpt( //
              setConfigurable(addOption("--username", "MQTT username", "string", CLI::TypeValidator<std::string>()), true))
        , passwordOpt( //
              setConfigurable(addOption("--password", "MQTT password", "string", CLI::TypeValidator<std::string>()), true)) {
    }

    ConfigSession::~ConfigSession() = default;

    std::string ConfigSession::getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setClientId(const std::string& clientId) const {
        clientIdOpt->default_val(clientId)->clear();

        return *this;
    }

    uint8_t ConfigSession::getQoS() const {
        return qoSOpt->as<uint8_t>();
    }

    const ConfigSession& ConfigSession::setQos(uint8_t qoS) const {
        qoSOpt->default_val(qoS)->clear();

        return *this;
    }

    bool ConfigSession::getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    const ConfigSession& ConfigSession::setRetainSession(bool retainSession) const {
        retainSessionOpt->default_val(retainSession)->clear();

        return *this;
    }

    uint16_t ConfigSession::getKeepAlive() const {
        return keepAliveOpt->as<uint16_t>();
    }

    const ConfigSession& ConfigSession::setKeepAlive(uint16_t keepAlive) const {
        keepAliveOpt->default_val(keepAlive)->clear();

        return *this;
    }

    std::string ConfigSession::getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setWillTopic(const std::string& willTopic) const {
        willTopicOpt->default_val(willTopic)->clear();

        return *this;
    }

    std::string ConfigSession::getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setWillMessage(const std::string& willMessage) const {
        willMessageOpt->default_val(willMessage)->clear();

        return *this;
    }

    uint8_t ConfigSession::getWillQoS() const {
        return willQoSOpt->as<uint8_t>();
    }

    const ConfigSession& ConfigSession::settWillQoS(uint8_t willQoS) const {
        willQoSOpt->default_val(willQoS)->clear();

        return *this;
    }

    bool ConfigSession::getWillRetain() const {
        return willRetainOpt->as<bool>();
    }

    const ConfigSession& ConfigSession::setWillRetain(bool willRetain) const {
        willRetainOpt->default_val(willRetain)->clear();

        return *this;
    }

    std::string ConfigSession::getUsername() const {
        return usernameOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::settUsername(const std::string& username) const {
        usernameOpt->default_val(username)->clear();

        return *this;
    }

    std::string ConfigSession::getPassword() const {
        return passwordOpt->as<std::string>();
    }

    const ConfigSession& ConfigSession::setPassword(const std::string& password) const {
        passwordOpt->default_val(password)->clear();

        return *this;
    }

} // namespace mqtt::mqttcli::lib

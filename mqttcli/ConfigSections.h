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

#ifndef CONFIG_SECTIONS
#define CONFIG_SECTIONS

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <net/config/ConfigSection.h>
#include <utils/CLI11.hpp>

#endif

class ConfigSubscribe : public net::config::ConfigSection {
public:
    ConfigSubscribe(net::config::ConfigInstance* instance);

    std::list<std::string> getTopic() const {
        return topicOpt->as<std::list<std::string>>();
    }

    const ConfigSubscribe* setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return this;
    }

private:
    CLI::Option* topicOpt;
};

class ConfigPublish : public net::config::ConfigSection {
public:
    ConfigPublish(net::config::ConfigInstance* instance);

    std::string getTopic() const {
        return topicOpt->as<std::string>();
    }

    const ConfigPublish* setTopic(const std::string& topic) {
        topicOpt->default_val(topic);

        return this;
    }

    std::string getMessage() const {
        return messageOpt->as<std::string>();
    }

    const ConfigPublish* setMessage(const std::string& message) {
        messageOpt->default_val(message);

        return this;
    }

    bool getRetain() const {
        return retainOpt->as<bool>();
    }

    const ConfigPublish* setRetain(bool retain) {
        retainOpt->default_val(retain);

        return this;
    }

private:
    CLI::Option* topicOpt;
    CLI::Option* messageOpt;
    CLI::Option* retainOpt;
};

class ConfigSession : public net::config::ConfigSection {
public:
    ConfigSession(net::config::ConfigInstance* instance);

    std::string getClientId() const {
        return clientIdOpt->as<std::string>();
    }

    const ConfigSession* setClientId(const std::string& clientId) const {
        clientIdOpt->default_val(clientId)->clear();

        return this;
    }

    uint8_t getQoS() const {
        return qoSOpt->as<uint8_t>();
    }

    const ConfigSession* setQos(uint8_t qoS) const {
        qoSOpt->default_val(qoS)->clear();

        return this;
    }

    bool getRetainSession() const {
        return retainSessionOpt->as<bool>();
    }

    const ConfigSession* setRetainSession(bool retainSession) const {
        retainSessionOpt->default_val(retainSession)->clear();

        return this;
    }

    uint16_t getKeepAlive() const {
        return keepAliveOpt->as<uint16_t>();
    }

    const ConfigSession* setKeepAlive(uint16_t keepAlive) const {
        keepAliveOpt->default_val(keepAlive)->clear();

        return this;
    }

    std::string getWillTopic() const {
        return willTopicOpt->as<std::string>();
    }

    const ConfigSession* setWillTopic(const std::string& willTopic) const {
        willTopicOpt->default_val(willTopic)->clear();

        return this;
    }

    std::string getWillMessage() const {
        return willMessageOpt->as<std::string>();
    }

    const ConfigSession* getWillMessage(const std::string& willMessage) const {
        willMessageOpt->default_val(willMessage)->clear();

        return this;
    }

    uint8_t getWillQoS() const {
        return willQoSOpt->as<uint8_t>();
    }

    const ConfigSession* settWillQoS(uint8_t willQoS) const {
        willQoSOpt->default_val(willQoS)->clear();

        return this;
    }

    bool getWillRetain() const {
        return willRetainOpt->as<bool>();
    }

    const ConfigSession* setWillRetain(bool willRetain) const {
        willRetainOpt->default_val(willRetain)->clear();

        return this;
    }

    std::string getUsername() const {
        return usernameOpt->as<std::string>();
    }

    const ConfigSession* settUsername(const std::string& username) const {
        usernameOpt->default_val(username)->clear();

        return this;
    }

    std::string getPassword() const {
        return passwordOpt->as<std::string>();
    }

    const ConfigSession* setPassword(const std::string& password) const {
        passwordOpt->default_val(password)->clear();

        return this;
    }

private:
    CLI::Option* clientIdOpt;
    CLI::Option* qoSOpt;
    CLI::Option* retainSessionOpt;
    CLI::Option* keepAliveOpt;
    CLI::Option* willTopicOpt;
    CLI::Option* willMessageOpt;
    CLI::Option* willQoSOpt;
    CLI::Option* willRetainOpt;
    CLI::Option* usernameOpt;
    CLI::Option* passwordOpt;
};

#endif // CONFIG_SECTIONS

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
#include <string_view>

namespace CLI {
    class Option;
}

#endif

namespace mqtt::mqttcli::lib {

    class ConfigSubscribe : public net::config::ConfigSection {
    public:
        constexpr static std::string_view name{"sub"};
        constexpr static std::string_view description{"Configuration for application mqttsub"};

        ConfigSubscribe(net::config::ConfigInstance* instance);

        ~ConfigSubscribe() override;

        std::list<std::string> getTopic() const;

        const ConfigSubscribe& setTopic(const std::string& topic);

    private:
        CLI::Option* topicOpt;
    };

    class ConfigPublish : public net::config::ConfigSection {
    public:
        constexpr static std::string_view name{"pub"};
        constexpr static std::string_view description{"Configuration for application mqttpub"};

        ConfigPublish(net::config::ConfigInstance* instance);

        ~ConfigPublish() override;

        std::string getTopic() const;
        const ConfigPublish& setTopic(const std::string& topic);

        std::string getMessage() const;
        const ConfigPublish& setMessage(const std::string& message);

        bool getRetain() const;
        const ConfigPublish& setRetain(bool retain);

    private:
        CLI::Option* topicOpt;
        CLI::Option* messageOpt;
        CLI::Option* retainOpt;
    };

    class ConfigSession : public net::config::ConfigSection {
    public:
        constexpr static std::string_view name{"session"};
        constexpr static std::string_view description{"MQTT session behavior"};

        ConfigSession(net::config::ConfigInstance* instance);

        ~ConfigSession() override;

        std::string getClientId() const;

        const ConfigSession& setClientId(const std::string& clientId) const;

        uint8_t getQoS() const;

        const ConfigSession& setQos(uint8_t qoS) const;

        bool getRetainSession() const;

        const ConfigSession& setRetainSession(bool retainSession) const;

        uint16_t getKeepAlive() const;

        const ConfigSession& setKeepAlive(uint16_t keepAlive) const;

        std::string getWillTopic() const;

        const ConfigSession& setWillTopic(const std::string& willTopic) const;

        std::string getWillMessage() const;

        const ConfigSession& setWillMessage(const std::string& willMessage) const;

        uint8_t getWillQoS() const;

        const ConfigSession& settWillQoS(uint8_t willQoS) const;

        bool getWillRetain() const;

        const ConfigSession& setWillRetain(bool willRetain) const;

        std::string getUsername() const;

        const ConfigSession& settUsername(const std::string& username) const;

        std::string getPassword() const;

        const ConfigSession& setPassword(const std::string& password) const;

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

} // namespace mqtt::mqttcli::lib

#endif // CONFIG_SECTIONS

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

#ifndef APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H
#define APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <string>
#include <string_view>

#endif

namespace mqtt::lib {

    class ConfigApplication {
    public:
        ConfigApplication(CLI::App* configSc);

        const ConfigApplication& setSessionStore(const std::string& sessionStore) const;

        std::string getSessionStore() const;

        const ConfigApplication& setMappingFile(const std::string& mappingFile) const;

        std::string getMappingFile() const;

        std::string MappingFile() const;

    protected:
        CLI::App* configSc;

        CLI::Option* sessionStoreOpt;
        CLI::Option* mappingFileOpt;
    };

    class ConfigMqttBroker : public ConfigApplication {
    public:
        constexpr static std::string_view name{"broker"};
        constexpr static std::string_view description{"Configuration for Application mqttbroker"};

        ConfigMqttBroker();
    };

    class ConfigMqttIntegrator : public ConfigApplication {
    public:
        constexpr static std::string_view name{"integrator"};
        constexpr static std::string_view description{"Configuration for Application mqttintegrator"};

        ConfigMqttIntegrator();
    };

    class ConfigWWW {
    public:
        constexpr static std::string_view name{"www"};
        constexpr static std::string_view description{"Web behavior of httpserver"};

        ConfigWWW();

        ConfigWWW& setHtmlRoot(const std::string& htmlRoot);
        std::string getHtmlRoot();

    private:
        CLI::App* configWWWSc;
        CLI::Option* htmlRootOpt;
    };

} // namespace mqtt::lib

#endif // APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H

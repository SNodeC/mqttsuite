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

#include "ConfigApplication.h"

#include "JsonMappingReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <stdexcept>

#endif

namespace mqtt::lib {

    template <typename ConcretConfigApplication>
    ConfigApplication::ConfigApplication(utils::SubCommand* parent, ConcretConfigApplication* concretConfigApplication)
        : utils::SubCommand(parent, concretConfigApplication, "Applications")
        , mqttMapper(std::make_shared<MqttMapper>())
        , mappingFileOpt(        //
              addOptionFunction( //
                  "--mqtt-mapping-file",
                  [this](const std::string& mappingFile) {
                      try {
                          mqttMapper->setMapping(JsonMappingReader::readMappingFromFile(mappingFile));
                      } catch (std::runtime_error& e) {
                          PLOG(ERROR) << "Applying mapping description file " << mappingFile << "\nWhat: " << e.what();
                      }
                  },
                  "MQTT mapping file (json format) for integration",
                  "filename",
                  !CLI::ExistingDirectory))
        , sessionStoreOpt( //
              addOption(   //
                  "--mqtt-session-store",
                  "Path to file for the persistent session store",
                  "filename",
                  !CLI::ExistingDirectory)) {
    }

    ConfigApplication::~ConfigApplication() {
    }

    ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) {
        setDefaultValue(sessionStoreOpt, sessionStore);

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    bool ConfigApplication::setMappingFile(const std::string& mappingFile) { // can throw
        setDefaultValue(mappingFileOpt, mappingFile);

        return setMapping(JsonMappingReader::readMappingFromFile(mappingFile));
    }

    std::string ConfigApplication::getMappingFile() const {
        return mappingFileOpt->as<std::string>();
    }

    bool ConfigApplication::reloadMapping() { // can throw
        return mqttMapper->setMapping(JsonMappingReader::readMappingFromFile(getMappingFile()));
    }

    bool ConfigApplication::setMapping(const nlohmann::json& mappingJson) { // can throw
        required(mappingFileOpt, false);

        return mqttMapper->setMapping(mappingJson);
    }

    const std::shared_ptr<MqttMapper> ConfigApplication::getMqttMapper() const {
        return mqttMapper;
    }

    ConfigMqttBroker::ConfigMqttBroker(utils::SubCommand* parent)
        : ConfigApplication(parent, this)
        , htmlRootOpt(addOption( //
              "--html-root",
              "HTML root directory",
              "directory",
              CLI::ExistingDirectory)) {
        required(htmlRootOpt);
    }

    ConfigMqttBroker& ConfigMqttBroker::setHtmlRoot(const std::string& htmlRoot) {
        setDefaultValue(htmlRootOpt, htmlRoot);
        required(htmlRootOpt, false);

        return *this;
    }

    std::string ConfigMqttBroker::getHtmlRoot() {
        std::string htmlRoot;

        return htmlRootOpt->as<std::string>();
    }

    ConfigMqttIntegrator::ConfigMqttIntegrator(utils::SubCommand* parent)
        : ConfigApplication(parent, this) {
        required(mappingFileOpt);
    }

} // namespace mqtt::lib

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

#include "MqttMapper.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
//
#include <nlohmann/json_fwd.hpp>

#endif

namespace mqtt::lib {

    template <typename ConcretConfigApplication>
    ConfigApplication::ConfigApplication(utils::SubCommand* parent, ConcretConfigApplication* concretConfigApplication)
        : utils::SubCommand(parent, concretConfigApplication, "Applications")
        , mqttMapper(std::make_shared<MqttMapper>())
        , mappingFileOpt(        //
              addOptionFunction( //
                  "--mqtt-mapping-file",
                  [this](const std::string& mappFilename) {
                      try {
                          loadMapping(mappFilename);
                      } catch (std::runtime_error& e) {
                          this->mapFilename.clear();

                          throw CLI::ValidationError(getName(),
                                                     std::string("Activating mapping description in '" + mappFilename +
                                                                 "' failed\n"
                                                                 "What: " +
                                                                 e.what()));
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

    ConfigApplication::~ConfigApplication() = default;

    ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) {
        setDefaultValue(sessionStoreOpt, sessionStore);

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    const std::shared_ptr<MqttMapper> ConfigApplication::getMqttMapper() const {
        return mqttMapper;
    }

    bool ConfigApplication::setMappingFile(const std::string& mapFilename) {
        setDefaultValue(mappingFileOpt, mapFilename);

        return loadMapping(mapFilename);
    }

    std::string ConfigApplication::getMappingFilename() const {
        return mapFilename;
    }

    bool ConfigApplication::setMapping(const std::string& mapping) const {
        return mqttMapper->setMapping(nlohmann::json::parse(mapping));
    }

    std::string ConfigApplication::getMapping(int indent) const {
        return mqttMapper->getMapping().dump(indent);
    }

    bool ConfigApplication::persistMapping() const {
        bool success = false;

        std::ofstream mapFile(mapFilename, std::ios::trunc);
        if (mapFile.is_open()) {
            try {
                mapFile << getMapping();

                success = true;

                VLOG(1) << "Write mapping file seccess";
            } catch (const std::exception& e) {
                VLOG(1) << "Write mapping file failed: " << e.what();
            }

            mapFile.close();
        } else {
            VLOG(1) << "Cannot open mapping file for writing: " << mapFilename;
        }

        return success;
    }

    bool ConfigApplication::loadMapping(const std::string mapFilename) {
        VLOG(1) << "Mapping file: " << mapFilename;

        this->mapFilename = mapFilename;

        bool mustReconnect = true;

        if (!mapFilename.empty()) {
            std::ifstream mapFile(mapFilename);

            if (mapFile.is_open()) {
                try {
                    mustReconnect = setMapping({std::istreambuf_iterator<char>(mapFile), std::istreambuf_iterator<char>()});

                    mapFile.close();

                    VLOG(1) << "Load mapping file success";
                } catch (const std::exception& e) {
                    mapFile.close();

                    throw std::runtime_error("Loading mapping description from '" + mapFilename +
                                             "' failed\n"
                                             "What: " +
                                             e.what());
                }

            } else {
                VLOG(1) << "Mapping file '" + mapFilename +
                               "' not found. Please provide a valid mapping file with the option --mqtt-mapping-file";
            }
        }

        return mustReconnect;
    }

    ConfigMqttBroker::ConfigMqttBroker(utils::SubCommand* parent)
        : ConfigApplication(parent, this)
        , htmlRootOpt(   //
              addOption( //
                  "--html-root",
                  "HTML root directory",
                  "directory",
                  CLI::ExistingDirectory)) {
        required(htmlRootOpt);
    }

    ConfigMqttBroker::~ConfigMqttBroker() = default;

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
    }

    ConfigMqttIntegrator::~ConfigMqttIntegrator() = default;

} // namespace mqtt::lib

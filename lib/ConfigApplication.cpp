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

#include <cerrno>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

#endif

namespace mqtt::lib {

    namespace {

        namespace fs = std::filesystem;

        void fsyncPath(const fs::path& path) {
            const int fd = ::open(path.c_str(), O_RDONLY);
            if (fd < 0) {
                throw std::system_error(errno, std::generic_category(), "open for fsync failed: " + path.string());
            }

            if (::fsync(fd) != 0) {
                const int err = errno;
                ::close(fd);
                throw std::system_error(err, std::generic_category(), "fsync failed: " + path.string());
            }

            ::close(fd);
        }

        void fsyncDirectory(const fs::path& directoryPath) {
            const int fd = ::open(directoryPath.c_str(), O_RDONLY | O_DIRECTORY);
            if (fd < 0) {
                throw std::system_error(errno, std::generic_category(), "open directory for fsync failed: " + directoryPath.string());
            }

            if (::fsync(fd) != 0) {
                const int err = errno;
                ::close(fd);
                throw std::system_error(err, std::generic_category(), "directory fsync failed: " + directoryPath.string());
            }

            ::close(fd);
        }

    } // namespace

    template <typename ConcretConfigApplication>
    ConfigApplication::ConfigApplication(utils::SubCommand* parent, ConcretConfigApplication* concretConfigApplication)
        : utils::SubCommand(parent, concretConfigApplication, "Applications")
        , mqttMapper(std::make_shared<MqttMapper>())
        , mappingFileOpt(        //
              addOptionFunction( //
                  "--mqtt-mapping-file",
                  [this](const std::string& mappFilename) {
                      try {
                          this->mappFilename = mappFilename;
                          loadMapping();
                      } catch (std::runtime_error& e) {
                          this->mappFilename.clear();

                          throw CLI::ValidationError(
                              getName(), std::string("Activating mapping description in '" + mappFilename + "' failed\nWhat: " + e.what()));
                      }
                  },
                  "MQTT mapping file (json format) for integration",
                  "filename",
                  !CLI::ExistingDirectory)
                  ->run_callback_for_default())
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

    ConfigApplication* ConfigApplication::setMappingFile(const std::string& mappingFile) {
        setDefaultValue(mappingFileOpt, mappingFile); // callback is called due to run_callback_for_default())

        return this;
    }

    bool ConfigApplication::persistMapping() const {
        bool success = false;

        if (mappFilename.empty()) {
            return success;
        }

        try {
            const fs::path targetPath(mappFilename);
            const fs::path parentDir = targetPath.parent_path().empty() ? fs::path(".") : targetPath.parent_path();
            const fs::path tempPath = mappFilename + ".tmp";

            {
                std::ofstream mapFile(tempPath, std::ios::trunc);
                if (!mapFile.is_open()) {
                    throw std::runtime_error("Cannot open temp file for writing: " + tempPath.string());
                }
                mapFile << mqttMapper->getMapping().dump(2);
            }

            fsyncPath(tempPath);
            fs::rename(tempPath, targetPath);
            fsyncDirectory(parentDir);

            VLOG(1) << "Write mapping file success";

            success = true;
        } catch (const std::exception& e) {
            VLOG(1) << "Write mapping file failed: " << e.what();
        }

        return success;
    }

    bool ConfigApplication::loadMapping() const {
        bool success = false;

        if (!mappFilename.empty()) {
            std::ifstream mapFile(mappFilename);

            if (mapFile.is_open()) {
                VLOG(1) << "Mapping file: " << mappFilename;

                try {
                    nlohmann::json mappingJson;
                    mapFile >> mappingJson;

                    mqttMapper->setMapping(mappingJson);

                    VLOG(1) << "Load mapping file success";

                    success = true;
                } catch (const std::exception& e) {
                    VLOG(1) << "Load mapping file failed: " << e.what() << " at " << mapFile.tellg();
                }

                mapFile.close();
            } else {
                VLOG(1) << "Mapping file: " << mappFilename << " not found";
            }
        }

        return success;
    }

    const std::shared_ptr<MqttMapper> ConfigApplication::getMqttMapper() const {
        return mqttMapper;
    }

    ConfigApplication* ConfigApplication::setMapping(const std::string& mapping) {
        mqttMapper->setMapping(nlohmann::json::parse(mapping));

        return this;
    }

    std::string ConfigApplication::getMapping(int indent) const {
        return mqttMapper->getMapping().dump(indent);
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

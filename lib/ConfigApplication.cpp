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

#include <net/config/ConfigInstanceAPI.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <functional>
#include <system_error>

#endif

namespace mqtt::lib {

    static CLI::Validator parent_dir_exists_and_target_not_dir() {
        return CLI::Validator(
            [](const std::string& value) -> std::string {
                namespace fs = std::filesystem;

                fs::path p{value};

                fs::path parent = p.parent_path();
                if (parent.empty())
                    parent = fs::path{"."};

                std::error_code ec;

                if (!fs::exists(parent, ec) || ec)
                    return "Parent directory does not exist: " + parent.string();

                if (!fs::is_directory(parent, ec) || ec)
                    return "Parent path is not a directory: " + parent.string();

                // Only forbid: target is an existing directory (including symlink-to-dir)
                if (fs::exists(p, ec) && !ec && fs::is_directory(p, ec) && !ec)
                    return "Target must not be an existing directory: " + p.string();

                return {}; // OK
            },
            "PATH(parent exists, target not directory)",
            "ParentExistsTargetNotDir");
    }

    ConfigApplication::ConfigApplication(CLI::App* configSc)
        : configSc(configSc) {
        mappingFileOpt = configSc->add_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration")
                             ->check(CLI::ExistingFile)
                             ->group(configSc->get_formatter()->get_label("Persistent Options"))
                             ->type_name("path")
                             ->configurable();

        sessionStoreOpt = configSc->add_option("--mqtt-session-store", "Path to file for the persistent session store")
                              ->check(parent_dir_exists_and_target_not_dir())
                              ->group(configSc->get_formatter()->get_label("Persistent Options"))
                              ->type_name("path")
                              ->configurable();
    }

    const ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) const {
        sessionStoreOpt->default_val(sessionStore)->clear();

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        std::string sessionStoreFile;
        try {
            sessionStoreFile = sessionStoreOpt->as<std::string>();
        } catch (CLI::ParseError&) {
        }

        return sessionStoreFile;
    }

    const ConfigApplication& ConfigApplication::setMappingFile(const std::string& mappingFile) const {
        mappingFileOpt->default_val(mappingFile)->clear();
        mappingFileOpt->required(false);

        configSc->required(false)->remove_needs(mappingFileOpt);
        configSc->get_parent()->remove_needs(configSc);

        return *this;
    }

    std::string ConfigApplication::getMappingFile() const {
        std::string mappingFile;
        try {
            mappingFile = mappingFileOpt->as<std::string>();
        } catch (CLI::ParseError&) {
        }

        return mappingFile;
    }

    ConfigMqttBroker::ConfigMqttBroker()
        : ConfigApplication(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        htmlRootOpt = configSc->add_option("--html-root", "HTML root directory")
                          ->group(configSc->get_formatter()->get_label("Persistent Options"))
                          ->check(CLI::ExistingDirectory)
                          ->type_name("path")
                          ->configurable()
                          ->required();

        configSc->required()->needs(htmlRootOpt);
        configSc->get_parent()->needs(configSc);
    }

    ConfigMqttBroker& ConfigMqttBroker::setHtmlRoot(const std::string& htmlRoot) {
        htmlRootOpt->default_val(htmlRoot)->clear();
        htmlRootOpt->required(false);

        configSc->required(false)->remove_needs(htmlRootOpt);
        configSc->get_parent()->remove_needs(configSc);

        return *this;
    }

    std::string ConfigMqttBroker::getHtmlRoot() {
        std::string htmlRoot;

        try {
            htmlRoot = htmlRootOpt->as<std::string>();
        } catch (CLI::ParseError&) {
        }

        return htmlRoot;
    }

    ConfigMqttIntegrator::ConfigMqttIntegrator()
        : ConfigApplication(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        mappingFileOpt->required();

        configSc->required()->needs(mappingFileOpt);
        configSc->get_parent()->needs(configSc);
    }

} // namespace mqtt::lib

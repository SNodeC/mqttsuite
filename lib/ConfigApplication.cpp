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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <net/config/ConfigInstanceAPI.hpp>

#endif

namespace mqtt::lib {

    ConfigApplication::ConfigApplication(CLI::App* configSc)
        : configSc(configSc) {
        mappingFileOpt = configSc->add_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration")
                             ->check(CLI::ExistingFile)
                             ->group(configSc->get_formatter()->get_label("Persistent Options"))
                             ->type_name("path")
                             ->configurable();

        sessionStoreOpt = configSc->add_option("--mqtt-session-store", "Path to file for the persistent session store")
                              ->check(CLI::ExistingFile)
                              ->group(configSc->get_formatter()->get_label("Persistent Options"))
                              ->type_name("path")
                              ->configurable();
    }

    const ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) const {
        sessionStoreOpt->default_str(sessionStore)->clear();

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    const ConfigApplication& ConfigApplication::setMappingFile(const std::string& mappingFile) const {
        mappingFileOpt->default_str(mappingFile)->clear();
        mappingFileOpt->required(false);

        configSc->remove_needs(mappingFileOpt);
        configSc->get_parent()->remove_needs(configSc);
        configSc->required(false);

        return *this;
    }

    std::string ConfigApplication::getMappingFile() const {
        return mappingFileOpt->as<std::string>();
    }

    ConfigMqttBroker::ConfigMqttBroker()
        : ConfigApplication(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
    }

    ConfigMqttIntegrator::ConfigMqttIntegrator()
        : ConfigApplication(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        mappingFileOpt->required();

        configSc->needs(mappingFileOpt)->required();
        configSc->get_parent()->needs(configSc);
    }

    ConfigWWW::ConfigWWW()
        : configWWWSc(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        htmlRootOpt = configWWWSc->add_option("--html-root", "HTML root directory")
                          ->group(configWWWSc->get_formatter()->get_label("Persistent Options"))
                          ->type_name("path")
                          ->configurable()
                          ->required();

        configWWWSc->needs(htmlRootOpt)->required();
        configWWWSc->get_parent()->needs(configWWWSc);
    }

    ConfigWWW& ConfigWWW::setHtmlRoot(const std::string& htmlRoot) {
        htmlRootOpt->default_str(htmlRoot)->clear();
        htmlRootOpt->required(false);

        configWWWSc->remove_needs(htmlRootOpt);
        configWWWSc->get_parent()->remove_needs(configWWWSc);
        configWWWSc->required(false);

        return *this;
    }

    std::string ConfigWWW::getHtmlRoot() {
        return htmlRootOpt->as<std::string>();
    }

} // namespace mqtt::lib

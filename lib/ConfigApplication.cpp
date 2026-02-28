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

#endif

namespace mqtt::lib {

    template <typename ConcretConfigApplication>
    ConfigApplication::ConfigApplication(ConcretConfigApplication* concretConfigApplication)
        : utils::SubCommand(utils::Config::newInstance(net::config::Instance(std::string(ConcretConfigApplication::name),
                                                                             std::string(ConcretConfigApplication::description),
                                                                             concretConfigApplication),
                                                       "Applications",
                                                       true)) {
        mappingFileOpt = addOption("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "filename", CLI::ExistingFile);

        sessionStoreOpt =
            addOption("--mqtt-session-store", "Path to file for the persistent session store", "filename", !CLI::ExistingDirectory);
    }

    const ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) const {
        setDefaultValue(sessionStoreOpt, sessionStore);

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    const ConfigApplication& ConfigApplication::setMappingFile(const std::string& mappingFile) const {
        setDefaultValue(mappingFileOpt, mappingFile);
        mappingFileOpt->required(false);

        subCommandSc->required(false)->remove_needs(mappingFileOpt);
        subCommandSc->get_parent()->remove_needs(subCommandSc);

        return *this;
    }

    std::string ConfigApplication::getMappingFile() const {
        return mappingFileOpt->as<std::string>();
    }

    ConfigMqttBroker::ConfigMqttBroker()
        : ConfigApplication(this) {
        htmlRootOpt = addOption("--html-root", "HTML root directory", "directory", CLI::ExistingDirectory);

        subCommandSc->required()->needs(htmlRootOpt);
        subCommandSc->get_parent()->needs(subCommandSc);
    }

    ConfigMqttBroker& ConfigMqttBroker::setHtmlRoot(const std::string& htmlRoot) {
        setDefaultValue(htmlRootOpt, htmlRoot);
        htmlRootOpt->required(false);

        subCommandSc->required(false)->remove_needs(htmlRootOpt);
        subCommandSc->get_parent()->remove_needs(subCommandSc);

        return *this;
    }

    std::string ConfigMqttBroker::getHtmlRoot() {
        std::string htmlRoot;

        return htmlRootOpt->as<std::string>();
    }

    ConfigMqttIntegrator::ConfigMqttIntegrator()
        : ConfigApplication(this) {
        mappingFileOpt->required();

        subCommandSc->required()->needs(mappingFileOpt);
        subCommandSc->get_parent()->needs(subCommandSc);
    }

} // namespace mqtt::lib

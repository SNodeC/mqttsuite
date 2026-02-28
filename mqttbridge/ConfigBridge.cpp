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

#include "ConfigBridge.h"

#include <net/config/ConfigInstanceAPI.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif

namespace mqtt::bridge {

    mqtt::bridge::ConfigBridge::ConfigBridge()
        : bridgeSc(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        bridgeDefinitionOpt = bridgeSc->add_option("--definition", "MQTT bridge definition file (JSON format)")
                                  ->check(CLI::ExistingFile)
                                  ->group(bridgeSc->get_formatter()->get_label("Persistent Options"))
                                  ->type_name("path")
                                  ->configurable()
                                  ->required();

        htmlDirOpt = bridgeSc->add_option("--html-dir", "Path to html source directory")
                         ->check(CLI::ExistingDirectory)
                         ->default_val(std::string(CMAKE_INSTALL_PREFIX) + "/var/www/mqttsuite/mqttbridge")
                         ->group(bridgeSc->get_formatter()->get_label("Persistent Options"))
                         ->type_name("path")
                         ->configurable();

        bridgeSc->needs(bridgeDefinitionOpt)->required();
        bridgeSc->get_parent()->needs(bridgeSc);
    }

    void mqtt::bridge::ConfigBridge::setDefinitionFile(const std::string& definitionFile) const {
        bridgeDefinitionOpt->default_val(definitionFile)->clear();
        bridgeDefinitionOpt->required(false);

        bridgeSc->required(false)->remove_needs(bridgeDefinitionOpt);
        bridgeSc->get_parent()->remove_needs(bridgeSc);
    }

    std::string mqtt::bridge::ConfigBridge::getDefinitionFile() const {
        return bridgeDefinitionOpt->as<std::string>();
    }

    void mqtt::bridge::ConfigBridge::setHtmlDir(const std::string& htmlDir) const {
        htmlDirOpt->default_val(htmlDir)->clear();
    }

    std::string mqtt::bridge::ConfigBridge::getHtmlDir() const {
        return htmlDirOpt->as<std::string>();
    }

} // namespace mqtt::bridge

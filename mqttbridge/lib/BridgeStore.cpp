/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
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

#include "BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <cmath>
#include <exception>
#include <fstream>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <utility>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    BridgeStore& BridgeStore::instance() {
        static BridgeStore bridgeConfigLoader;

        return bridgeConfigLoader;
    }

    bool BridgeStore::loadAndValidate(const std::string& fileName) {
        bool success = !brokers.empty();

#include "bridge-schema.json.h" // IWYU pragma: keep

        if (!success) {
            try {
                const nlohmann::json bridgeJsonSchema = nlohmann::json::parse(bridgeJsonSchemaString);

                if (!fileName.empty()) {
                    std::ifstream bridgeConfigJsonFile(fileName);

                    if (bridgeConfigJsonFile.is_open()) {
                        VLOG(1) << "Bridge config JSON: " << fileName;

                        try {
                            nlohmann::json bridgesConfigJson;

                            bridgeConfigJsonFile >> bridgesConfigJson;

                            try {
                                const nlohmann::json_schema::json_validator validator(bridgeJsonSchema);

                                try {
                                    const nlohmann::json defaultPatch = validator.validate(bridgesConfigJson);

                                    try {
                                        bridgesConfigJson = bridgesConfigJson.patch(defaultPatch);

                                        for (const nlohmann::json& bridgeConfigJson : bridgesConfigJson["bridges"]) {
                                            Bridge& bridge = bridgeList.emplace_back(bridgeConfigJson["name"]);

                                            for (const nlohmann::json& brokerConfigJson : bridgeConfigJson["brokers"]) {
                                                std::list<iot::mqtt::Topic> topics;
                                                for (const nlohmann::json& topicJson : brokerConfigJson["topics"]) {
                                                    if (!topicJson["topic"].get<std::string>().empty()) {
                                                        topics.emplace_back(topicJson["topic"], // cppcheck-suppress useStlAlgorithm
                                                                            topicJson["qos"]);
                                                    }
                                                }

                                                const nlohmann::json& mqtt = brokerConfigJson["mqtt"];
                                                const nlohmann::json& network = brokerConfigJson["network"];

                                                brokers.emplace(network["instance_name"],
                                                                Broker(bridge,
                                                                       brokerConfigJson["session_store"],
                                                                       network["instance_name"],
                                                                       network["protocol"],
                                                                       network["encryption"],
                                                                       network["transport"],
                                                                       network[network["protocol"]],
                                                                       mqtt["client_id"],
                                                                       mqtt["keep_alive"],
                                                                       mqtt["clean_session"],
                                                                       mqtt["will_topic"],
                                                                       mqtt["will_message"],
                                                                       mqtt["will_qos"],
                                                                       mqtt["will_retain"],
                                                                       mqtt["username"],
                                                                       mqtt["password"],
                                                                       mqtt["loop_prevention"],
                                                                       brokerConfigJson["prefix"],
                                                                       topics));
                                            }
                                        }

                                        success = true;
                                    } catch (const std::exception& e) {
                                        VLOG(1) << "  Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                        VLOG(1) << "    " << e.what();
                                    }
                                } catch (const std::exception& e) {
                                    VLOG(1) << "  Validating JSON failed:\n" << bridgesConfigJson.dump(4);
                                    VLOG(1) << "    " << e.what();
                                }
                            } catch (const std::exception& e) {
                                VLOG(1) << "  Setting root json mapping schema failed:\n" << bridgeJsonSchema.dump(4);
                                VLOG(1) << "    " << e.what();
                            }
                        } catch (const std::exception& e) {
                            VLOG(1) << "  JSON map file parsing failed:" << e.what() << " at " << bridgeConfigJsonFile.tellg();
                        }

                        bridgeConfigJsonFile.close();
                    } else {
                        VLOG(1) << "BridgeJsonConfig: " << fileName << " not found";
                    }
                } else {
                    // Do not log missing path. In regular use this missing option is captured by the command line interface
                }
            } catch (const std::exception& e) {
                VLOG(1) << "Parsing schema failed: " << e.what();
                VLOG(1) << bridgeJsonSchemaString;
            }
        } else {
            VLOG(1) << "MappingFile already loaded and validated";
        }

        return success;
    }

    const Broker& BridgeStore::getBroker(const std::string& instanceName) {
        return brokers.find(instanceName)->second;
    }

    const std::map<std::string, Broker>& BridgeStore::getBrokers() {
        return brokers;
    }

} // namespace mqtt::bridge::lib

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

#include "BridgeStore.h"

#include "lib/SSEDistributor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <exception>
#include <fstream>
#include <list>
#include <log/Logger.h>
#include <map>
#include <utility>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

#include "bridge-schema.json.h" // IWYU pragma: keep

    BridgeStore& BridgeStore::instance() {
        static BridgeStore bridgeConfigLoader;

        return bridgeConfigLoader;
    }

    bool BridgeStore::loadAndValidate(const std::string& fileName) {
        this->fileName = fileName;

        bool success = bridgeMap.empty();

        if (success) {
            try {
                const nlohmann::json bridgeJsonSchema = nlohmann::json::parse(bridgeJsonSchemaString);
                validator.set_root_schema(bridgeJsonSchema);

                if (!fileName.empty()) {
                    std::ifstream bridgeConfigJsonFile(fileName);

                    if (bridgeConfigJsonFile.is_open()) {
                        VLOG(1) << "Bridge config JSON: " << fileName;

                        try {
                            bridgeConfigJsonFile >> bridgesConfigJsonStaged;

                            try {
                                try {
                                    const nlohmann::json defaultPatch = validator.validate(bridgesConfigJsonStaged);

                                    try {
                                        bridgesConfigJsonStaged = bridgesConfigJsonStaged.patch(defaultPatch);

                                        activateStaged();

                                        success = true;
                                    } catch (const std::exception& e) {
                                        VLOG(1) << "  Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                        VLOG(1) << "    " << e.what();
                                    }
                                } catch (const std::exception& e) {
                                    VLOG(1) << "  Validating JSON failed:\n" << bridgesConfigJsonActive.dump(4);
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

    bool BridgeStore::patch(const nlohmann::json& jsonPatch) {
        bool success = false;

        try {
            bridgesConfigJsonStaged = bridgesConfigJsonActive.patch(jsonPatch);

            try {
                const nlohmann::json defaultPatch = validator.validate(bridgesConfigJsonStaged);

                try {
                    bridgesConfigJsonStaged = bridgesConfigJsonStaged.patch(defaultPatch);

                    success = true;
                } catch (const std::exception& e) {
                    VLOG(1) << "  Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                    VLOG(1) << "    " << e.what();
                }
            } catch (const std::exception& e) {
                VLOG(1) << "  Validating JSON failed:\n" << bridgesConfigJsonActive.dump(4);
                VLOG(1) << "    " << e.what();
            }
        } catch (const std::exception& e) {
            VLOG(1) << "  Default Patch:\n" << bridgesConfigJsonActive.dump(4);

            VLOG(1) << "  Patching JSON with update failed:\n" << jsonPatch.dump(4);
            VLOG(1) << "    " << e.what();
        }

        return success;
    }

    void BridgeStore::activateStaged() {
        for (auto& [fullInstanceName, bridge] : bridgeMap) {
            bridge.clear();
        }

        bridgeMap.clear();

        bridgesConfigJsonActive = bridgesConfigJsonStaged;

        std::ofstream ofs(fileName, std::ios::binary);

        ofs << bridgesConfigJsonActive.dump(4);

        ofs.close();

        for (const nlohmann::json& bridgeConfigJson : bridgesConfigJsonActive["bridges"]) {
            bridgeMap.emplace(bridgeConfigJson["name"],
                              Bridge{bridgeConfigJson["name"], bridgeConfigJson["prefix"], bridgeConfigJson["disabled"]});

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

                const std::string fullInstanceName =
                    bridgeMap.find(bridgeConfigJson["name"])->second.getName() + "+" + network["instance_name"].get<std::string>();

                bridgeMap.find(bridgeConfigJson["name"])
                    ->second.addBroker(fullInstanceName,
                                       Broker(bridgeMap.find(bridgeConfigJson["name"])->second,
                                              brokerConfigJson["session_store"],
                                              fullInstanceName,
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
                                              brokerConfigJson["disabled"],
                                              topics));
            }
        }
    }

    const std::map<const std::string, Bridge>& BridgeStore::getBridgeMap() {
        return bridgeMap;
    }

    const nlohmann::json& BridgeStore::getBridgesConfigJson() {
        return bridgesConfigJsonActive;
    }

    void BridgeStore::mqttConnected(Broker& broker, Mqtt* mqtt) const {
        broker.getBridge().addMqtt(mqtt);

        bool allConnected = true;

        for (const auto& [bridgeName, bridge] : bridgeMap) {
            allConnected &= bridge.getAllConnected1();
        }

        if (allConnected) {
            mqtt::bridge::lib::SSEDistributor::instance().bridgesStarted();
        }
    }

    void BridgeStore::mqttDisconnected(Broker& broker, Mqtt* mqtt) const {
        broker.getBridge().removeMqtt(mqtt);
    }

    static std::pair<std::string, std::string> split_plus(const std::string& s) {
        const auto pos = s.find('+');
        if (pos == std::string::npos)
            return {s, {}};
        return {s.substr(0, pos), s.substr(pos + 1)};
    };

    Broker* BridgeStore::getBroker(const std::string& fullInstanceName) {
        auto [bridgeName, instanceName] = split_plus(fullInstanceName);

        return bridgeMap.find(bridgeName)->second.getBroker(fullInstanceName);
    }

} // namespace mqtt::bridge::lib

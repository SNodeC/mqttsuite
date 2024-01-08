/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <exception>
#include <fstream>
#include <iot/mqtt/Topic.h>
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
                nlohmann::json bridgeConfigJson;

                if (!fileName.empty()) {
                    std::ifstream bridgeConfigJsonFile(fileName);

                    if (bridgeConfigJsonFile.is_open()) {
                        LOG(TRACE) << "Bridge config JSON: " << fileName;

                        try {
                            bridgeConfigJsonFile >> bridgeConfigJson;

                            try {
                                const nlohmann::json_schema::json_validator validator(bridgeJsonSchema);

                                try {
                                    const nlohmann::json defaultPatch = validator.validate(bridgeConfigJson);

                                    if (!defaultPatch.empty()) {
                                        try {
                                            LOG(TRACE) << "  Default patch:\n" << defaultPatch.dump(4);
                                            bridgeConfigJson = bridgeConfigJson.patch(defaultPatch);

                                            for (const nlohmann::json& bridgeConfigJson : // cppcheck-suppress shadowVariable

                                                 bridgeConfigJson["bridges"]) {
                                                const nlohmann::json& connection = bridgeConfigJson["connection"];

                                                Bridge& bridge = bridgeList.emplace_back(connection["client_id"],
                                                                                         connection["keep_alive"],
                                                                                         connection["clean_session"],
                                                                                         connection["will_topic"],
                                                                                         connection["will_message"],
                                                                                         connection["will_qos"],
                                                                                         connection["will_retain"],
                                                                                         connection["username"],
                                                                                         connection["password"]);

                                                for (const nlohmann::json& brokerConfigJson : bridgeConfigJson["brokers"]) {
                                                    std::list<iot::mqtt::Topic> topics;
                                                    for (const nlohmann::json& topicJson : brokerConfigJson["topics"]) {
                                                        topics.emplace_back(topicJson["topic"], // cppcheck-suppress useStlAlgorithm
                                                                            topicJson["qos"]);
                                                    }

                                                    brokers.emplace(brokerConfigJson["instance_name"],
                                                                    Broker(bridge,
                                                                           brokerConfigJson["instance_name"],
                                                                           brokerConfigJson["protocol"],
                                                                           brokerConfigJson["encryption"],
                                                                           brokerConfigJson["transport"],
                                                                           std::move(topics)));
                                                }
                                            }

                                            success = true;
                                        } catch (const std::exception& e) {
                                            LOG(ERROR) << "  Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                            LOG(ERROR) << "    " << e.what();
                                            bridgeConfigJson.clear();
                                        }
                                    }
                                } catch (const std::exception& e) {
                                    LOG(ERROR) << "  Validating JSON failed:\n" << bridgeConfigJson.dump(4);
                                    LOG(ERROR) << "    " << e.what();
                                    bridgeConfigJson.clear();
                                }
                            } catch (const std::exception& e) {
                                LOG(ERROR) << "  Setting root json mapping schema failed:\n" << bridgeJsonSchema.dump(4);
                                LOG(ERROR) << "    " << e.what();
                                bridgeConfigJson.clear();
                            }
                        } catch (const std::exception& e) {
                            LOG(ERROR) << "  JSON map file parsing failed:" << e.what() << " at " << bridgeConfigJsonFile.tellg();
                            bridgeConfigJson.clear();
                        }
                        bridgeConfigJsonFile.close();
                    } else {
                        LOG(ERROR) << "BridgeJsonConfig: " << fileName << " not found";
                    }
                } else {
                    // Do not log missing path. In regular use this missing option is captured by the command line interface
                }
            } catch (const std::exception& e) {
                LOG(ERROR) << "Parsing schema failed: " << e.what();
                LOG(ERROR) << bridgeJsonSchemaString;
            }
        } else {
            LOG(TRACE) << "MappingFile already loaded and validated";
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

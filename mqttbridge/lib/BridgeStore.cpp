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

#include "Bridge.h"
#include "Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <exception>
#include <initializer_list>
#include <iot/mqtt/Topic.h>
#include <istream>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <vector>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    BridgeStore::~BridgeStore() {
        std::set<Bridge*> bridgeSet;

        for (const auto& [instanceName, bridge] : bridges) {
            bridgeSet.insert(bridge);
        }

        for (Bridge* bridge : bridgeSet) {
            delete bridge;
        }
    }

    BridgeStore& BridgeStore::instance() {
        static BridgeStore bridgeConfigLoader;

        return bridgeConfigLoader;
    }

    bool BridgeStore::loadAndValidate(const std::string& fileName) {
        bool success = !bridges.empty();

#include "bridge-schema.json.h"

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

                                            for (const nlohmann::json& bridgeConfigJson : // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
                                                 bridgeConfigJson["bridges"]) {
                                                Bridge* bridge = new Bridge(bridgeConfigJson["connection"]["client_id"],
                                                                            bridgeConfigJson["connection"]["keep_alive"],
                                                                            bridgeConfigJson["connection"]["clean_session"],
                                                                            bridgeConfigJson["connection"]["will_topic"],
                                                                            bridgeConfigJson["connection"]["will_message"],
                                                                            bridgeConfigJson["connection"]["will_qos"],
                                                                            bridgeConfigJson["connection"]["will_retain"],
                                                                            bridgeConfigJson["connection"]["username"],
                                                                            bridgeConfigJson["connection"]["password"]);

                                                for (const nlohmann::json& brokerConfigJson : bridgeConfigJson["brokers"]) {
                                                    bridges[brokerConfigJson["instance_name"]] = bridge;

                                                    std::list<iot::mqtt::Topic> topics;
                                                    for (const nlohmann::json& topicJson : brokerConfigJson["topics"]) {
                                                        topics.emplace_back(topicJson["topic"], topicJson["qos"]);
                                                    }

                                                    brokers[brokerConfigJson["instance_name"]] = Broker(brokerConfigJson["instance_name"],
                                                                                                        brokerConfigJson["protocol"],
                                                                                                        brokerConfigJson["encryption"],
                                                                                                        brokerConfigJson["transport"],
                                                                                                        topics);
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

    Bridge* BridgeStore::getBridge(const std::string& instanceName) {
        return bridges[instanceName];
    }

    const Broker& BridgeStore::getBroker(const std::string& instanceName) {
        return brokers[instanceName];
    }

    const std::map<std::string, Broker>& BridgeStore::getBrokers() {
        return brokers;
    }

} // namespace mqtt::bridge::lib

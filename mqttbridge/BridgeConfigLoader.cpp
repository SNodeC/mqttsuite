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

#include "BridgeConfigLoader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "nlohmann/json-schema.hpp"

#include <exception>
#include <initializer_list>
#include <istream>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "lib/bridge-schema.json.h"

static nlohmann::json bridgeJsonSchema = nlohmann::json::parse(bridgeJsonSchemaString);

nlohmann::json BridgeConfigLoader::loadAndValidate(const std::string& fileName) {
    nlohmann::json bridgeConfigJson;

    if (!fileName.empty()) {
        std::ifstream bridgeConfigJsonFile(fileName);

        if (bridgeConfigJsonFile.is_open()) {
            LOG(TRACE) << "BridgeJsonSchemaPath: " << fileName;

            try {
                bridgeConfigJsonFile >> bridgeConfigJson;

                nlohmann::json_schema::json_validator validator;

                try {
                    validator.set_root_schema(bridgeJsonSchema);

                    try {
                        nlohmann::json defaultPatch = validator.validate(bridgeConfigJson);

                        if (!defaultPatch.empty()) {
                            try {
                                LOG(TRACE) << "  Default patch:\n" << defaultPatch.dump(4);
                                bridgeConfigJson = bridgeConfigJson.patch(defaultPatch);
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
                bridgeConfigJsonFile.close();
            } catch (const std::exception& e) {
                LOG(ERROR) << "  JSON map file parsing failed:" << e.what() << " at " << bridgeConfigJsonFile.tellg();
                bridgeConfigJson.clear();
            }
            bridgeConfigJsonFile.close();
        } else {
            LOG(TRACE) << "BridgeJsonConfig: " << fileName << " not found";
        }
    } else {
        LOG(TRACE) << "MappingFilePath empty";
    }

    return bridgeConfigJson;
}

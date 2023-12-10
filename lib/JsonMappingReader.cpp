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

#include "JsonMappingReader.h"

#include "nlohmann/json-schema.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <fstream>
#include <initializer_list>
#include <log/Logger.h>
#include <vector>

#endif

// IWYU pragma: no_include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

namespace mqtt::lib {

#include "mapping-schema.json.h" // definition of mappingJsonSchemaString

    nlohmann::json JsonMappingReader::mappingJsonSchema = nlohmann::json::parse(mappingJsonSchemaString);

    std::map<std::string, nlohmann::json> JsonMappingReader::mapFileJsons;

    nlohmann::json& JsonMappingReader::readMappingFromFile(const std::string& mapFilePath) {
        if (!mapFileJsons.contains(mapFilePath)) {
            if (!mapFilePath.empty()) {
                std::ifstream mapFile(mapFilePath);

                if (mapFile.is_open()) {
                    LOG(TRACE) << "MappingFilePath: " << mapFilePath;

                    try {
                        mapFile >> mapFileJsons[mapFilePath];

                        try {
                            const nlohmann::json_schema::json_validator validator(mappingJsonSchema);

                            try {
                                const nlohmann::json defaultPatch = validator.validate(mapFileJsons[mapFilePath]);

                                if (!defaultPatch.empty()) {
                                    try {
                                        mapFileJsons[mapFilePath] = mapFileJsons[mapFilePath].patch(defaultPatch);
                                        LOG(TRACE) << "Patched JSON:\n" << mapFileJsons[mapFilePath].dump(4);
                                    } catch (const std::exception& e) {
                                        LOG(ERROR) << e.what();
                                        LOG(ERROR) << "Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                        mapFileJsons[mapFilePath].clear();
                                    }
                                }
                            } catch (const std::exception& e) {
                                LOG(ERROR) << "  Validating JSON failed:\n" << mapFileJsons[mapFilePath].dump(4);
                                LOG(ERROR) << "    " << e.what();
                                mapFileJsons[mapFilePath].clear();
                            }
                        } catch (const std::exception& e) {
                            LOG(ERROR) << e.what();
                            LOG(ERROR) << "Setting root json mapping schema failed:\n" << mappingJsonSchema.dump(4);
                            mapFileJsons[mapFilePath].clear();
                        }
                    } catch (const std::exception& e) {
                        LOG(ERROR) << "JSON map file parsing failed: " << e.what() << " at " << mapFile.tellg();
                        mapFileJsons[mapFilePath].clear();
                    }
                    mapFile.close();
                } else {
                    LOG(TRACE) << "MappingFilePath: " << mapFilePath << " not found";
                }
            } else {
                LOG(TRACE) << "MappingFilePath empty";
            }
        }

        return mapFileJsons[mapFilePath];
    }

} // namespace mqtt::lib

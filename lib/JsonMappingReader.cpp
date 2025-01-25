/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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
#include <log/Logger.h>
#include <nlohmann/json.hpp>

#endif

namespace mqtt::lib {

#include "mapping-schema.json.h" // definition of mappingJsonSchemaString

    nlohmann::json JsonMappingReader::mappingJsonSchema = nlohmann::json::parse(mappingJsonSchemaString);

    std::map<std::string, nlohmann::json> JsonMappingReader::mapFileJsons;

    nlohmann::json& JsonMappingReader::readMappingFromFile(const std::string& mapFilePath) {
        if (!mapFileJsons.contains(mapFilePath)) {
            if (!mapFilePath.empty()) {
                std::ifstream mapFile(mapFilePath);

                if (mapFile.is_open()) {
                    VLOG(1) << "MappingFilePath: " << mapFilePath;

                    try {
                        mapFile >> mapFileJsons[mapFilePath];

                        try {
                            const nlohmann::json_schema::json_validator validator(mappingJsonSchema);

                            try {
                                const nlohmann::json defaultPatch = validator.validate(mapFileJsons[mapFilePath]);

                                if (!defaultPatch.empty()) {
                                    try {
                                        mapFileJsons[mapFilePath] = mapFileJsons[mapFilePath].patch(defaultPatch);
                                    } catch (const std::exception& e) {
                                        VLOG(1) << e.what();
                                        VLOG(1) << "Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                        mapFileJsons[mapFilePath].clear();
                                    }
                                }
                            } catch (const std::exception& e) {
                                VLOG(1) << "  Validating JSON failed:\n" << mapFileJsons[mapFilePath].dump(4);
                                VLOG(1) << "    " << e.what();
                                mapFileJsons[mapFilePath].clear();
                            }
                        } catch (const std::exception& e) {
                            VLOG(1) << e.what();
                            VLOG(1) << "Setting root json mapping schema failed:\n" << mappingJsonSchema.dump(4);
                            mapFileJsons[mapFilePath].clear();
                        }
                    } catch (const std::exception& e) {
                        VLOG(1) << "JSON map file parsing failed: " << e.what() << " at " << mapFile.tellg();
                        mapFileJsons[mapFilePath].clear();
                    }
                    mapFile.close();
                } else {
                    VLOG(1) << "MappingFilePath: " << mapFilePath << " not found";
                }
            } else {
                VLOG(1) << "MappingFilePath empty";
            }
        }

        return mapFileJsons[mapFilePath];
    }

} // namespace mqtt::lib

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
#include <SemanticLog.h>
#include <vector>

#endif

// IWYU pragma: no_include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

namespace mqtt::lib {

#include "mapping-schema.json.h" // definition of mappingJsonSchemaString

    nlohmann::json JsonMappingReader::mappingJsonSchema = nlohmann::json::parse(mappingJsonSchemaString);

    std::map<std::string, nlohmann::json> JsonMappingReader::mapFileJsons;

    class custom_error_handler : public nlohmann::json_schema::basic_error_handler {
        void error(const nlohmann::json::json_pointer& ptr, const nlohmann::json& instance, const std::string& message) override {
            nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
            snode::semantic::appLog().error() << ptr.to_string() << " - " << instance << "': " << message << "\n";
        }
    };

    nlohmann::json& JsonMappingReader::readMappingFromFile(const std::string& mapFilePath) {
        if (!mapFileJsons.contains(mapFilePath)) {
            if (!mapFilePath.empty()) {
                std::ifstream mapFile(mapFilePath);

                if (mapFile.is_open()) {
                    snode::semantic::appLog().trace() << "MappingFilePath: " << mapFilePath;

                    try {
                        mapFile >> mapFileJsons[mapFilePath];

                        nlohmann::json_schema::json_validator validator(nullptr, nlohmann::json_schema::default_string_format_check);

                        try {
                            validator.set_root_schema(mappingJsonSchema);

                            custom_error_handler err;
                            nlohmann::json defaultPatch = validator.validate(mapFileJsons[mapFilePath], err);

                            if (!err) {
                                try {
                                    mapFileJsons[mapFilePath] = mapFileJsons[mapFilePath].patch(defaultPatch);
                                } catch (const std::exception& e) {
                                    snode::semantic::appLog().error() << e.what();
                                    snode::semantic::appLog().error() << "Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                    mapFileJsons[mapFilePath].clear();
                                }
                            } else {
                                mapFileJsons[mapFilePath].clear();
                            }
                        } catch (const std::exception& e) {
                            snode::semantic::appLog().error() << e.what();
                            snode::semantic::appLog().error() << "Setting root json mapping schema failed:\n" << mappingJsonSchema.dump(4);
                            mapFileJsons[mapFilePath].clear();
                        }

                        mapFile.close();
                    } catch (const std::exception& e) {
                        snode::semantic::appLog().error() << "JSON map file parsing failed: " << e.what() << " at " << mapFile.tellg();
                        mapFileJsons[mapFilePath].clear();
                    }
                    mapFile.close();
                } else {
                    snode::semantic::appLog().trace() << "MappingFilePath: " << mapFilePath << " not found";
                }
            } else {
                snode::semantic::appLog().trace() << "MappingFilePath empty";
            }
        }

        return mapFileJsons[mapFilePath];
    }

} // namespace mqtt::lib

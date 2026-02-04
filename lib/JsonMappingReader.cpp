/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
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

#include "JsonMappingReader.h"

#include "nlohmann/json-schema.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <chrono>
#include <compare>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <log/Logger.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#endif

namespace mqtt::lib {

    namespace fs = std::filesystem;

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
                            const nlohmann::json_schema::json_validator validator(
                                mappingJsonSchema, nullptr, nlohmann::json_schema::default_string_format_check);

                            try {
                                const nlohmann::json defaultPatch = validator.validate(mapFileJsons[mapFilePath]);

                                try {
                                    mapFileJsons[mapFilePath] = mapFileJsons[mapFilePath].patch(defaultPatch);
                                    VLOG(0) << "Patched: \n" << mapFileJsons[mapFilePath].dump(4);
                                } catch (const std::exception& e) {
                                    VLOG(1) << e.what();
                                    VLOG(1) << "Patching JSON with default patch failed:\n" << defaultPatch.dump(4);
                                    mapFileJsons[mapFilePath].clear();
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

    void JsonMappingReader::invalidate(const std::string& mapFilePath) {
        if (mapFileJsons.contains(mapFilePath)) {
            mapFileJsons.erase(mapFilePath);
        }
    }

    const nlohmann::json& JsonMappingReader::getSchema() {
        return mappingJsonSchema;
    }

    std::string JsonMappingReader::getDraftPath(const std::string& mapFilePath) {
        return mapFilePath + ".draft";
    }

    void JsonMappingReader::saveDraft(const std::string& mapFilePath, const nlohmann::json& content) {
        std::ofstream out(getDraftPath(mapFilePath), std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Cannot open draft file for writing: " + getDraftPath(mapFilePath));
        }
        out << content.dump(4) << std::endl;
    }

    nlohmann::json JsonMappingReader::readDraftOrActive(const std::string& mapFilePath) {
        std::string draftPath = getDraftPath(mapFilePath);
        if (fs::exists(draftPath)) {
            std::ifstream f(draftPath);
            if (f) {
                nlohmann::json j;
                f >> j;
                return j;
            }
        }
        // Fallback to active file
        std::ifstream f(mapFilePath);
        if (!f)
            throw std::runtime_error("Cannot open mapping file: " + mapFilePath);
        nlohmann::json j;
        f >> j;
        return j;
    }

    void JsonMappingReader::deployDraft(const std::string& mapFilePath) {
        std::string draftPath = getDraftPath(mapFilePath);
        if (!fs::exists(draftPath))
            return;

        // 1. Inject creation timestamp into draft
        try {
            std::ifstream f(draftPath);
            nlohmann::json j;
            f >> j;
            f.close();

            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");

            if (!j.contains("meta"))
                j["meta"] = nlohmann::json::object();
            j["meta"]["created"] = ss.str();
            j["meta"]["version"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

            std::ofstream out(draftPath, std::ios::trunc);
            out << j.dump(4);
            out.close();
        } catch (const std::exception& e) {
            VLOG(1) << "Failed to inject metadata into draft: " << e.what();
        }

        // 2. Backup current active file
        if (fs::exists(mapFilePath)) {
            fs::path versionDir = fs::path(mapFilePath).parent_path() / "versions";
            if (!fs::exists(versionDir)) {
                fs::create_directories(versionDir);
            }

            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            std::string filename = fs::path(mapFilePath).filename().string();
            std::string backupPath = versionDir / (filename + "." + std::to_string(timestamp));

            fs::copy_file(mapFilePath, backupPath, fs::copy_options::overwrite_existing);

            // 3. Prune old versions (Keep last 50)
            try {
                std::vector<fs::path> versions;
                for (const auto& entry : fs::directory_iterator(versionDir)) {
                    if (entry.path().filename().string().starts_with(filename + ".")) {
                        versions.push_back(entry.path());
                    }
                }
                if (versions.size() > 50) {
                    std::sort(versions.begin(), versions.end(), [](const fs::path& a, const fs::path& b) {
                        return fs::last_write_time(a) < fs::last_write_time(b); // Oldest first
                    });
                    for (size_t i = 0; i < versions.size() - 50; ++i) {
                        fs::remove(versions[i]);
                    }
                }
            } catch (...) {
            }
        }

        // 4. Promote draft to active
        fs::rename(draftPath, mapFilePath);

        // Invalidate cache so next readMappingFromFile reloads it
        invalidate(mapFilePath);
    }

    void JsonMappingReader::discardDraft(const std::string& mapFilePath) {
        std::string draftPath = getDraftPath(mapFilePath);
        if (fs::exists(draftPath)) {
            fs::remove(draftPath);
        }
    }

    std::vector<JsonMappingReader::VersionEntry> JsonMappingReader::getHistory(const std::string& mapFilePath) {
        std::vector<VersionEntry> history;
        fs::path versionDir = fs::path(mapFilePath).parent_path() / "versions";
        std::string baseName = fs::path(mapFilePath).filename().string();

        if (!fs::exists(versionDir))
            return history;

        for (const auto& entry : fs::directory_iterator(versionDir)) {
            if (entry.path().filename().string().starts_with(baseName + ".")) {
                VersionEntry v;
                v.filename = entry.path().string();
                // Extract ID (timestamp) from filename extension
                v.id = entry.path().extension().string().substr(1);

                // Peek inside JSON to get the comment
                try {
                    std::ifstream f(v.filename);
                    nlohmann::json j;
                    f >> j;
                    if (j.contains("meta")) {
                        if (j["meta"].contains("comment"))
                            v.comment = j["meta"]["comment"];
                        if (j["meta"].contains("created"))
                            v.date = j["meta"]["created"];
                    }
                } catch (...) {
                }

                // Fallback date if not in meta
                if (v.date.empty()) {
                    try {
                        long long ts = std::stoll(v.id);
                        std::time_t t = static_cast<std::time_t>(ts);
                        std::stringstream ss;
                        ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
                        v.date = ss.str();
                    } catch (...) {
                        v.date = "Unknown";
                    }
                }

                history.push_back(v);
            }
        }
        // Sort by ID (descending)
        std::sort(history.begin(), history.end(), [](const VersionEntry& a, const VersionEntry& b) {
            // String comparison of timestamps works if they are same length, but better to be safe
            try {
                return std::stoll(a.id) > std::stoll(b.id);
            } catch (...) {
                return a.id > b.id;
            }
        });
        return history;
    }

    void JsonMappingReader::rollbackTo(const std::string& mapFilePath, const std::string& versionId) {
        fs::path versionDir = fs::path(mapFilePath).parent_path() / "versions";
        std::string baseName = fs::path(mapFilePath).filename().string();
        fs::path backupPath = versionDir / (baseName + "." + versionId);

        if (!fs::exists(backupPath)) {
            throw std::runtime_error("Version not found: " + versionId);
        }

        // Validate before rollback
        try {
            std::ifstream f(backupPath);
            nlohmann::json j;
            f >> j;
            const nlohmann::json_schema::json_validator validator(
                mappingJsonSchema, nullptr, nlohmann::json_schema::default_string_format_check);
            validator.validate(j);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Cannot rollback: Version is invalid against current schema: ") + e.what());
        }

        // Overwrite active file
        fs::copy_file(backupPath, mapFilePath, fs::copy_options::overwrite_existing);

        // Delete any existing draft to avoid confusion
        discardDraft(mapFilePath);

        invalidate(mapFilePath);
    }

} // namespace mqtt::lib

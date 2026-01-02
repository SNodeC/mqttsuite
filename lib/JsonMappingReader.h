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

#ifndef MQTTBROKER_LIB_JSONMAPPINGREADER_H
#define MQTTBROKER_LIB_JSONMAPPINGREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <vector>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <string>

#endif

namespace mqtt::lib {

    class JsonMappingReader {
    public:
        JsonMappingReader() = delete;

        static nlohmann::json& readMappingFromFile(const std::string& mapFilePath);
        static void invalidate(const std::string& mapFilePath);
        static const nlohmann::json& getSchema();

        // Admin / Live Reload Support
        static void saveDraft(const std::string& mapFilePath, const nlohmann::json& content);
        static nlohmann::json readDraftOrActive(const std::string& mapFilePath);
        static void deployDraft(const std::string& mapFilePath);
        static void discardDraft(const std::string& mapFilePath);
        static std::string getDraftPath(const std::string& mapFilePath);

        struct VersionEntry {
            std::string id;
            std::string filename;
            std::string comment;
            std::string date;
        };

        static std::vector<VersionEntry> getHistory(const std::string& mapFilePath);
        static void rollbackTo(const std::string& mapFilePath, const std::string& versionId);

    private:
        static nlohmann::json mappingJsonSchema;
        static const std::string mappingJsonSchemaString;

        static std::map<std::string, nlohmann::json> mapFileJsons;
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_JSONMAPPINGREADER_H

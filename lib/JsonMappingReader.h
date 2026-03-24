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

#ifndef MQTTBROKER_LIB_JSONMAPPINGREADER_H
#define MQTTBROKER_LIB_JSONMAPPINGREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <optional>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib {

    class OCCConflictError : public std::runtime_error {
    public:
        explicit OCCConflictError(const std::string& whatArg)
            : std::runtime_error(whatArg) {
        }
    };

    class EntityNotFoundError : public std::runtime_error {
    public:
        explicit EntityNotFoundError(const std::string& whatArg)
            : std::runtime_error(whatArg) {
        }
    };

    class JsonMappingReader {
    public:
        JsonMappingReader() = delete;

    public:
        static nlohmann::json readMappingFromFile(const std::string& mapFilePath);

        static nlohmann::json readActive(const std::string& mapFilePath);
        static std::uint64_t readActiveRevision(const std::string& mapFilePath);

        // Admin / Live Reload Support
        static std::string getDraftsDirPath(const std::string& mapFilePath);
        static std::string getDraftPath(const std::string& mapFilePath, const std::string& draftId);
        static std::string createDraftFromActive(const std::string& mapFilePath, const std::string& draftId = "");
        static std::vector<nlohmann::json> listDrafts(const std::string& mapFilePath);
        static nlohmann::json readDraft(const std::string& mapFilePath, const std::string& draftId);
        static nlohmann::json replaceDraft(const std::string& mapFilePath, const std::string& draftId, const nlohmann::json& mapping, std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static nlohmann::json patchDraft(const std::string& mapFilePath, const std::string& draftId, const nlohmann::json& patchOps, std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static void discardDraft(const std::string& mapFilePath, const std::string& draftId);

        static void saveDraft(const std::string& mapFilePath, const nlohmann::json& content);
        static nlohmann::json readDraftOrActive(const std::string& mapFilePath);
        static nlohmann::json deployDraft(const std::string& mapFilePath, bool enableVersioning = true);
        static void discardDraft(const std::string& mapFilePath);
        static std::string getDraftPath(const std::string& mapFilePath);

        static nlohmann::json deployDraft(const std::string& mapFilePath,
                                          const std::string& draftId,
                                          bool enableVersioning,
                                          const std::optional<std::uint64_t>& expectedActiveRevision);

        struct VersionEntry {
            std::string id;
            std::string filename;
            std::string comment;
            std::string date;
        };

        static std::vector<VersionEntry> getHistory(const std::string& mapFilePath);
        static nlohmann::json rollbackTo(const std::string& mapFilePath,
                                         const std::string& versionId,
                                         bool enableVersioning,
                                         const std::optional<std::uint64_t>& expectedActiveRevision);
        static nlohmann::json rollbackTo(const std::string& mapFilePath, const std::string& versionId);

    private:
        static nlohmann::json getDefaultPatch(const nlohmann::json& inputJson);
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_JSONMAPPINGREADER_H

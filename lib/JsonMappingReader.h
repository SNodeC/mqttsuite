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

#include <cstdint>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib {

    class ConfigApplication;

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

    // Called synchronously by the event loop; each process owns its administration storage.
    class JsonMappingReader {
    public:
        JsonMappingReader() = delete;

        struct ApplyResult {
            std::uint64_t revision{0};
            std::string draftId;
            bool mappingPersisted{false};
            bool mustReconnect{false};
        };

        // Admin / Live Reload Support
        static nlohmann::json
        createDraftFromActive(const std::string& adminStorageRoot, ConfigApplication* configApplication, const std::string& draftId = "");
        static std::vector<nlohmann::json> listDrafts(const std::string& adminStorageRoot);
        static nlohmann::json readDraft(const std::string& adminStorageRoot, const std::string& draftId);
        static std::optional<int64_t> readDraftRevision(const std::string& adminStorageRoot, const std::string& draftId);
        static nlohmann::json replaceDraft(const std::string& adminStorageRoot,
                                           const std::string& draftId,
                                           const nlohmann::json& mapping,
                                           std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static nlohmann::json patchDraft(const std::string& adminStorageRoot,
                                         const std::string& draftId,
                                         const nlohmann::json& patchOps,
                                         std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static nlohmann::json replaceDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                         ConfigApplication* configApplication,
                                                         const std::string& draftId,
                                                         const nlohmann::json& mapping,
                                                         std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static nlohmann::json patchDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                       ConfigApplication* configApplication,
                                                       const std::string& draftId,
                                                       const nlohmann::json& patchOps,
                                                       std::optional<int64_t> expectedDraftRevision = std::nullopt);
        static bool isMappingValid(const nlohmann::json& mapping);
        static bool isDraftValid(const std::string& adminStorageRoot, const std::string& draftId);
        static void discardDraft(const std::string& adminStorageRoot, const std::string& draftId);

        struct VersionEntry {
            std::string snapshotId;
            std::string filename;
            std::string comment;
            std::string date;
        };

        static std::vector<VersionEntry> getHistory(const std::string& adminStorageRoot);

        static ApplyResult deployAndApplyDraft(const std::string& adminStorageRoot,
                                               ConfigApplication* configApplication,
                                               const std::string& draftId,
                                               const std::optional<std::uint64_t>& expectedActiveRevision);

        static ApplyResult rollbackAndApplyVersion(const std::string& adminStorageRoot,
                                                   ConfigApplication* configApplication,
                                                   const std::string& snapshotId,
                                                   const std::optional<std::uint64_t>& expectedActiveRevision);
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_JSONMAPPINGREADER_H

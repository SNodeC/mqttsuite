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

// JSON and chrono provide these implementation dependencies through their public headers.
// IWYU pragma: no_include <cmath>
// IWYU pragma: no_include <compare>
// IWYU pragma: no_include <map>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include "JsonMappingReader.h"

#include "ConfigApplication.h"
#include "MqttMapper.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// #include "nlohmann/json-schema.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include <utility>

#endif

namespace mqtt::lib {

    namespace fs = std::filesystem;

    namespace {

        struct ActiveState {
            nlohmann::json mapping;
            std::uint64_t revision{0};
        };

        constexpr std::size_t MAX_HISTORY_ENTRIES = 50;

        std::uint64_t idCounter{0};

        fs::path adminRootPath(const std::string& adminStorageRoot) {
            if (adminStorageRoot.empty()) {
                throw std::runtime_error("Admin storage root must not be empty");
            }

            const fs::path root = fs::path(adminStorageRoot);
            fs::create_directories(root);
            return root;
        }

        std::string nowIsoUtc() {
            const auto now = std::chrono::system_clock::now();
            const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");
            return ss.str();
        }

        std::string makeUniqueId() {
            const auto ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            const auto localCounter = idCounter++;
            return std::to_string(ns) + "-" + std::to_string(static_cast<long long>(::getpid())) + "-" + std::to_string(localCounter);
        }

        std::string makeUuidV4() {
            static std::mt19937_64 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, 255);

            std::array<unsigned char, 16> bytes{};
            for (auto& b : bytes) {
                b = static_cast<unsigned char>(dist(rng));
            }

            bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
            bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

            std::ostringstream ss;
            ss << std::hex << std::setfill('0');
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                ss << std::setw(2) << static_cast<int>(bytes[i]);
                if (i == 3 || i == 5 || i == 7 || i == 9) {
                    ss << '-';
                }
            }

            return ss.str();
        }

        std::string sanitizeDraftId(const std::string& draftId) {
            if (draftId.empty()) {
                throw std::runtime_error("Draft id must not be empty");
            }

            for (const char c : draftId) {
                const unsigned char uc = static_cast<unsigned char>(c);
                if (!(std::isalnum(uc) || c == '-' || c == '_')) {
                    throw std::runtime_error("Draft id contains unsupported characters");
                }
            }
            return draftId;
        }

        fs::path getDraftsDir(const std::string& adminStorageRoot) {
            return adminRootPath(adminStorageRoot) / "drafts";
        }

        fs::path getDraftFile(const std::string& adminStorageRoot, const std::string& draftId) {
            return getDraftsDir(adminStorageRoot) / (sanitizeDraftId(draftId) + ".json");
        }

        fs::path getVersionDir(const std::string& adminStorageRoot) {
            return adminRootPath(adminStorageRoot) / "versions";
        }

        nlohmann::json readJsonFromFile(const fs::path& path) {
            std::ifstream in(path);
            if (!in) {
                throw std::runtime_error("Cannot open JSON file: " + path.string());
            }

            nlohmann::json j;
            in >> j;
            return j;
        }

        void writeJson(const fs::path& targetPath, const nlohmann::json& j) {
            const fs::path parentDir = targetPath.parent_path().empty() ? fs::path(".") : targetPath.parent_path();
            fs::create_directories(parentDir);

            std::ofstream out(targetPath, std::ios::trunc);
            if (!out) {
                throw std::runtime_error("Cannot open JSON file for writing: " + targetPath.string());
            }
            out << j.dump(2) << '\n';
            out.close();
            if (!out) {
                throw std::runtime_error("Cannot write JSON file: " + targetPath.string());
            }
        }

        void setDeployMetadata(nlohmann::json& mapping, std::uint64_t revision) {
            auto& meta = mapping["meta"];
            const std::string timestamp = nowIsoUtc();
            const std::string deploymentId = makeUniqueId();

            if (!meta.contains("created")) {
                meta["created"] = timestamp;
            }
            meta["deployed_at"] = timestamp;
            meta["deployment_id"] = deploymentId;
            meta["revision"] = revision;
        }

        int64_t readDraftRevisionValue(const nlohmann::json& envelope, const fs::path& draftPath) {
            try {
                return envelope.at("draft_revision").get<int64_t>();
            } catch (const nlohmann::json::exception&) {
                throw std::runtime_error("Draft file is malformed: " + draftPath.string());
            }
        }

        void ensureDraftEnvelopeShape(const nlohmann::json& envelope, const fs::path& draftPath) {
            try {
                (void) envelope.at("id").get<std::string>();
                (void) envelope.at("base_revision");
                (void) readDraftRevisionValue(envelope, draftPath);
                (void) envelope.at("mapping");
            } catch (const std::exception&) {
                throw std::runtime_error("Draft file is malformed: " + draftPath.string());
            }
        }

        void setDraftRevision(nlohmann::json& envelope, int64_t draftRevision) {
            envelope["draft_revision"] = draftRevision;
        }

        std::string readMetaString(const nlohmann::json& document, std::string_view key) {
            const auto metaIt = document.find("meta");
            if (metaIt == document.end() || !metaIt->is_object()) {
                return "";
            }
            const auto keyIt = metaIt->find(key);
            return keyIt != metaIt->end() && keyIt->is_string() ? keyIt->get<std::string>() : "";
        }

        void ensureExpectedDraftRevision(const std::optional<int64_t>& expectedDraftRevision, int64_t currentRevision) {
            if (expectedDraftRevision && *expectedDraftRevision != currentRevision) {
                throw OCCConflictError("Draft revision conflict: expected " + std::to_string(*expectedDraftRevision) + ", got " +
                                       std::to_string(currentRevision));
            }
        }

        void ensureExpectedActiveRevision(const std::optional<std::uint64_t>& expectedActiveRevision, std::uint64_t activeRevision) {
            if (expectedActiveRevision && *expectedActiveRevision != activeRevision) {
                throw OCCConflictError("Active revision conflict: expected " + std::to_string(*expectedActiveRevision) + ", got " +
                                       std::to_string(activeRevision));
            }
        }

        void ensureDraftBaseRevision(std::uint64_t baseRevision, std::uint64_t activeRevision) {
            if (baseRevision != activeRevision) {
                throw OCCConflictError("Draft base revision does not match active revision");
            }
        }

        void validateMapping(const nlohmann::json& mapping) {
            MqttMapper::validate(mapping);
        }

        nlohmann::json readDraftEnvelope(const std::string& adminStorageRoot, const std::string& draftId) {
            const fs::path draftPath = getDraftFile(adminStorageRoot, draftId);

            if (!fs::exists(draftPath)) {
                throw EntityNotFoundError("Draft not found: " + sanitizeDraftId(draftId));
            }

            nlohmann::json draft = readJsonFromFile(draftPath);
            ensureDraftEnvelopeShape(draft, draftPath);

            return draft;
        }

        void writeDraftEnvelope(const std::string& adminStorageRoot, const std::string& draftId, const nlohmann::json& envelope) {
            writeJson(getDraftFile(adminStorageRoot, draftId), envelope);
        }

        void removeDraft(const std::string& adminStorageRoot, const std::string& draftId) {
            fs::remove(getDraftFile(adminStorageRoot, draftId));
        }

        nlohmann::json buildDeployMapping(const std::string& adminStorageRoot,
                                          const std::string& draftId,
                                          std::uint64_t activeRevision,
                                          const std::optional<std::uint64_t>& expectedActiveRevision) {
            ensureExpectedActiveRevision(expectedActiveRevision, activeRevision);

            nlohmann::json draft = readDraftEnvelope(adminStorageRoot, draftId);
            const std::uint64_t baseRevision = draft.value("base_revision", 0ULL);
            ensureDraftBaseRevision(baseRevision, activeRevision);

            nlohmann::json mapping = draft.at("mapping");
            validateMapping(mapping);

            setDeployMetadata(mapping, activeRevision + 1);
            mapping["meta"]["source_draft_id"] = sanitizeDraftId(draftId);

            return mapping;
        }

        std::string createDraftFromMapping(const std::string& adminStorageRoot,
                                           const nlohmann::json& activeMapping,
                                           std::uint64_t activeRevision,
                                           const std::string& draftId);

        template <typename MappingMutator>
        nlohmann::json mutateDraft(const std::string& adminStorageRoot,
                                   const std::string& draftId,
                                   std::optional<int64_t> expectedDraftRevision,
                                   MappingMutator&& mutator) {
            nlohmann::json envelope = readDraftEnvelope(adminStorageRoot, draftId);
            const int64_t currentRevision = readDraftRevisionValue(envelope, getDraftFile(adminStorageRoot, draftId));

            ensureExpectedDraftRevision(expectedDraftRevision, currentRevision);

            nlohmann::json updatedMapping = std::forward<MappingMutator>(mutator)(envelope.at("mapping"));
            validateMapping(updatedMapping);

            setDraftRevision(envelope, currentRevision + 1);
            envelope["mapping"] = std::move(updatedMapping);
            envelope["updated"] = nowIsoUtc();

            writeDraftEnvelope(adminStorageRoot, draftId, envelope);
            return envelope;
        }

        template <typename MappingMutator>
        nlohmann::json mutateDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                 const nlohmann::json& activeMapping,
                                                 std::uint64_t activeRevision,
                                                 const std::string& draftId,
                                                 std::optional<int64_t> expectedDraftRevision,
                                                 MappingMutator&& mutator) {
            nlohmann::json envelope;
            try {
                envelope = mutateDraft(adminStorageRoot, draftId, expectedDraftRevision, std::forward<MappingMutator>(mutator));
            } catch (const EntityNotFoundError&) {
                createDraftFromMapping(adminStorageRoot, activeMapping, activeRevision, draftId);
                envelope = mutateDraft(adminStorageRoot, draftId, expectedDraftRevision, std::forward<MappingMutator>(mutator));
            }
            return envelope;
        }

        ActiveState readActiveState(const ConfigApplication* configApplication) {
            return ActiveState{configApplication->getMqttMapper()->getMapping(), configApplication->getMqttMapper()->getRevision()};
        }

        JsonMappingReader::ApplyResult
        applyMappingAndPersist(ConfigApplication* configApplication, const nlohmann::json& mapping, const std::string& draftId) {
            const bool mustReconnect = configApplication->getMqttMapper()->setMapping(mapping);
            const bool mappingPersisted = configApplication->persistMapping();
            return JsonMappingReader::ApplyResult{
                configApplication->getMqttMapper()->getRevision(), draftId, mappingPersisted, mustReconnect};
        }

        void pruneVersions(const std::string& adminStorageRoot) {
            const fs::path versionDir = getVersionDir(adminStorageRoot);
            if (fs::exists(versionDir)) {
                std::vector<fs::path> versions;
                for (const auto& entry : fs::directory_iterator(versionDir)) {
                    if (entry.path().extension() == ".json") {
                        versions.push_back(entry.path());
                    }
                }
                if (versions.size() > MAX_HISTORY_ENTRIES) {
                    std::sort(versions.begin(), versions.end(), [](const fs::path& a, const fs::path& b) {
                        return fs::last_write_time(a) < fs::last_write_time(b);
                    });
                    for (std::size_t i = 0; i < versions.size() - MAX_HISTORY_ENTRIES; ++i) {
                        fs::remove(versions[i]);
                    }
                }
            }
        }

        void saveCurrentAsVersion(const std::string& adminStorageRoot, const nlohmann::json& activeMapping) {
            const fs::path versionDir = getVersionDir(adminStorageRoot);
            fs::create_directories(versionDir);

            const std::string snapshotId = makeUniqueId();
            const fs::path backupPath = versionDir / (snapshotId + ".json");

            writeJson(backupPath, activeMapping);
            pruneVersions(adminStorageRoot);
        }

        std::string createDraftFromMapping(const std::string& adminStorageRoot,
                                           const nlohmann::json& activeMapping,
                                           std::uint64_t activeRevision,
                                           const std::string& draftId) {
            const std::string resolvedDraftId = draftId.empty() ? makeUuidV4() : sanitizeDraftId(draftId);
            const fs::path draftPath = getDraftFile(adminStorageRoot, resolvedDraftId);
            if (fs::exists(draftPath)) {
                throw OCCConflictError("Draft already exists: " + resolvedDraftId);
            }

            const std::string createdAt = nowIsoUtc();

            nlohmann::json envelope = {{"id", resolvedDraftId},
                                       {"base_revision", activeRevision},
                                       {"draft_revision", 1},
                                       {"created", createdAt},
                                       {"updated", createdAt},
                                       {"mapping", activeMapping}};

            writeDraftEnvelope(adminStorageRoot, resolvedDraftId, envelope);
            return resolvedDraftId;
        }

    } // namespace

    nlohmann::json JsonMappingReader::createDraftFromActive(const std::string& adminStorageRoot,
                                                            ConfigApplication* configApplication,
                                                            const std::string& draftId) {
        const ActiveState activeState = readActiveState(configApplication);
        const std::string resolvedDraftId = createDraftFromMapping(adminStorageRoot, activeState.mapping, activeState.revision, draftId);
        return readDraftEnvelope(adminStorageRoot, resolvedDraftId);
    }

    std::vector<nlohmann::json> JsonMappingReader::listDrafts(const std::string& adminStorageRoot) {
        std::vector<nlohmann::json> drafts;
        const fs::path draftsDir = getDraftsDir(adminStorageRoot);
        if (fs::exists(draftsDir)) {
            for (const auto& entry : fs::directory_iterator(draftsDir)) {
                if (entry.path().filename().string().ends_with(".json")) {
                    try {
                        nlohmann::json envelope = readJsonFromFile(entry.path());
                        ensureDraftEnvelopeShape(envelope, entry.path());
                        envelope.erase("mapping");
                        drafts.push_back(std::move(envelope));
                    } catch (const std::exception&) {
                    }
                }
            }
        }
        std::sort(drafts.begin(), drafts.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            return a.value("updated", "") > b.value("updated", "");
        });
        return drafts;
    }

    nlohmann::json JsonMappingReader::readDraft(const std::string& adminStorageRoot, const std::string& draftId) {
        return readDraftEnvelope(adminStorageRoot, draftId);
    }

    std::optional<int64_t> JsonMappingReader::readDraftRevision(const std::string& adminStorageRoot, const std::string& draftId) {
        const nlohmann::json envelope = readDraftEnvelope(adminStorageRoot, draftId);
        return envelope["draft_revision"].get<int64_t>();
    }

    nlohmann::json JsonMappingReader::replaceDraft(const std::string& adminStorageRoot,
                                                   const std::string& draftId,
                                                   const nlohmann::json& mapping,
                                                   std::optional<int64_t> expectedDraftRevision) {
        return mutateDraft(adminStorageRoot, draftId, expectedDraftRevision, [&](const nlohmann::json&) {
            return mapping;
        });
    }

    nlohmann::json JsonMappingReader::patchDraft(const std::string& adminStorageRoot,
                                                 const std::string& draftId,
                                                 const nlohmann::json& patchOps,
                                                 std::optional<int64_t> expectedDraftRevision) {
        return mutateDraft(adminStorageRoot, draftId, expectedDraftRevision, [&](const nlohmann::json& currentMapping) {
            return currentMapping.patch(patchOps);
        });
    }

    nlohmann::json JsonMappingReader::replaceDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                                 ConfigApplication* configApplication,
                                                                 const std::string& draftId,
                                                                 const nlohmann::json& mapping,
                                                                 std::optional<int64_t> expectedDraftRevision) {
        const ActiveState activeState = readActiveState(configApplication);
        return mutateDraftWithAutoCreate(
            adminStorageRoot, activeState.mapping, activeState.revision, draftId, expectedDraftRevision, [&](const nlohmann::json&) {
                return mapping;
            });
    }

    nlohmann::json JsonMappingReader::patchDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                               ConfigApplication* configApplication,
                                                               const std::string& draftId,
                                                               const nlohmann::json& patchOps,
                                                               std::optional<int64_t> expectedDraftRevision) {
        const ActiveState activeState = readActiveState(configApplication);
        return mutateDraftWithAutoCreate(adminStorageRoot,
                                         activeState.mapping,
                                         activeState.revision,
                                         draftId,
                                         expectedDraftRevision,
                                         [&](const nlohmann::json& currentMapping) {
                                             return currentMapping.patch(patchOps);
                                         });
    }

    bool JsonMappingReader::isMappingValid(const nlohmann::json& mapping) {
        try {
            validateMapping(mapping);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool JsonMappingReader::isDraftValid(const std::string& adminStorageRoot, const std::string& draftId) {
        return isMappingValid(readDraftEnvelope(adminStorageRoot, draftId).at("mapping"));
    }

    void JsonMappingReader::discardDraft(const std::string& adminStorageRoot, const std::string& draftId) {
        removeDraft(adminStorageRoot, draftId);
    }

    std::vector<JsonMappingReader::VersionEntry> JsonMappingReader::getHistory(const std::string& adminStorageRoot) {
        std::vector<VersionEntry> history;
        fs::path versionDir = getVersionDir(adminStorageRoot);

        struct VersionSortEntry {
            VersionEntry entry;
            fs::file_time_type mtime;
        };
        std::vector<VersionSortEntry> versionEntries;

        if (fs::exists(versionDir)) {
            for (const auto& entry : fs::directory_iterator(versionDir)) {
                if (entry.path().extension() == ".json") {
                    VersionEntry v;
                    v.filename = entry.path().string();
                    v.snapshotId = entry.path().stem().string();

                    // Peek inside JSON to get the comment
                    try {
                        std::ifstream f(v.filename);
                        nlohmann::json j;
                        f >> j;
                        v.comment = readMetaString(j, "comment");
                        v.date = readMetaString(j, "deployed_at");
                        v.date = v.date.empty() ? readMetaString(j, "created") : v.date;
                    } catch (const std::exception&) {
                    }

                    v.date = v.date.empty() ? "Unknown" : v.date;

                    versionEntries.push_back(VersionSortEntry{std::move(v), entry.last_write_time()});
                }
            }
        }

        // Sort by file modification time (descending) to avoid assumptions about ID format.
        std::sort(versionEntries.begin(), versionEntries.end(), [](const VersionSortEntry& a, const VersionSortEntry& b) {
            return a.mtime > b.mtime;
        });

        history.reserve(versionEntries.size());
        for (auto& item : versionEntries) {
            history.push_back(std::move(item.entry));
        }

        return history;
    }

    JsonMappingReader::ApplyResult JsonMappingReader::deployAndApplyDraft(const std::string& adminStorageRoot,
                                                                          ConfigApplication* configApplication,
                                                                          const std::string& draftId,
                                                                          const std::optional<std::uint64_t>& expectedActiveRevision) {
        const ActiveState activeState = readActiveState(configApplication);
        nlohmann::json newMapping = buildDeployMapping(adminStorageRoot, draftId, activeState.revision, expectedActiveRevision);
        saveCurrentAsVersion(adminStorageRoot, activeState.mapping);
        removeDraft(adminStorageRoot, draftId);
        return applyMappingAndPersist(configApplication, newMapping, draftId);
    }

    JsonMappingReader::ApplyResult JsonMappingReader::rollbackAndApplyVersion(const std::string& adminStorageRoot,
                                                                              ConfigApplication* configApplication,
                                                                              const std::string& snapshotId,
                                                                              const std::optional<std::uint64_t>& expectedActiveRevision) {
        const ActiveState activeState = readActiveState(configApplication);
        const fs::path backupPath = getVersionDir(adminStorageRoot) / (snapshotId + ".json");
        if (!fs::exists(backupPath)) {
            throw EntityNotFoundError("Snapshot not found: " + snapshotId);
        }

        ensureExpectedActiveRevision(expectedActiveRevision, activeState.revision);

        nlohmann::json rollbackMapping = readJsonFromFile(backupPath);
        validateMapping(rollbackMapping);

        saveCurrentAsVersion(adminStorageRoot, activeState.mapping);

        setDeployMetadata(rollbackMapping, activeState.revision + 1);
        rollbackMapping["meta"]["rolled_back_from"] = snapshotId;

        return applyMappingAndPersist(configApplication, rollbackMapping, "");
    }

} // namespace mqtt::lib

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
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/file.h>
#include <system_error>
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

        std::atomic<std::uint64_t> idCounter{0};

        fs::path adminRootPath(const std::string& adminStorageRoot) {
            if (adminStorageRoot.empty()) {
                throw std::runtime_error("Admin storage root must not be empty");
            }

            const fs::path root = fs::path(adminStorageRoot);
            fs::create_directories(root);
            return root;
        }

        fs::path getAdminLockFile(const std::string& adminStorageRoot) {
            return adminRootPath(adminStorageRoot) / "admin.lock";
        }

        class FileLock {
        public:
            explicit FileLock(const fs::path& lockFilePath, int lockType)
                : lockPath(lockFilePath.string()) {
                fd = ::open(lockPath.c_str(), O_CREAT | O_RDWR, 0644);
                if (fd < 0) {
                    throw std::system_error(errno, std::generic_category(), "open lock file failed: " + lockPath);
                }

                if (::flock(fd, lockType) != 0) {
                    const int err = errno;
                    ::close(fd);
                    fd = -1;
                    throw std::system_error(err, std::generic_category(), "lock acquisition failed: " + lockPath);
                }
            }

            ~FileLock() {
                if (fd >= 0) {
                    ::flock(fd, LOCK_UN);
                    ::close(fd);
                }
            }

            FileLock(const FileLock&) = delete;
            FileLock& operator=(const FileLock&) = delete;

        private:
            int fd{-1};
            std::string lockPath;
        };

        class ExclusiveFileLock : public FileLock {
        public:
            explicit ExclusiveFileLock(const std::string& adminStorageRoot)
                : FileLock(getAdminLockFile(adminStorageRoot), LOCK_EX) {
            }
        };

        class SharedFileLock : public FileLock {
        public:
            explicit SharedFileLock(const std::string& adminStorageRoot)
                : FileLock(getAdminLockFile(adminStorageRoot), LOCK_SH) {
            }
        };

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
            const auto localCounter = idCounter.fetch_add(1, std::memory_order_relaxed);
            return std::to_string(ns) + "-" + std::to_string(static_cast<long long>(::getpid())) + "-" + std::to_string(localCounter);
        }

        std::string makeUuidV4() {
            static thread_local std::mt19937_64 rng(std::random_device{}());
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

        void fsyncPath(const fs::path& path) {
            const int fd = ::open(path.c_str(), O_RDONLY);
            if (fd < 0) {
                throw std::system_error(errno, std::generic_category(), "open for fsync failed: " + path.string());
            }

            if (::fsync(fd) != 0) {
                const int err = errno;
                ::close(fd);
                throw std::system_error(err, std::generic_category(), "fsync failed: " + path.string());
            }

            ::close(fd);
        }

        void fsyncDirectory(const fs::path& directoryPath) {
            const int fd = ::open(directoryPath.c_str(), O_RDONLY | O_DIRECTORY);
            if (fd < 0) {
                throw std::system_error(errno, std::generic_category(), "open directory for fsync failed: " + directoryPath.string());
            }

            if (::fsync(fd) != 0) {
                const int err = errno;
                ::close(fd);
                throw std::system_error(err, std::generic_category(), "directory fsync failed: " + directoryPath.string());
            }

            ::close(fd);
        }

        void writeJsonAtomically(const fs::path& targetPath, const nlohmann::json& j) {
            const fs::path parentDir = targetPath.parent_path().empty() ? fs::path(".") : targetPath.parent_path();
            fs::create_directories(parentDir);

            const fs::path tempPath = targetPath.string() + ".tmp." + makeUniqueId();
            {
                std::ofstream out(tempPath, std::ios::trunc);
                if (!out) {
                    throw std::runtime_error("Cannot open temp file for writing: " + tempPath.string());
                }
                out << j.dump(2) << std::endl;
            }

            fsyncPath(tempPath);
            fs::rename(tempPath, targetPath);
            fsyncDirectory(parentDir);
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

        nlohmann::json readDraftEnvelopeNoLock(const std::string& adminStorageRoot, const std::string& draftId) {
            const fs::path draftPath = getDraftFile(adminStorageRoot, draftId);

            if (!fs::exists(draftPath)) {
                throw EntityNotFoundError("Draft not found: " + sanitizeDraftId(draftId));
            }

            nlohmann::json draft = readJsonFromFile(draftPath);
            ensureDraftEnvelopeShape(draft, draftPath);

            return draft;
        }

        void writeDraftEnvelopeNoLock(const std::string& adminStorageRoot, const std::string& draftId, const nlohmann::json& envelope) {
            writeJsonAtomically(getDraftFile(adminStorageRoot, draftId), envelope);
        }

        void removeDraftNoLock(const std::string& adminStorageRoot, const std::string& draftId) {
            fs::remove(getDraftFile(adminStorageRoot, draftId));
        }

        nlohmann::json buildDeployMappingNoLock(const std::string& adminStorageRoot,
                                                const std::string& draftId,
                                                std::uint64_t activeRevision,
                                                const std::optional<std::uint64_t>& expectedActiveRevision) {
            ensureExpectedActiveRevision(expectedActiveRevision, activeRevision);

            nlohmann::json draft = readDraftEnvelopeNoLock(adminStorageRoot, draftId);
            const std::uint64_t baseRevision = draft.value("base_revision", 0ULL);
            ensureDraftBaseRevision(baseRevision, activeRevision);

            nlohmann::json mapping = draft.at("mapping");
            validateMapping(mapping);

            setDeployMetadata(mapping, activeRevision + 1);
            mapping["meta"]["source_draft_id"] = sanitizeDraftId(draftId);

            return mapping;
        }

        std::string createDraftFromMappingNoLock(const std::string& adminStorageRoot,
                                                 const nlohmann::json& activeMapping,
                                                 std::uint64_t activeRevision,
                                                 const std::string& draftId);

        template <typename MappingMutator>
        nlohmann::json mutateDraftNoLock(const std::string& adminStorageRoot,
                                         const std::string& draftId,
                                         std::optional<int64_t> expectedDraftRevision,
                                         MappingMutator&& mutator) {
            nlohmann::json envelope = readDraftEnvelopeNoLock(adminStorageRoot, draftId);
            const int64_t currentRevision = readDraftRevisionValue(envelope, getDraftFile(adminStorageRoot, draftId));

            ensureExpectedDraftRevision(expectedDraftRevision, currentRevision);

            nlohmann::json updatedMapping = std::forward<MappingMutator>(mutator)(envelope.at("mapping"));
            validateMapping(updatedMapping);

            setDraftRevision(envelope, currentRevision + 1);
            envelope["mapping"] = std::move(updatedMapping);
            envelope["updated"] = nowIsoUtc();

            writeDraftEnvelopeNoLock(adminStorageRoot, draftId, envelope);
            return envelope;
        }

        template <typename MappingMutator>
        nlohmann::json mutateDraftWithAutoCreateNoLock(const std::string& adminStorageRoot,
                                                       const nlohmann::json& activeMapping,
                                                       std::uint64_t activeRevision,
                                                       const std::string& draftId,
                                                       std::optional<int64_t> expectedDraftRevision,
                                                       MappingMutator&& mutator) {
            try {
                return mutateDraftNoLock(adminStorageRoot, draftId, expectedDraftRevision, std::forward<MappingMutator>(mutator));
            } catch (const EntityNotFoundError&) {
                createDraftFromMappingNoLock(adminStorageRoot, activeMapping, activeRevision, draftId);
                return mutateDraftNoLock(adminStorageRoot, draftId, expectedDraftRevision, std::forward<MappingMutator>(mutator));
            }
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

        template <typename Operation>
        auto withActiveStateLock(const std::string& adminStorageRoot, ConfigApplication* configApplication, Operation&& operation) {
            ExclusiveFileLock lock(adminStorageRoot);
            const ActiveState activeState = readActiveState(configApplication);
            return std::forward<Operation>(operation)(activeState);
        }

        void pruneVersionsNoLock(const std::string& adminStorageRoot) {
            const fs::path versionDir = getVersionDir(adminStorageRoot);
            if (!fs::exists(versionDir)) {
                return;
            }

            std::vector<fs::path> versions;
            for (const auto& entry : fs::directory_iterator(versionDir)) {
                if (entry.path().extension() == ".json") {
                    versions.push_back(entry.path());
                }
            }

            if (versions.size() <= MAX_HISTORY_ENTRIES) {
                return;
            }

            std::sort(versions.begin(), versions.end(), [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) < fs::last_write_time(b);
            });

            for (std::size_t i = 0; i < versions.size() - MAX_HISTORY_ENTRIES; ++i) {
                fs::remove(versions[i]);
            }
        }

        void saveCurrentAsVersionNoLock(const std::string& adminStorageRoot, const nlohmann::json& activeMapping) {
            const fs::path versionDir = getVersionDir(adminStorageRoot);
            fs::create_directories(versionDir);

            const std::string snapshotId = makeUniqueId();
            const fs::path backupPath = versionDir / (snapshotId + ".json");

            writeJsonAtomically(backupPath, activeMapping);
            pruneVersionsNoLock(adminStorageRoot);
        }

        std::string createDraftFromMappingNoLock(const std::string& adminStorageRoot,
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

            writeDraftEnvelopeNoLock(adminStorageRoot, resolvedDraftId, envelope);
            return resolvedDraftId;
        }

    } // namespace

    nlohmann::json JsonMappingReader::createDraftFromActive(const std::string& adminStorageRoot,
                                                            ConfigApplication* configApplication,
                                                            const std::string& draftId) {
        return withActiveStateLock(adminStorageRoot, configApplication, [&](const ActiveState& activeState) {
            const std::string resolvedDraftId =
                createDraftFromMappingNoLock(adminStorageRoot, activeState.mapping, activeState.revision, draftId);
            return readDraftEnvelopeNoLock(adminStorageRoot, resolvedDraftId);
        });
    }

    std::vector<nlohmann::json> JsonMappingReader::listDrafts(const std::string& adminStorageRoot) {
        SharedFileLock lock(adminStorageRoot);

        std::vector<nlohmann::json> drafts;
        const fs::path draftsDir = getDraftsDir(adminStorageRoot);
        if (!fs::exists(draftsDir)) {
            return drafts;
        }

        for (const auto& entry : fs::directory_iterator(draftsDir)) {
            if (!entry.path().filename().string().ends_with(".json")) {
                continue;
            }

            try {
                nlohmann::json envelope = readJsonFromFile(entry.path());
                ensureDraftEnvelopeShape(envelope, entry.path());
                envelope.erase("mapping");
                drafts.push_back(std::move(envelope));
            } catch (const std::exception&) {
            }
        }

        std::sort(drafts.begin(), drafts.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            return a.value("updated", "") > b.value("updated", "");
        });

        return drafts;
    }

    nlohmann::json JsonMappingReader::readDraft(const std::string& adminStorageRoot, const std::string& draftId) {
        SharedFileLock lock(adminStorageRoot);
        return readDraftEnvelopeNoLock(adminStorageRoot, draftId);
    }

    std::optional<int64_t> JsonMappingReader::readDraftRevision(const std::string& adminStorageRoot, const std::string& draftId) {
        SharedFileLock lock(adminStorageRoot);
        const nlohmann::json envelope = readDraftEnvelopeNoLock(adminStorageRoot, draftId);
        return envelope["draft_revision"].get<int64_t>();
    }

    nlohmann::json JsonMappingReader::replaceDraft(const std::string& adminStorageRoot,
                                                   const std::string& draftId,
                                                   const nlohmann::json& mapping,
                                                   std::optional<int64_t> expectedDraftRevision) {
        ExclusiveFileLock lock(adminStorageRoot);
        return mutateDraftNoLock(adminStorageRoot, draftId, expectedDraftRevision, [&](const nlohmann::json&) {
            return mapping;
        });
    }

    nlohmann::json JsonMappingReader::patchDraft(const std::string& adminStorageRoot,
                                                 const std::string& draftId,
                                                 const nlohmann::json& patchOps,
                                                 std::optional<int64_t> expectedDraftRevision) {
        ExclusiveFileLock lock(adminStorageRoot);
        return mutateDraftNoLock(adminStorageRoot, draftId, expectedDraftRevision, [&](const nlohmann::json& currentMapping) {
            return currentMapping.patch(patchOps);
        });
    }

    nlohmann::json JsonMappingReader::replaceDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                                 ConfigApplication* configApplication,
                                                                 const std::string& draftId,
                                                                 const nlohmann::json& mapping,
                                                                 std::optional<int64_t> expectedDraftRevision) {
        return withActiveStateLock(adminStorageRoot, configApplication, [&](const ActiveState& activeState) {
            return mutateDraftWithAutoCreateNoLock(
                adminStorageRoot, activeState.mapping, activeState.revision, draftId, expectedDraftRevision, [&](const nlohmann::json&) {
                    return mapping;
                });
        });
    }

    nlohmann::json JsonMappingReader::patchDraftWithAutoCreate(const std::string& adminStorageRoot,
                                                               ConfigApplication* configApplication,
                                                               const std::string& draftId,
                                                               const nlohmann::json& patchOps,
                                                               std::optional<int64_t> expectedDraftRevision) {
        return withActiveStateLock(adminStorageRoot, configApplication, [&](const ActiveState& activeState) {
            return mutateDraftWithAutoCreateNoLock(adminStorageRoot,
                                                   activeState.mapping,
                                                   activeState.revision,
                                                   draftId,
                                                   expectedDraftRevision,
                                                   [&](const nlohmann::json& currentMapping) {
                                                       return currentMapping.patch(patchOps);
                                                   });
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
        SharedFileLock lock(adminStorageRoot);
        return isMappingValid(readDraftEnvelopeNoLock(adminStorageRoot, draftId).at("mapping"));
    }

    void JsonMappingReader::discardDraft(const std::string& adminStorageRoot, const std::string& draftId) {
        ExclusiveFileLock lock(adminStorageRoot);
        removeDraftNoLock(adminStorageRoot, draftId);
    }

    std::vector<JsonMappingReader::VersionEntry> JsonMappingReader::getHistory(const std::string& adminStorageRoot) {
        SharedFileLock lock(adminStorageRoot);

        std::vector<VersionEntry> history;
        fs::path versionDir = getVersionDir(adminStorageRoot);

        struct VersionSortEntry {
            VersionEntry entry;
            fs::file_time_type mtime;
        };
        std::vector<VersionSortEntry> versionEntries;

        if (!fs::exists(versionDir))
            return history;

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
        return withActiveStateLock(adminStorageRoot, configApplication, [&](const ActiveState& activeState) {
            nlohmann::json newMapping = buildDeployMappingNoLock(adminStorageRoot, draftId, activeState.revision, expectedActiveRevision);
            saveCurrentAsVersionNoLock(adminStorageRoot, activeState.mapping);
            removeDraftNoLock(adminStorageRoot, draftId);
            return applyMappingAndPersist(configApplication, newMapping, draftId);
        });
    }

    JsonMappingReader::ApplyResult JsonMappingReader::rollbackAndApplyVersion(const std::string& adminStorageRoot,
                                                                              ConfigApplication* configApplication,
                                                                              const std::string& snapshotId,
                                                                              const std::optional<std::uint64_t>& expectedActiveRevision) {
        return withActiveStateLock(adminStorageRoot, configApplication, [&](const ActiveState& activeState) {
            const fs::path backupPath = getVersionDir(adminStorageRoot) / (snapshotId + ".json");
            if (!fs::exists(backupPath)) {
                throw EntityNotFoundError("Snapshot not found: " + snapshotId);
            }

            ensureExpectedActiveRevision(expectedActiveRevision, activeState.revision);

            nlohmann::json rollbackMapping = readJsonFromFile(backupPath);
            validateMapping(rollbackMapping);

            saveCurrentAsVersionNoLock(adminStorageRoot, activeState.mapping);

            setDeployMetadata(rollbackMapping, activeState.revision + 1);
            rollbackMapping["meta"]["rolled_back_from"] = snapshotId;

            return applyMappingAndPersist(configApplication, rollbackMapping, "");
        });
    }

} // namespace mqtt::lib

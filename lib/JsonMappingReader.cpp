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

#include "MqttMapper.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// #include "nlohmann/json-schema.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <compare>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <log/Logger.h>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <system_error>
#include <unistd.h>

#endif

namespace mqtt::lib {

    namespace fs = std::filesystem;

    namespace {

        constexpr const char* LEGACY_DRAFT_ID = "default";
        constexpr std::size_t MAX_HISTORY_ENTRIES = 50;

        std::atomic<std::uint64_t> idCounter{0};

        class FileLock {
        public:
            explicit FileLock(const std::string& mapFilePath, int lockType) {
                const std::string lockPath = mapFilePath + ".lock";
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
        };

        class ExclusiveFileLock : public FileLock {
        public:
            explicit ExclusiveFileLock(const std::string& mapFilePath) : FileLock(mapFilePath, LOCK_EX) {}
        };

        class SharedFileLock : public FileLock {
        public:
            explicit SharedFileLock(const std::string& mapFilePath) : FileLock(mapFilePath, LOCK_SH) {}
        };

        std::string nowIsoUtc() {
            const auto now = std::chrono::system_clock::now();
            const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");
            return ss.str();
        }

        std::string makeUniqueId() {
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            const auto localCounter = idCounter.fetch_add(1, std::memory_order_relaxed);
            return std::to_string(ns) + "-" + std::to_string(static_cast<long long>(::getpid())) + "-" + std::to_string(localCounter);
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

        fs::path toMapPath(const std::string& mapFilePath) {
            return fs::path(mapFilePath);
        }

        fs::path getDraftsDir(const std::string& mapFilePath) {
            const fs::path mapPath = toMapPath(mapFilePath);
            return mapPath.parent_path() / "drafts" / mapPath.filename();
        }

        fs::path getDraftFile(const std::string& mapFilePath, const std::string& draftId) {
            return getDraftsDir(mapFilePath) / (sanitizeDraftId(draftId) + ".json");
        }

        fs::path getVersionDir(const std::string& mapFilePath) {
            return toMapPath(mapFilePath).parent_path() / "versions";
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
            fs::create_directories(targetPath.parent_path());

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
            fsyncDirectory(targetPath.parent_path());
        }

        void ensureMetaObject(nlohmann::json& mapping) {
            if (!mapping.contains("meta") || !mapping["meta"].is_object()) {
                mapping["meta"] = nlohmann::json::object();
            }
        }

        std::uint64_t extractRevision(const nlohmann::json& mapping) {
            if (!mapping.contains("meta") || !mapping["meta"].is_object()) {
                return 0;
            }

            const auto& meta = mapping["meta"];
            if (!meta.contains("revision")) {
                return 0;
            }

            try {
                if (meta["revision"].is_number_unsigned()) {
                    return meta["revision"].get<std::uint64_t>();
                }
                if (meta["revision"].is_number_integer()) {
                    const auto value = meta["revision"].get<long long>();
                    return value >= 0 ? static_cast<std::uint64_t>(value) : 0;
                }
                if (meta["revision"].is_string()) {
                    return static_cast<std::uint64_t>(std::stoull(meta["revision"].get<std::string>()));
                }
            } catch (...) {
            }

            return 0;
        }

        int64_t extractDraftRevision(const nlohmann::json& envelope) {
            if (envelope.contains("meta") && envelope["meta"].is_object() && envelope["meta"].contains("draft_revision") &&
                envelope["meta"]["draft_revision"].is_number_integer()) {
                return envelope["meta"]["draft_revision"].get<int64_t>();
            }

            if (envelope.contains("draft_revision") && envelope["draft_revision"].is_number_integer()) {
                return envelope["draft_revision"].get<int64_t>();
            }

            return 1;
        }

        void setDraftRevision(nlohmann::json& envelope, int64_t draftRevision) {
            if (!envelope.contains("meta") || !envelope["meta"].is_object()) {
                envelope["meta"] = nlohmann::json::object();
            }
            envelope["meta"]["draft_revision"] = draftRevision;
        }

        void ensureExpectedDraftRevision(const std::optional<int64_t>& expectedDraftRevision, int64_t currentRevision) {
            if (expectedDraftRevision.has_value() && expectedDraftRevision.value() != currentRevision) {
                throw OCCConflictError(
                    "Draft revision conflict: expected " + std::to_string(expectedDraftRevision.value()) + ", got " + std::to_string(currentRevision));
            }
        }

        void ensureExpectedActiveRevision(const std::optional<std::uint64_t>& expectedActiveRevision, std::uint64_t activeRevision) {
            if (expectedActiveRevision.has_value() && expectedActiveRevision.value() != activeRevision) {
                throw OCCConflictError("Active revision mismatch");
            }
        }

        void ensureDraftBaseRevision(std::uint64_t baseRevision, std::uint64_t activeRevision) {
            if (baseRevision != activeRevision) {
                throw OCCConflictError("Draft base revision does not match active revision");
            }
        }

        nlohmann::json readActiveNoLock(const std::string& mapFilePath) {
            return readJsonFromFile(toMapPath(mapFilePath));
        }

        void validateMapping(const nlohmann::json& mapping) {
            if (!mapping.is_object()) {
                throw std::runtime_error("Mapping must be a JSON object");
            }
            MqttMapper::validate(mapping);
        }

        nlohmann::json readDraftEnvelopeNoLock(const std::string& mapFilePath, const std::string& draftId) {
            const fs::path draftPath = getDraftFile(mapFilePath, draftId);
            if (!fs::exists(draftPath)) {
                throw EntityNotFoundError("Draft not found: " + sanitizeDraftId(draftId));
            }

            nlohmann::json draft = readJsonFromFile(draftPath);
            if (!draft.is_object() || !draft.contains("mapping") || !draft["mapping"].is_object()) {
                throw std::runtime_error("Draft file is malformed: " + draftPath.string());
            }

            if (!draft.contains("id") || !draft["id"].is_string()) {
                draft["id"] = sanitizeDraftId(draftId);
            }
            if (!draft.contains("base_revision")) {
                draft["base_revision"] = 0;
            }

            return draft;
        }

        void writeDraftEnvelopeNoLock(const std::string& mapFilePath, const std::string& draftId, const nlohmann::json& envelope) {
            writeJsonAtomically(getDraftFile(mapFilePath, draftId), envelope);
        }

        template <typename MappingMutator>
        nlohmann::json mutateDraftNoLock(const std::string& mapFilePath,
                                         const std::string& draftId,
                                         std::optional<int64_t> expectedDraftRevision,
                                         MappingMutator&& mutator) {
            nlohmann::json envelope = readDraftEnvelopeNoLock(mapFilePath, draftId);
            const int64_t currentRevision = extractDraftRevision(envelope);

            ensureExpectedDraftRevision(expectedDraftRevision, currentRevision);

            nlohmann::json updatedMapping = std::forward<MappingMutator>(mutator)(envelope.at("mapping"));
            validateMapping(updatedMapping);

            setDraftRevision(envelope, currentRevision + 1);
            envelope["mapping"] = std::move(updatedMapping);
            envelope["updated"] = nowIsoUtc();

            writeDraftEnvelopeNoLock(mapFilePath, draftId, envelope);
            return envelope;
        }

        void pruneVersionsNoLock(const std::string& mapFilePath) {
            const fs::path versionDir = getVersionDir(mapFilePath);
            const std::string baseName = toMapPath(mapFilePath).filename().string();

            std::vector<fs::path> versions;
            for (const auto& entry : fs::directory_iterator(versionDir)) {
                if (entry.path().filename().string().starts_with(baseName + ".")) {
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

        void saveCurrentAsVersionNoLock(const std::string& mapFilePath, bool enableVersioning) {
            if (!enableVersioning || !fs::exists(toMapPath(mapFilePath))) {
                return;
            }

            const fs::path versionDir = getVersionDir(mapFilePath);
            fs::create_directories(versionDir);

            const std::string baseName = toMapPath(mapFilePath).filename().string();
            const std::string versionId = makeUniqueId();
            const fs::path backupPath = versionDir / (baseName + "." + versionId);

            fs::copy_file(toMapPath(mapFilePath), backupPath, fs::copy_options::none);
            pruneVersionsNoLock(mapFilePath);
        }

    } // namespace

    nlohmann::json JsonMappingReader::readMappingFromFile(const std::string& mapFilePath) {
        if (mapFilePath.empty()) {
            throw std::runtime_error("MappingFile not set");
        }
        return readJsonFromFile(mapFilePath);
    }

    nlohmann::json JsonMappingReader::readActive(const std::string& mapFilePath) {
        SharedFileLock lock(mapFilePath);
        return readActiveNoLock(mapFilePath);
    }

    std::uint64_t JsonMappingReader::readActiveRevision(const std::string& mapFilePath) {
        SharedFileLock lock(mapFilePath);
        return extractRevision(readActiveNoLock(mapFilePath));
    }

    std::string JsonMappingReader::getDraftsDirPath(const std::string& mapFilePath) {
        return getDraftsDir(mapFilePath).string();
    }

    std::string JsonMappingReader::getDraftPath(const std::string& mapFilePath, const std::string& draftId) {
        return getDraftFile(mapFilePath, draftId).string();
    }

    std::string JsonMappingReader::createDraftFromActive(const std::string& mapFilePath, const std::string& draftId) {
        ExclusiveFileLock lock(mapFilePath);

        const std::string resolvedDraftId = draftId.empty() ? makeUniqueId() : sanitizeDraftId(draftId);
        const fs::path draftPath = getDraftFile(mapFilePath, resolvedDraftId);
        if (fs::exists(draftPath)) {
            throw OCCConflictError("Draft already exists: " + resolvedDraftId);
        }

        const nlohmann::json active = readActiveNoLock(mapFilePath);
        nlohmann::json envelope = {
            {"id", resolvedDraftId},
            {"base_revision", extractRevision(active)},
            {"created", nowIsoUtc()},
            {"updated", nowIsoUtc()},
            {"mapping", active},
            {"meta", {{"draft_revision", 1}}}
        };

        writeDraftEnvelopeNoLock(mapFilePath, resolvedDraftId, envelope);
        return resolvedDraftId;
    }

    std::vector<nlohmann::json> JsonMappingReader::listDrafts(const std::string& mapFilePath) {
        SharedFileLock lock(mapFilePath);

        std::vector<nlohmann::json> drafts;
        const fs::path draftsDir = getDraftsDir(mapFilePath);
        if (!fs::exists(draftsDir)) {
            return drafts;
        }

        for (const auto& entry : fs::directory_iterator(draftsDir)) {
            if (!entry.path().filename().string().ends_with(".json")) {
                continue;
            }

            try {
                nlohmann::json envelope = readJsonFromFile(entry.path());
                if (!envelope.is_object()) {
                    continue;
                }

                envelope.erase("mapping");
                drafts.push_back(envelope);
            } catch (...) {
            }
        }

        std::sort(drafts.begin(), drafts.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            return a.value("updated", "") > b.value("updated", "");
        });

        return drafts;
    }

    nlohmann::json JsonMappingReader::readDraft(const std::string& mapFilePath, const std::string& draftId) {
        SharedFileLock lock(mapFilePath);
        return readDraftEnvelopeNoLock(mapFilePath, draftId);
    }

    nlohmann::json JsonMappingReader::replaceDraft(const std::string& mapFilePath, const std::string& draftId, const nlohmann::json& mapping, std::optional<int64_t> expectedDraftRevision) {
        ExclusiveFileLock lock(mapFilePath);
        return mutateDraftNoLock(mapFilePath, draftId, expectedDraftRevision, [&](const nlohmann::json&) {
            return mapping;
        });
    }

    nlohmann::json JsonMappingReader::patchDraft(const std::string& mapFilePath, const std::string& draftId, const nlohmann::json& patchOps, std::optional<int64_t> expectedDraftRevision) {
        ExclusiveFileLock lock(mapFilePath);
        return mutateDraftNoLock(mapFilePath, draftId, expectedDraftRevision, [&](const nlohmann::json& currentMapping) {
            return currentMapping.patch(patchOps);
        });
    }

    void JsonMappingReader::discardDraft(const std::string& mapFilePath, const std::string& draftId) {
        ExclusiveFileLock lock(mapFilePath);

        const fs::path draftPath = getDraftFile(mapFilePath, draftId);
        if (fs::exists(draftPath)) {
            fs::remove(draftPath);
        }
    }

    void JsonMappingReader::saveDraft(const std::string& mapFilePath, const nlohmann::json& content) {
        try {
            replaceDraft(mapFilePath, LEGACY_DRAFT_ID, content);
        } catch (const EntityNotFoundError&) {
            createDraftFromActive(mapFilePath, LEGACY_DRAFT_ID);
            replaceDraft(mapFilePath, LEGACY_DRAFT_ID, content);
        }
    }

    nlohmann::json JsonMappingReader::readDraftOrActive(const std::string& mapFilePath) {
        try {
            return readDraft(mapFilePath, LEGACY_DRAFT_ID).at("mapping");
        } catch (const EntityNotFoundError&) {
            return readActive(mapFilePath);
        }
    }

    nlohmann::json JsonMappingReader::deployDraft(const std::string& mapFilePath, bool enableVersioning) {
        return deployDraft(mapFilePath, LEGACY_DRAFT_ID, enableVersioning, std::nullopt);
    }

    std::string JsonMappingReader::getDraftPath(const std::string& mapFilePath) {
        return getDraftPath(mapFilePath, LEGACY_DRAFT_ID);
    }

    nlohmann::json JsonMappingReader::deployDraft(const std::string& mapFilePath,
                                                  const std::string& draftId,
                                                  bool enableVersioning,
                                                  const std::optional<std::uint64_t>& expectedActiveRevision) {
        ExclusiveFileLock lock(mapFilePath);

        nlohmann::json active = readActiveNoLock(mapFilePath);
        const std::uint64_t activeRevision = extractRevision(active);
        ensureExpectedActiveRevision(expectedActiveRevision, activeRevision);

        nlohmann::json draft = readDraftEnvelopeNoLock(mapFilePath, draftId);
        const std::uint64_t baseRevision = draft.value("base_revision", 0ULL);
        ensureDraftBaseRevision(baseRevision, activeRevision);

        nlohmann::json mapping = draft.at("mapping");
        validateMapping(mapping);

        saveCurrentAsVersionNoLock(mapFilePath, enableVersioning);

        ensureMetaObject(mapping);
        mapping["meta"]["created"] = nowIsoUtc();
        mapping["meta"]["version"] = makeUniqueId();
        mapping["meta"]["revision"] = activeRevision + 1;
        mapping["meta"]["source_draft_id"] = sanitizeDraftId(draftId);

        writeJsonAtomically(toMapPath(mapFilePath), mapping);
        fs::remove(getDraftFile(mapFilePath, draftId));

        return mapping;
    }

    void JsonMappingReader::discardDraft(const std::string& mapFilePath) {
        discardDraft(mapFilePath, LEGACY_DRAFT_ID);
    }

    std::vector<JsonMappingReader::VersionEntry> JsonMappingReader::getHistory(const std::string& mapFilePath) {
        SharedFileLock lock(mapFilePath);

        std::vector<VersionEntry> history;
        fs::path versionDir = getVersionDir(mapFilePath);
        std::string baseName = toMapPath(mapFilePath).filename().string();

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

    nlohmann::json JsonMappingReader::rollbackTo(const std::string& mapFilePath,
                                                 const std::string& versionId,
                                                 bool enableVersioning,
                                                 const std::optional<std::uint64_t>& expectedActiveRevision) {
        ExclusiveFileLock lock(mapFilePath);

        fs::path versionDir = getVersionDir(mapFilePath);
        std::string baseName = toMapPath(mapFilePath).filename().string();
        fs::path backupPath = versionDir / (baseName + "." + versionId);

        if (!fs::exists(backupPath)) {
            throw EntityNotFoundError("Version not found: " + versionId);
        }

        nlohmann::json active = readActiveNoLock(mapFilePath);
        const std::uint64_t activeRevision = extractRevision(active);
        ensureExpectedActiveRevision(expectedActiveRevision, activeRevision);

        nlohmann::json rollbackMapping = readJsonFromFile(backupPath);
        validateMapping(rollbackMapping);

        saveCurrentAsVersionNoLock(mapFilePath, enableVersioning);

        ensureMetaObject(rollbackMapping);
        rollbackMapping["meta"]["created"] = nowIsoUtc();
        rollbackMapping["meta"]["version"] = makeUniqueId();
        rollbackMapping["meta"]["revision"] = activeRevision + 1;
        rollbackMapping["meta"]["rolled_back_from"] = versionId;

        writeJsonAtomically(toMapPath(mapFilePath), rollbackMapping);
        return rollbackMapping;
    }

    nlohmann::json JsonMappingReader::rollbackTo(const std::string& mapFilePath, const std::string& versionId) {
        return rollbackTo(mapFilePath, versionId, true, std::nullopt);
    }

} // namespace mqtt::lib

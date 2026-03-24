/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Tobias Pfeil
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

#include "MappingAdminRouter.h"

#include "ConfigApplication.h"
#include "JsonMappingReader.h"
#include "MqttMapper.h"

#include <express/middleware/BasicAuthentication.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <memory>
#include <stdlib.h>
#include <cstdint>
#include <system_error>
#include <utility>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace {

    namespace fs = std::filesystem;

    class RuntimeMappingWorkspace {
    public:
        explicit RuntimeMappingWorkspace(const nlohmann::json& mappingJson)
            : workspaceDir(createWorkspaceDir())
            , mappingFilePath(workspaceDir / "mapping.json")
            , mappingFilePathString(mappingFilePath.string()) {
            std::ofstream out(mappingFilePath, std::ios::trunc);
            if (!out) {
                throw std::runtime_error("Cannot create runtime mapping file: " + mappingFilePathString);
            }

            out << mappingJson.dump(2) << std::endl;
        }

        ~RuntimeMappingWorkspace() {
            std::error_code ec;
            fs::remove_all(workspaceDir, ec);
        }

        const std::string& getMappingFilePath() const {
            return mappingFilePathString;
        }

    private:
        static fs::path getTempBaseDir() {
            std::error_code ec;
            fs::path tempDir = fs::temp_directory_path(ec);
            if (ec || tempDir.empty()) {
                return "/tmp";
            }

            return tempDir;
        }

        static fs::path createWorkspaceDir() {
            const fs::path baseDir = getTempBaseDir();
            std::string dirTemplate = (baseDir / "mqttsuite-integrator-XXXXXX").string();

            std::vector<char> mutableTemplate(dirTemplate.begin(), dirTemplate.end());
            mutableTemplate.push_back('\0');

            char* createdPath = ::mkdtemp(mutableTemplate.data());
            if (createdPath == nullptr) {
                throw std::runtime_error("Cannot create runtime mapping workspace in temp directory: " + dirTemplate);
            }

            return createdPath;
        }

        fs::path workspaceDir;
        fs::path mappingFilePath;
        std::string mappingFilePathString;
    };

    struct AdminMappingContext {
        std::string mappingFilePath;
        bool enableVersioning{true};
        std::unique_ptr<RuntimeMappingWorkspace> runtimeMappingWorkspace;
    };

    std::optional<std::uint64_t> parseRevisionToken(const std::string& rawValue) {
        if (rawValue.empty()) {
            return std::nullopt;
        }

        std::string value = rawValue;

        if (value.starts_with("W/")) {
            value = value.substr(2);
        }

        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (value.starts_with("rev-")) {
            value = value.substr(4);
        }

        if (value.empty()) {
            return std::nullopt;
        }

        try {
            return static_cast<std::uint64_t>(std::stoull(value));
        } catch (...) {
            return std::nullopt;
        }
    }

    template <typename RequestPtr>
    std::optional<std::uint64_t> resolveExpectedRevision(const RequestPtr& req, const nlohmann::json* body) {
        if (body != nullptr && body->is_object() && body->contains("expected_revision")) {
            try {
                return (*body).at("expected_revision").get<std::uint64_t>();
            } catch (...) {
            }
        }

        return parseRevisionToken(req->get("If-Match"));
    }

    template <typename RequestPtr>
    std::string resolveDraftId(const RequestPtr& req, const nlohmann::json* body) {
        if (body != nullptr && body->is_object() && body->contains("draft_id") && (*body).at("draft_id").is_string()) {
            return (*body).at("draft_id").get<std::string>();
        }

        const std::string headerDraftId = req->get("X-Draft-Id");
        if (!headerDraftId.empty()) {
            return headerDraftId;
        }

        return "default";
    }

    template <typename ResponsePtr>
    void setRevisionHeaders(const ResponsePtr& res, std::uint64_t revision) {
        const std::string revisionString = std::to_string(revision);
        res->set("ETag", "\"rev-" + revisionString + "\"");
        res->set("X-Mapping-Revision", revisionString);
    }

    template <typename ResponsePtr>
    void setLegacyRouteWarningHeaders(const ResponsePtr& res) {
        res->set("Deprecation", "true");
        res->set("Warning", "299 - Legacy /config draft mutation routes are deprecated; prefer /drafts/* endpoints");
    }

    std::optional<int64_t> parseExpectedDraftRevision(const nlohmann::json& body) {
        if (body.is_object() && body.contains("expected_draft_revision") && body["expected_draft_revision"].is_number_integer()) {
            return body["expected_draft_revision"].get<int64_t>();
        }

        return std::nullopt;
    }

    std::optional<int64_t> extractDraftRevisionFromEnvelope(const nlohmann::json& draftEnvelope) {
        if (draftEnvelope.is_object() && draftEnvelope.contains("meta") && draftEnvelope["meta"].is_object() &&
            draftEnvelope["meta"].contains("draft_revision") && draftEnvelope["meta"]["draft_revision"].is_number_integer()) {
            return draftEnvelope["meta"]["draft_revision"].get<int64_t>();
        }

        if (draftEnvelope.is_object() && draftEnvelope.contains("draft_revision") && draftEnvelope["draft_revision"].is_number_integer()) {
            return draftEnvelope["draft_revision"].get<int64_t>();
        }

        return std::nullopt;
    }

    std::optional<int64_t> readCurrentDraftRevisionSafe(const std::string& mapFilePath, const std::string& draftId) {
        try {
            const nlohmann::json draftEnvelope = mqtt::lib::JsonMappingReader::readDraft(mapFilePath, draftId);
            return extractDraftRevisionFromEnvelope(draftEnvelope);
        } catch (...) {
            return std::nullopt;
        }
    }

    nlohmann::json makeDraftConflictResponse(const std::string& mapFilePath, const std::string& draftId, const std::string& details) {
        nlohmann::json conflictResponse = {{"error", "Draft modified concurrently"}, {"details", details}, {"draft_id", draftId}};

        const std::optional<int64_t> currentDraftRevision = readCurrentDraftRevisionSafe(mapFilePath, draftId);
        if (currentDraftRevision.has_value()) {
            conflictResponse["current_draft_revision"] = currentDraftRevision.value();
        }

        return conflictResponse;
    }

    template <typename RequestPtr>
    nlohmann::json parseJsonBodyOrEmpty(const RequestPtr& req) {
        if (req->body.empty()) {
            return nlohmann::json::object();
        }

        return nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
    }

    template <typename MutationFn>
    void applyDraftMutationWithAutoCreate(const std::string& mapFilePath, const std::string& draftId, MutationFn&& mutation) {
        try {
            std::forward<MutationFn>(mutation)();
        } catch (const mqtt::lib::EntityNotFoundError&) {
            mqtt::lib::JsonMappingReader::createDraftFromActive(mapFilePath, draftId);
            std::forward<MutationFn>(mutation)();
        }
    }

    template <typename RequestPtr, typename ResponsePtr>
    void handleDeployRequest(const RequestPtr& req,
                             const ResponsePtr& res,
                             mqtt::lib::ConfigApplication* configApplication,
                             const std::shared_ptr<AdminMappingContext>& mappingContext,
                             const mqtt::lib::admin::ReloadCallback& onDeploy) {
        try {
            const nlohmann::json body = parseJsonBodyOrEmpty(req);
            const std::string draftId = resolveDraftId(req, &body);
            const std::optional<std::uint64_t> expectedRevision = resolveExpectedRevision(req, &body);

            const nlohmann::json newMappingJson = mqtt::lib::JsonMappingReader::deployDraft(
                mappingContext->mappingFilePath,
                draftId,
                mappingContext->enableVersioning,
                expectedRevision);

            const bool mustReconnect = configApplication->setMapping(newMappingJson);

            mqtt::lib::admin::ReloadResult reloadResult;
            if (onDeploy) {
                reloadResult = onDeploy(mustReconnect);
            }

            const std::uint64_t revision = mqtt::lib::JsonMappingReader::readActiveRevision(mappingContext->mappingFilePath);
            setRevisionHeaders(res, revision);

            res->status(200).json({{"status", "deploy-ack"},
                                   {"draft_id", draftId},
                                   {"revision", revision},
                                   {"reload_mode", reloadResult.mode},
                                   {"instances", reloadResult.instances},
                                   {"subscribed", reloadResult.subscribed},
                                   {"unsubscribed", reloadResult.unsubscribed}});
        } catch (const nlohmann::json::parse_error& e) {
            res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
        } catch (const mqtt::lib::EntityNotFoundError& e) {
            res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
        } catch (const mqtt::lib::OCCConflictError& e) {
            nlohmann::json conflictResponse = {{"error", "Revision conflict"}, {"details", e.what()}};
            try {
                conflictResponse["current_revision"] = mqtt::lib::JsonMappingReader::readActiveRevision(mappingContext->mappingFilePath);
            } catch (...) {
            }
            res->status(412).json(conflictResponse);
        } catch (const std::exception& e) {
            res->status(500).json({{"error", "Deploy failed"}, {"details", e.what()}});
        }
    }

    bool hasConfiguredMappingFilePath(mqtt::lib::ConfigApplication* configApplication) {
        const CLI::Option* option = configApplication->getOption("--mqtt-mapping-file");
        return option != nullptr && option->count() > 0;
    }

    nlohmann::json getCurrentMappingOrThrow(mqtt::lib::ConfigApplication* configApplication, const std::string& configuredMappingFilePath) {
        const nlohmann::json& currentMapping = configApplication->getMqttMapper()->getMapping();
        if (currentMapping.is_object() && !currentMapping.empty()) {
            return currentMapping;
        }

        if (configuredMappingFilePath.empty()) {
            throw std::runtime_error(
                "No mapping file configured and no inline mapping available. Provide --mqtt-mapping-file or set the mapping JSON at startup.");
        }

        throw std::runtime_error("Configured mapping file does not exist and no inline mapping is available: " + configuredMappingFilePath);
    }

    std::shared_ptr<AdminMappingContext> buildAdminMappingContext(mqtt::lib::ConfigApplication* configApplication) {
        auto context = std::make_shared<AdminMappingContext>();

        if (hasConfiguredMappingFilePath(configApplication)) {
            const std::string configuredMappingFilePath = configApplication->getMappingFile();
            if (configuredMappingFilePath.empty()) {
                throw std::runtime_error("Configured mapping file path is empty.");
            }

            std::error_code ec;
            if (!fs::exists(configuredMappingFilePath, ec)) {
                if (ec) {
                    throw std::runtime_error("Failed to check mapping file path '" + configuredMappingFilePath + "': " + ec.message());
                }

                throw std::runtime_error("Configured mapping file does not exist: " + configuredMappingFilePath);
            }

            context->mappingFilePath = configuredMappingFilePath;
            context->enableVersioning = true;
            return context;
        }

        context->runtimeMappingWorkspace = std::make_unique<RuntimeMappingWorkspace>(getCurrentMappingOrThrow(configApplication, ""));
        context->mappingFilePath = context->runtimeMappingWorkspace->getMappingFilePath();
        context->enableVersioning = false;
        return context;
    }

} // namespace

namespace mqtt::lib::admin {

    express::Router makeMappingAdminRouter(ConfigApplication* configApplication, const AdminOptions& opt, ReloadCallback onDeploy) {
        express::Router api;
        const std::shared_ptr<AdminMappingContext> mappingContext = buildAdminMappingContext(configApplication);

        api.use(express::middleware::JsonMiddleware());
        api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

        // GET /schema
        api.get("/schema", [] APPLICATION(req, res) {
            res->status(200).send(MqttMapper::getSchema());
        });

        // GET /config (active mapping)
        api.get("/config", [mappingContext] APPLICATION(req, res) {
            try {
                const nlohmann::json active = JsonMappingReader::readActive(mappingContext->mappingFilePath);
                const std::uint64_t revision = JsonMappingReader::readActiveRevision(mappingContext->mappingFilePath);
                setRevisionHeaders(res, revision);

                res->status(200).json({{"revision", revision}, {"mapping", active}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to load configuration"}, {"details", e.what()}});
            }
        });

        // POST /drafts/create
        api.post("/drafts/create", [mappingContext] APPLICATION(req, res) {
            try {
                nlohmann::json body = nlohmann::json::object();
                if (!req->body.empty()) {
                    body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                }

                const std::string draftId = JsonMappingReader::createDraftFromActive(
                    mappingContext->mappingFilePath,
                    (body.is_object() && body.contains("draft_id") && body["draft_id"].is_string()) ? body["draft_id"].get<std::string>() : "");

                const nlohmann::json draft = JsonMappingReader::readDraft(mappingContext->mappingFilePath, draftId);
                res->status(201).json(draft);
            } catch (const OCCConflictError& e) {
                res->status(409).json({{"error", "Draft already exists"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Draft creation failed"}, {"details", e.what()}});
            }
        });

        // GET /drafts/list
        api.get("/drafts/list", [mappingContext] APPLICATION(req, res) {
            try {
                res->status(200).json(JsonMappingReader::listDrafts(mappingContext->mappingFilePath));
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to list drafts"}, {"details", e.what()}});
            }
        });

        // POST /drafts/get
        api.post("/drafts/get", [mappingContext] APPLICATION(req, res) {
            try {
                const nlohmann::json body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                const std::string draftId = resolveDraftId(req, &body);
                res->status(200).json(JsonMappingReader::readDraft(mappingContext->mappingFilePath, draftId));
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Failed to load draft"}, {"details", e.what()}});
            }
        });

        // PATCH /drafts/patch
        api.patch("/drafts/patch", [mappingContext] APPLICATION(req, res) {
            std::string draftId = "default";
            try {
                const nlohmann::json body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                if (!body.is_object() || !body.contains("patch")) {
                    res->status(400).json({{"error", "Body must be an object containing patch"}});
                    return;
                }

                const std::optional<int64_t> expectedDraftRevision = parseExpectedDraftRevision(body);
                if (!expectedDraftRevision.has_value()) {
                    res->status(428).json({{"error", "Missing expected_draft_revision"},
                                           {"details", "PATCH /drafts/patch requires expected_draft_revision to prevent lost updates"}});
                    return;
                }

                draftId = resolveDraftId(req, &body);
                const nlohmann::json draft = JsonMappingReader::patchDraft(mappingContext->mappingFilePath, draftId, body["patch"], expectedDraftRevision);
                res->status(200).json(draft);
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
            } catch (const OCCConflictError& e) {
                res->status(412).json(makeDraftConflictResponse(mappingContext->mappingFilePath, draftId, e.what()));
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Draft patch failed"}, {"details", e.what()}});
            }
        });

        // POST /drafts/replace
        api.post("/drafts/replace", [mappingContext] APPLICATION(req, res) {
            std::string draftId = "default";
            try {
                const nlohmann::json body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                if (!body.is_object() || !body.contains("mapping") || !body["mapping"].is_object()) {
                    res->status(422).json({{"error", "Body must contain mapping object"}});
                    return;
                }

                const std::optional<int64_t> expectedDraftRevision = parseExpectedDraftRevision(body);
                if (!expectedDraftRevision.has_value()) {
                    res->status(428).json({{"error", "Missing expected_draft_revision"},
                                           {"details", "POST /drafts/replace requires expected_draft_revision to prevent lost updates"}});
                    return;
                }

                draftId = resolveDraftId(req, &body);
                const nlohmann::json draft = JsonMappingReader::replaceDraft(mappingContext->mappingFilePath, draftId, body["mapping"], expectedDraftRevision);
                res->status(200).json(draft);
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
            } catch (const OCCConflictError& e) {
                res->status(412).json(makeDraftConflictResponse(mappingContext->mappingFilePath, draftId, e.what()));
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Draft replacement failed"}, {"details", e.what()}});
            }
        });

        // POST /drafts/validate
        api.post("/drafts/validate", [mappingContext] APPLICATION(req, res) {
            try {
                const nlohmann::json body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                const std::string draftId = resolveDraftId(req, &body);

                const nlohmann::json draft = JsonMappingReader::readDraft(mappingContext->mappingFilePath, draftId);
                nlohmann::json_schema::basic_error_handler err;
                MqttMapper::validate(draft.at("mapping"), err);

                if (err) {
                    res->status(422).json({{"valid", false}, {"error", "Draft validation failed"}, {"draft_id", draftId}});
                } else {
                    res->status(200).json({{"valid", true}, {"draft_id", draftId}});
                }
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"valid", false}, {"error", "Draft not found"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"valid", false}, {"error", "Draft validation exception"}, {"details", e.what()}});
            }
        });

        // POST /drafts/deploy
        api.post("/drafts/deploy", [configApplication, mappingContext, onDeploy] APPLICATION(req, res) {
            handleDeployRequest(req, res, configApplication, mappingContext, onDeploy);
        });

        // POST /drafts/delete
        api.post("/drafts/delete", [mappingContext] APPLICATION(req, res) {
            try {
                const nlohmann::json body = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
                const std::string draftId = resolveDraftId(req, &body);
                JsonMappingReader::discardDraft(mappingContext->mappingFilePath, draftId);
                res->status(200).json({{"status", "deleted"}, {"draft_id", draftId}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Draft delete failed"}, {"details", e.what()}});
            }
        });

        // PATCH /config (legacy default draft)
        api.patch("/config", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                const nlohmann::json patchOps = nlohmann::json::parse(bodyStr);

                const std::string draftId = "default";
                setLegacyRouteWarningHeaders(res);

                applyDraftMutationWithAutoCreate(mappingContext->mappingFilePath, draftId, [&]() {
                    JsonMappingReader::patchDraft(mappingContext->mappingFilePath, draftId, patchOps);
                });

                res->status(200).json({{"status", "patched"}, {"draft_id", draftId}, {"path", mappingContext->mappingFilePath}});
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Patch application failed"}, {"details", e.what()}});
            }
        });

        // POST /config (legacy replace full draft)
        api.post("/config", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                nlohmann::json replacement = nlohmann::json::parse(bodyStr);

                if (!replacement.is_object()) {
                    res->status(422).json({{"error", "Config replacement must be a JSON object"}});
                    return;
                }

                const std::string draftId = "default";
                setLegacyRouteWarningHeaders(res);

                applyDraftMutationWithAutoCreate(mappingContext->mappingFilePath, draftId, [&]() {
                    JsonMappingReader::replaceDraft(mappingContext->mappingFilePath, draftId, replacement);
                });

                res->status(200).json({{"status", "replaced"}, {"draft_id", draftId}, {"path", mappingContext->mappingFilePath}});
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Config replacement failed"}, {"details", e.what()}});
            }
        });

        // POST /config/deploy (legacy wrapper)
        api.post("/config/deploy", [configApplication, mappingContext, onDeploy] APPLICATION(req, res) {
            handleDeployRequest(req, res, configApplication, mappingContext, onDeploy);
        });

        // POST /config/validate
        api.post("/config/validate", [] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                auto document = nlohmann::json::parse(bodyStr);

                nlohmann::json_schema::basic_error_handler err;
                MqttMapper::validate(document, err);

                if (err) {
                    res->status(422).json({{"valid", false}, {"error", "Validation failed"}});
                } else {
                    res->status(200).json({{"valid", true}});
                }
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Validation exception"}, {"details", e.what()}});
            }
        });

        // GET /config/validateDraft (legacy wrapper)
        api.get("/config/validateDraft", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string draftId = "default";
                setLegacyRouteWarningHeaders(res);
                const nlohmann::json draft = JsonMappingReader::readDraft(mappingContext->mappingFilePath, draftId);

                nlohmann::json_schema::basic_error_handler err;
                MqttMapper::validate(draft.at("mapping"), err);

                if (err) {
                    res->status(422).json({{"valid", false}, {"error", "Draft validation failed"}, {"draft_id", draftId}});
                } else {
                    res->status(200).json({{"valid", true}, {"draft_id", draftId}});
                }
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"valid", false}, {"error", "No draft configuration available"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"valid", false}, {"error", "Draft validation exception"}, {"details", e.what()}});
            }
        });

        // POST /config/rollback
        api.post("/config/rollback", [configApplication, mappingContext, onDeploy] APPLICATION(req, res) {
            try {
                if (!mappingContext->enableVersioning) {
                    res->status(409).json({{"error", "Rollback is disabled in ephemeral runtime mode"}});
                    return;
                }

                const std::string bodyStr(req->body.begin(), req->body.end());
                auto jsonBody = nlohmann::json::parse(bodyStr);

                if (!jsonBody.contains("version_id")) {
                    res->status(400).json({{"error", "Missing version_id"}});
                    return;
                }

                std::string versionId = jsonBody["version_id"];
                const std::optional<std::uint64_t> expectedRevision = resolveExpectedRevision(req, &jsonBody);

                nlohmann::json rolledbackMappingJson = JsonMappingReader::rollbackTo(
                    mappingContext->mappingFilePath,
                    versionId,
                    mappingContext->enableVersioning,
                    expectedRevision);

                bool mustReconnect = configApplication->setMapping(rolledbackMappingJson); // throws in case of an error during loading
                                                                                           // or validation. This exeption is catched
                                                                                           // in the MappingAdminRouter
                ReloadResult reloadResult;
                if (onDeploy) {
                    reloadResult = onDeploy(mustReconnect); // Trigger hot-reload
                }

                const std::uint64_t revision = JsonMappingReader::readActiveRevision(mappingContext->mappingFilePath);
                setRevisionHeaders(res, revision);

                res->status(200).json({{"status", "deploy-ack"},
                                       {"revision", revision},
                                       {"reload_mode", reloadResult.mode},
                                       {"instances", reloadResult.instances},
                                       {"subscribed", reloadResult.subscribed},
                                       {"unsubscribed", reloadResult.unsubscribed}});
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const OCCConflictError& e) {
                nlohmann::json conflictResponse = {{"error", "Revision conflict"}, {"details", e.what()}};
                try {
                    conflictResponse["current_revision"] = JsonMappingReader::readActiveRevision(mappingContext->mappingFilePath);
                } catch (...) {
                }
                res->status(412).json(conflictResponse);
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Version not found"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Rollback failed"}, {"details", e.what()}});
            }
        });

        // GET /config/history
        api.get("/config/history", [mappingContext] APPLICATION(req, res) {
            try {
                if (!mappingContext->enableVersioning) {
                    res->status(200).json(nlohmann::json::array());
                    return;
                }

                auto history = JsonMappingReader::getHistory(mappingContext->mappingFilePath);
                nlohmann::json list = nlohmann::json::array();
                for (const auto& h : history) {
                    list.push_back({{"id", h.id}, {"comment", h.comment}, {"date", h.date}});
                }
                res->status(200).json(list);
            } catch ([[maybe_unused]] const std::exception& e) {
                res->status(500).json({{"error", "Failed to fetch history"}});
            }
        });

        api.get("/", [] APPLICATION(req, res) {
            res->redirect("/ui");
        });

        api.get("/ui", [] APPLICATION(req, res) {
            res->redirect("/ui/index.html");
        });

        api.use("/ui",
                express::middleware::StaticMiddleware("/home/voc/tmp/integrator/mqtt-integrator-ui/dist/mqtt-integrator-ui/browser"));

        api.get("*", [] APPLICATION(req, res) {
            res->redirect("/ui/index.html");
        });

        return api;
    }

} // namespace mqtt::lib::admin

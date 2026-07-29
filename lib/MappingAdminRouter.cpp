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

#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <optional>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace {

    constexpr const char* DEFAULT_DRAFT_ID = "default";

    struct MappingApplyResult {
        std::uint64_t revision{0};
        std::string draftId;
        bool mappingPersisted{false};
        mqtt::lib::admin::ReloadResult reloadResult;
    };

    std::optional<std::uint64_t> parseRevisionToken(std::string value) {
        if (value.empty()) {
            return std::nullopt;
        }

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
            std::size_t pos = 0;
            const auto parsed = std::stoull(value, &pos);
            return pos == value.size() ? std::optional<std::uint64_t>(static_cast<std::uint64_t>(parsed)) : std::nullopt;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    template <typename RequestPtr>
    std::optional<std::uint64_t> resolveExpectedRevision(const RequestPtr& req, const nlohmann::json* body) {
        if (body != nullptr) {
            try {
                return body->at("expected_revision").get<std::uint64_t>();
            } catch (const nlohmann::json::exception&) {
            }
        }

        return parseRevisionToken(req->get("If-Match"));
    }

    template <typename RequestPtr>
    std::string resolveDraftId(const RequestPtr& req, const nlohmann::json* body) {
        if (body != nullptr) {
            try {
                return body->at("draft_id").get<std::string>();
            } catch (const nlohmann::json::exception&) {
            }
        }

        const std::string headerDraftId = req->get("X-Draft-Id");
        return headerDraftId.empty() ? DEFAULT_DRAFT_ID : headerDraftId;
    }

    template <typename ResponsePtr>
    void setRevisionHeaders(const ResponsePtr& res, std::uint64_t revision) {
        const std::string revisionString = std::to_string(revision);
        res->set("ETag", "\"rev-" + revisionString + "\"");
        res->set("X-Mapping-Revision", revisionString);
    }

    std::optional<std::string> resolveSnapshotId(const nlohmann::json& body) {
        try {
            return body.at("snapshot_id").get<std::string>();
        } catch (const nlohmann::json::exception&) {
            return std::nullopt;
        }
    }

    nlohmann::json makeDraftConflictResponse(const std::string& adminStorageRoot,
                                             const std::string& draftId,
                                             const std::string& details) {
        nlohmann::json conflictResponse = {{"error", "Draft modified concurrently"}, {"details", details}, {"draft_id", draftId}};
        try {
            if (const std::optional<int64_t> currentDraftRevision =
                    mqtt::lib::JsonMappingReader::readDraftRevision(adminStorageRoot, draftId);
                currentDraftRevision) {
                conflictResponse["current_draft_revision"] = *currentDraftRevision;
            }
        } catch (const std::exception&) {
        }
        return conflictResponse;
    }

    nlohmann::json makeDeployAckResponse(const MappingApplyResult& applyResult) {
        nlohmann::json response = {{"status", applyResult.mappingPersisted ? "persist-ack" : "deploy-ack"},
                                   {"revision", applyResult.revision},
                                   {"reload_mode", applyResult.reloadResult.mode},
                                   {"instances", applyResult.reloadResult.instances},
                                   {"subscribed", applyResult.reloadResult.subscribed},
                                   {"unsubscribed", applyResult.reloadResult.unsubscribed}};
        if (!applyResult.draftId.empty()) {
            response["draft_id"] = applyResult.draftId;
        }

        return response;
    }

    template <typename ResponsePtr>
    bool requireExpectedDraftRevision(const nlohmann::json& body,
                                      const std::string& endpoint,
                                      const ResponsePtr& res,
                                      std::optional<int64_t>& outRevision) {
        try {
            outRevision = body.at("expected_draft_revision").get<int64_t>();
        } catch (const nlohmann::json::exception&) {
            outRevision = std::nullopt;
        }

        if (outRevision) {
            return true;
        }

        res->status(428).json({{"error", "Missing expected_draft_revision"},
                               {"details", endpoint + " requires expected_draft_revision to prevent lost updates"}});
        return false;
    }

    template <typename ResponsePtr>
    void respondRevisionConflict(const ResponsePtr& res,
                                 const std::string& details,
                                 const std::function<std::optional<std::uint64_t>()>& currentRevisionProvider) {
        nlohmann::json conflictResponse = {{"error", "Revision conflict"}, {"details", details}};
        try {
            const std::optional<std::uint64_t> currentRevision = currentRevisionProvider();
            if (currentRevision) {
                conflictResponse["current_revision"] = *currentRevision;
            }
        } catch (const std::exception&) {
        }

        res->status(412).json(conflictResponse);
    }

    template <typename RequestPtr>
    nlohmann::json parseJsonBody(const RequestPtr& req, bool allowEmpty = false) {
        if (allowEmpty && req->body.empty()) {
            return nlohmann::json::object();
        }

        return nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));
    }

    MappingApplyResult toMappingApplyResult(const mqtt::lib::JsonMappingReader::ApplyResult& applyResult,
                                            const mqtt::lib::admin::ReloadCallback& onDeploy) {
        mqtt::lib::admin::ReloadResult reloadResult;
        if (onDeploy) {
            reloadResult = onDeploy(applyResult.mustReconnect);
        }

        return MappingApplyResult{applyResult.revision, applyResult.draftId, applyResult.mappingPersisted, reloadResult};
    }

    template <typename RequestPtr, typename ResponsePtr, typename DraftPayloadFn, typename MutateDraftFn>
    void handleDraftMutationRequest(const RequestPtr& req,
                                    const ResponsePtr& res,
                                    const std::string& adminStorageRoot,
                                    const std::string& endpoint,
                                    const std::string& failureError,
                                    DraftPayloadFn&& draftPayload,
                                    MutateDraftFn&& mutateDraft) {
        std::string draftId = DEFAULT_DRAFT_ID;
        try {
            const nlohmann::json body = parseJsonBody(req);
            std::optional<int64_t> expectedDraftRevision;
            if (!requireExpectedDraftRevision(body, endpoint, res, expectedDraftRevision)) {
                return;
            }

            draftId = resolveDraftId(req, &body);
            const nlohmann::json draft = mutateDraft(draftId, draftPayload(body), expectedDraftRevision);
            res->status(200).json(draft);
        } catch (const nlohmann::json::parse_error& e) {
            res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
        } catch (const mqtt::lib::EntityNotFoundError& e) {
            res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
        } catch (const mqtt::lib::OCCConflictError& e) {
            res->status(412).json(makeDraftConflictResponse(adminStorageRoot, draftId, e.what()));
        } catch (const std::exception& e) {
            res->status(422).json({{"error", failureError}, {"details", e.what()}});
        }
    }

    template <typename RequestPtr, typename ResponsePtr, typename LegacyMutationFn>
    void handleLegacyDefaultDraftMutation(const RequestPtr& req,
                                          const ResponsePtr& res,
                                          const std::string& successStatus,
                                          const std::string& failureError,
                                          LegacyMutationFn&& mutation) {
        try {
            const nlohmann::json payload = parseJsonBody(req);
            const std::string draftId = DEFAULT_DRAFT_ID;
            std::forward<LegacyMutationFn>(mutation)(payload, draftId);
            res->status(200).json({{"status", successStatus}, {"draft_id", draftId}});
        } catch (const nlohmann::json::parse_error& e) {
            res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
        } catch (const std::exception& e) {
            res->status(422).json({{"error", failureError}, {"details", e.what()}});
        }
    }

    template <typename ResponsePtr>
    void respondDraftValidationResult(const ResponsePtr& res, const std::string& draftId, bool isValid) {
        if (!isValid) {
            res->status(422).json({{"valid", false}, {"error", "Draft validation failed"}, {"draft_id", draftId}});
            return;
        }

        res->status(200).json({{"valid", true}, {"draft_id", draftId}});
    }

    template <typename ResponsePtr, typename ResolveDraftIdFn>
    void handleDraftValidationRequest(const ResponsePtr& res,
                                      const std::string& adminStorageRoot,
                                      const std::string& notFoundError,
                                      ResolveDraftIdFn&& resolveDraftIdFn) {
        try {
            const std::string draftId = std::forward<ResolveDraftIdFn>(resolveDraftIdFn)();
            respondDraftValidationResult(res, draftId, mqtt::lib::JsonMappingReader::isDraftValid(adminStorageRoot, draftId));
        } catch (const mqtt::lib::EntityNotFoundError& e) {
            res->status(404).json({{"valid", false}, {"error", notFoundError}, {"details", e.what()}});
        } catch (const std::exception& e) {
            res->status(400).json({{"valid", false}, {"error", "Draft validation exception"}, {"details", e.what()}});
        }
    }

    template <typename RequestPtr, typename ResponsePtr>
    void handleDeployRequest(const RequestPtr& req,
                             const ResponsePtr& res,
                             mqtt::lib::ConfigApplication* configApplication,
                             const std::string& adminStorageRoot,
                             const mqtt::lib::admin::ReloadCallback& onDeploy) {
        try {
            const nlohmann::json body = parseJsonBody(req, true);
            const std::string draftId = resolveDraftId(req, &body);
            const std::optional<std::uint64_t> expectedRevision = resolveExpectedRevision(req, &body);

            const mqtt::lib::JsonMappingReader::ApplyResult applyResult =
                mqtt::lib::JsonMappingReader::deployAndApplyDraft(adminStorageRoot, configApplication, draftId, expectedRevision);

            const MappingApplyResult responseResult = toMappingApplyResult(applyResult, onDeploy);
            setRevisionHeaders(res, responseResult.revision);
            res->status(200).json(makeDeployAckResponse(responseResult));
        } catch (const nlohmann::json::parse_error& e) {
            res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
        } catch (const mqtt::lib::EntityNotFoundError& e) {
            res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
        } catch (const mqtt::lib::OCCConflictError& e) {
            respondRevisionConflict(res, e.what(), [&]() -> std::optional<std::uint64_t> {
                return configApplication->getMqttMapper()->getRevision();
            });
        } catch (const std::invalid_argument& e) {
            res->status(422).json({{"error", "Deploy validation failed"}, {"details", e.what()}});
        } catch (const std::exception& e) {
            res->status(500).json({{"error", "Deploy failed"}, {"details", e.what()}});
        }
    }

    std::string buildAdminStorageRoot(const mqtt::lib::ConfigApplication* configApplication) {
        namespace fs = std::filesystem;
        const fs::path adminRoot = fs::temp_directory_path() / "mqttsuite" / "admin" / configApplication->getName();
        fs::create_directories(adminRoot);
        return adminRoot.string();
    }

} // namespace

namespace mqtt::lib::admin {

    express::Router makeMappingAdminRouter(ConfigApplication* configApplication, const AdminOptions& opt, ReloadCallback onDeploy) {
        express::Router api;
        const std::string adminStorageRoot = buildAdminStorageRoot(configApplication);

        api.use(express::middleware::JsonMiddleware());
        api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

        // GET /schema
        api.get("/schema", [] APPLICATION(req, res) {
            res->status(200).send(MqttMapper::getSchema());
        });

        // GET /config (active mapping)
        api.get("/config", [configApplication] APPLICATION(req, res) {
            try {
                const nlohmann::json active = configApplication->getMqttMapper()->getMapping();
                const std::uint64_t revision = configApplication->getMqttMapper()->getRevision();
                setRevisionHeaders(res, revision);

                res->status(200).json(active);
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to load configuration"}, {"details", e.what()}});
            }
        });

        // POST /drafts/create
        api.post("/drafts/create", [configApplication, adminStorageRoot] APPLICATION(req, res) {
            try {
                nlohmann::json body = parseJsonBody(req, true);
                const nlohmann::json draft = JsonMappingReader::createDraftFromActive(
                    adminStorageRoot,
                    configApplication,
                    body.contains("draft_id") ? body.at("draft_id").get<std::string>() : "");
                res->status(201).json(draft);
            } catch (const OCCConflictError& e) {
                res->status(409).json({{"error", "Draft already exists"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Draft creation failed"}, {"details", e.what()}});
            }
        });

        // GET /drafts/list
        api.get("/drafts/list", [adminStorageRoot] APPLICATION(req, res) {
            try {
                res->status(200).json(JsonMappingReader::listDrafts(adminStorageRoot));
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to list drafts"}, {"details", e.what()}});
            }
        });

        // POST /drafts/get
        api.post("/drafts/get", [adminStorageRoot] APPLICATION(req, res) {
            try {
                const nlohmann::json body = parseJsonBody(req, true);
                const std::string draftId = resolveDraftId(req, &body);
                res->status(200).json(JsonMappingReader::readDraft(adminStorageRoot, draftId));
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Draft not found"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Failed to load draft"}, {"details", e.what()}});
            }
        });

        // PATCH /drafts/patch
        api.patch("/drafts/patch", [adminStorageRoot] APPLICATION(req, res) {
            handleDraftMutationRequest(
                req,
                res,
                adminStorageRoot,
                "PATCH /drafts/patch",
                "Draft patch failed",
                [](const nlohmann::json& body) -> const nlohmann::json& {
                    return body.at("patch");
                },
                [adminStorageRoot](const std::string& draftId,
                                   const nlohmann::json& patchOps,
                                   std::optional<int64_t> expectedDraftRevision) {
                    return JsonMappingReader::patchDraft(adminStorageRoot, draftId, patchOps, expectedDraftRevision);
                });
        });

        // POST /drafts/replace
        api.post("/drafts/replace", [adminStorageRoot] APPLICATION(req, res) {
            handleDraftMutationRequest(
                req,
                res,
                adminStorageRoot,
                "POST /drafts/replace",
                "Draft replacement failed",
                [](const nlohmann::json& body) -> const nlohmann::json& {
                    return body.at("mapping");
                },
                [adminStorageRoot](const std::string& draftId,
                                   const nlohmann::json& mapping,
                                   std::optional<int64_t> expectedDraftRevision) {
                    return JsonMappingReader::replaceDraft(adminStorageRoot, draftId, mapping, expectedDraftRevision);
                });
        });

        // POST /drafts/validate
        api.post("/drafts/validate", [adminStorageRoot] APPLICATION(req, res) {
            handleDraftValidationRequest(res, adminStorageRoot, "Draft not found", [&]() {
                const nlohmann::json body = parseJsonBody(req, true);
                return resolveDraftId(req, &body);
            });
        });

        // POST /drafts/deploy
        api.post("/drafts/deploy", [configApplication, adminStorageRoot, onDeploy] APPLICATION(req, res) {
            handleDeployRequest(req, res, configApplication, adminStorageRoot, onDeploy);
        });

        // POST /drafts/delete
        api.post("/drafts/delete", [adminStorageRoot] APPLICATION(req, res) {
            try {
                const nlohmann::json body = parseJsonBody(req, true);
                const std::string draftId = resolveDraftId(req, &body);
                JsonMappingReader::discardDraft(adminStorageRoot, draftId);
                res->status(200).json({{"status", "deleted"}, {"draft_id", draftId}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Draft delete failed"}, {"details", e.what()}});
            }
        });

        // PATCH /config (legacy default draft)
        api.patch("/config", [configApplication, adminStorageRoot] APPLICATION(req, res) {
            handleLegacyDefaultDraftMutation(
                req,
                res,
                "patched",
                "Patch application failed",
                [adminStorageRoot, configApplication](const nlohmann::json& payload, const std::string& draftId) {
                    JsonMappingReader::patchDraftWithAutoCreate(adminStorageRoot, configApplication, draftId, payload, std::nullopt);
                });
        });

        // POST /config (legacy replace full draft)
        api.post("/config", [configApplication, adminStorageRoot] APPLICATION(req, res) {
            handleLegacyDefaultDraftMutation(
                req,
                res,
                "replaced",
                "Config replacement failed",
                [adminStorageRoot, configApplication](const nlohmann::json& payload, const std::string& draftId) {
                    JsonMappingReader::replaceDraftWithAutoCreate(adminStorageRoot, configApplication, draftId, payload, std::nullopt);
                });
        });

        // POST /config/deploy (legacy wrapper)
        api.post("/config/deploy", [configApplication, adminStorageRoot, onDeploy] APPLICATION(req, res) {
            handleDeployRequest(req, res, configApplication, adminStorageRoot, onDeploy);
        });

        // POST /config/validate
        api.post("/config/validate", [] APPLICATION(req, res) {
            try {
                auto document = parseJsonBody(req);
                const bool valid = JsonMappingReader::isMappingValid(document);
                res->status(valid ? 200 : 422)
                    .json(valid ? nlohmann::json{{"valid", true}} : nlohmann::json{{"valid", false}, {"error", "Validation failed"}});
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Validation exception"}, {"details", e.what()}});
            }
        });

        // GET /config/validateDraft (legacy wrapper)
        api.get("/config/validateDraft", [adminStorageRoot] APPLICATION(req, res) {
            handleDraftValidationRequest(res, adminStorageRoot, "No draft configuration available", []() {
                return std::string(DEFAULT_DRAFT_ID);
            });
        });

        // POST /config/rollback
        api.post("/config/rollback", [configApplication, adminStorageRoot, onDeploy] APPLICATION(req, res) {
            try {
                auto jsonBody = parseJsonBody(req);

                const std::optional<std::string> snapshotId = resolveSnapshotId(jsonBody);
                if (!snapshotId) {
                    res->status(400).json({{"error", "Missing snapshot_id"}, {"details", "Provide snapshot_id"}});
                    return;
                }
                const std::optional<std::uint64_t> expectedRevision = resolveExpectedRevision(req, &jsonBody);

                const mqtt::lib::JsonMappingReader::ApplyResult applyResult =
                    mqtt::lib::JsonMappingReader::rollbackAndApplyVersion(
                        adminStorageRoot, configApplication, *snapshotId, expectedRevision);

                const MappingApplyResult responseResult = toMappingApplyResult(applyResult, onDeploy);
                setRevisionHeaders(res, responseResult.revision);
                res->status(200).json(makeDeployAckResponse(responseResult));
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const OCCConflictError& e) {
                respondRevisionConflict(res, e.what(), [&]() -> std::optional<std::uint64_t> {
                    return configApplication->getMqttMapper()->getRevision();
                });
            } catch (const EntityNotFoundError& e) {
                res->status(404).json({{"error", "Snapshot not found"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Rollback failed"}, {"details", e.what()}});
            }
        });

        // GET /config/history
        api.get("/config/history", [adminStorageRoot] APPLICATION(req, res) {
            try {
                auto history = JsonMappingReader::getHistory(adminStorageRoot);
                nlohmann::json list = nlohmann::json::array();
                for (const auto& h : history) {
                    list.push_back({{"snapshot_id", h.snapshotId}, {"comment", h.comment}, {"date", h.date}});
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

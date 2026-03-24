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
#include <memory>
#include <stdlib.h>
#include <system_error>
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

        // GET /config
        api.get("/config", [configApplication] APPLICATION(req, res) {
            try {
                res->status(200).json(configApplication->getMqttMapper()->getMapping());
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to load configuration"}, {"details", e.what()}});
            }
        });

        // PATCH /config
        api.patch("/config", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                nlohmann::json patchOps = nlohmann::json::parse(bodyStr);

                nlohmann::json current = JsonMappingReader::readDraftOrActive(mappingContext->mappingFilePath);
                current = current.patch(patchOps);

                JsonMappingReader::saveDraft(mappingContext->mappingFilePath, current);

                res->status(200).json({{"status", "patched"}, {"path", mappingContext->mappingFilePath}});
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Patch application failed"}, {"details", e.what()}});
            }
        });

        // POST /config (replace full draft config)
        api.post("/config", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                nlohmann::json replacement = nlohmann::json::parse(bodyStr);

                if (!replacement.is_object()) {
                    res->status(422).json({{"error", "Config replacement must be a JSON object"}});
                    return;
                }

                JsonMappingReader::saveDraft(mappingContext->mappingFilePath, replacement);

                res->status(200).json({{"status", "replaced"}, {"path", mappingContext->mappingFilePath}});
            } catch (const nlohmann::json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Config replacement failed"}, {"details", e.what()}});
            }
        });

        // POST /config/deploy
        api.post("/config/deploy", [configApplication, mappingContext, onDeploy] APPLICATION(req, res) {
            try {
                nlohmann::json newMappingJson = JsonMappingReader::deployDraft(mappingContext->mappingFilePath, mappingContext->enableVersioning);

                if (newMappingJson.is_null()) {
                    res->status(404).json(
                        {{"error", "No draft configuration available"}, {"path", JsonMappingReader::getDraftPath(mappingContext->mappingFilePath)}});
                    return;
                }

                bool mustReconnect = configApplication->setMapping(newMappingJson); // throws in case of an error during loading
                                                                                    // or validation. This exeption is catched
                                                                                    // in the MappingAdminRouter
                ReloadResult reloadResult;
                if (onDeploy) {
                    reloadResult = onDeploy(mustReconnect);
                }

                res->status(200).json({{"status", "deploy-ack"},
                                       {"reload_mode", reloadResult.mode},
                                       {"instances", reloadResult.instances},
                                       {"subscribed", reloadResult.subscribed},
                                       {"unsubscribed", reloadResult.unsubscribed}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Deploy failed"}, {"details", e.what()}});
            }
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

        // GET /config/validateDraft
        api.get("/config/validateDraft", [mappingContext] APPLICATION(req, res) {
            try {
                const std::string draftPath = JsonMappingReader::getDraftPath(mappingContext->mappingFilePath);

                if (!std::filesystem::exists(draftPath)) {
                    res->status(404).json({{"valid", false}, {"error", "No draft configuration available"}, {"path", draftPath}});
                    return;
                }

                std::ifstream draftFile(draftPath);
                if (!draftFile) {
                    res->status(500).json({{"valid", false}, {"error", "Cannot open draft configuration"}, {"path", draftPath}});
                    return;
                }

                nlohmann::json draftDocument;
                draftFile >> draftDocument;

                nlohmann::json_schema::basic_error_handler err;
                MqttMapper::validate(draftDocument, err);

                if (err) {
                    res->status(422).json({{"valid", false}, {"error", "Draft validation failed"}, {"path", draftPath}});
                } else {
                    res->status(200).json({{"valid", true}, {"path", draftPath}});
                }
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

                nlohmann::json rolledbackMappingJson = JsonMappingReader::rollbackTo(mappingContext->mappingFilePath, versionId);

                bool mustReconnect = configApplication->setMapping(rolledbackMappingJson); // throws in case of an error during loading
                                                                                           // or validation. This exeption is catched
                                                                                           // in the MappingAdminRouter
                ReloadResult reloadResult;
                if (onDeploy) {
                    reloadResult = onDeploy(mustReconnect); // Trigger hot-reload
                }

                res->status(200).json({{"status", "deploy-ack"},
                                       {"reload_mode", reloadResult.mode},
                                       {"instances", reloadResult.instances},
                                       {"subscribed", reloadResult.subscribed},
                                       {"unsubscribed", reloadResult.unsubscribed}});
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

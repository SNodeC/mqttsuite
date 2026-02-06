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

#include "JsonMappingReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <express/middleware/BasicAuthentication.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>
//
#include <exception>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

using nlohmann::json;

namespace mqtt::lib::admin {

    express::Router makeMappingAdminRouter(const std::string& mappingFilePath, const AdminOptions& opt, ReloadCallback onDeploy) {
        express::Router api;
        const auto& schema = JsonMappingReader::getSchema();
        auto validator =
            std::make_shared<nlohmann::json_schema::json_validator>(schema, nullptr, nlohmann::json_schema::default_string_format_check);

        api.setStrictRouting();

        api.use(express::middleware::JsonMiddleware());
        api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

        // GET /schema
        api.get("/schema", [schema] APPLICATION(req, res) {
            res->status(200).json(schema);
        });

        // GET /config
        api.get("/config", [mappingFilePath] APPLICATION(req, res) {
            try {
                res->status(200).json(JsonMappingReader::readDraftOrActive(mappingFilePath));
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Failed to load configuration"}, {"details", e.what()}});
            }
        });

        // PATCH /config
        api.patch("/config", [mappingFilePath] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                json patchOps = json::parse(bodyStr);

                json current = JsonMappingReader::readDraftOrActive(mappingFilePath);
                current = current.patch(patchOps);

                JsonMappingReader::saveDraft(mappingFilePath, current);

                res->status(200).json({{"status", "patched"}, {"path", mappingFilePath}});
            } catch (const json::parse_error& e) {
                res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
            } catch (const std::exception& e) {
                res->status(422).json({{"error", "Patch application failed"}, {"details", e.what()}});
            }
        });

        // POST /config/deploy
        api.post("/config/deploy", [mappingFilePath, onDeploy] APPLICATION(req, res) {
            try {
                JsonMappingReader::deployDraft(mappingFilePath);
                if (onDeploy)
                    onDeploy();
                res->status(200).json({{"status", "deploy-ack"}, {"note", "hot-reload triggered"}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Deploy failed"}, {"details", e.what()}});
            }
        });

        // POST /config/validate
        api.post("/config/validate", [validator] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                auto document = nlohmann::json::parse(bodyStr);

                nlohmann::json_schema::basic_error_handler err;
                validator->validate(document, err);

                if (err) {
                    res->status(422).json({{"valid", false}, {"error", "Validation failed"}});
                } else {
                    res->status(200).json({{"valid", true}});
                }
            } catch (const std::exception& e) {
                res->status(400).json({{"error", "Validation exception"}, {"details", e.what()}});
            }
        });

        // POST /config/rollback
        api.post("/config/rollback", [mappingFilePath, onDeploy] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                auto jsonBody = nlohmann::json::parse(bodyStr);

                if (!jsonBody.contains("version_id")) {
                    res->status(400).json({{"error", "Missing version_id"}});
                    return;
                }

                std::string versionId = jsonBody["version_id"];
                JsonMappingReader::rollbackTo(mappingFilePath, versionId);

                if (onDeploy)
                    onDeploy(); // Trigger hot-reload

                res->status(200).json({{"status", "rolled_back"}, {"version", versionId}});
            } catch (const std::exception& e) {
                res->status(500).json({{"error", "Rollback failed"}, {"details", e.what()}});
            }
        });

        // GET /config/history
        api.get("/config/history", [mappingFilePath] APPLICATION(req, res) {
            try {
                auto history = JsonMappingReader::getHistory(mappingFilePath);
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

        api.get("/ui",
                express::middleware::StaticMiddleware("/home/voc/tmp/integrator/mqtt-integrator-ui/dist/mqtt-integrator-ui/browser"));

        return api;
    }

} // namespace mqtt::lib::admin

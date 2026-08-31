/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Tobias Pfeil
 *               2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "MappingAdminRouter.h"

#include "ConfigApplication.h"
#include "JsonMappingReader.h"
#include "MqttMapper.h"

#include <express/middleware/BasicAuthentication.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json-schema.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace {

    bool isSecretKey(const std::string& key) {
        return key == "password" || key == "pass" || key == "secret" || key == "token";
    }

    nlohmann::json redactSecrets(nlohmann::json value) {
        if (value.is_object()) {
            for (auto& [key, child] : value.items()) {
                if (isSecretKey(key)) {
                    child = "<redacted>";
                } else {
                    child = redactSecrets(std::move(child));
                }
            }
        } else if (value.is_array()) {
            for (auto& child : value) {
                child = redactSecrets(std::move(child));
            }
        }
        return value;
    }

} // namespace

namespace mqtt::lib::admin {

    express::Router makeMappingAdminRouter(ConfigApplication* configApplication, const AdminOptions& opt, ReloadCallback onDeploy) {
        express::Router api;

        api.use(express::middleware::JsonMiddleware());
        api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

        api.get("/schema", [] APPLICATION(req, res) {
            res->status(200).send(MqttMapper::getSchema());
        });

        api.get("/config", [configApplication] APPLICATION(req, res) {
            try {
                res->status(200).json(redactSecrets(configApplication->getMqttMapper()->getMapping()));
            } catch (const std::exception&) {
                res->status(500).json({{"error", "Failed to load configuration"}});
            }
        });

        api.patch("/config", [configApplication] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                nlohmann::json patchOps = nlohmann::json::parse(bodyStr);

                nlohmann::json current = configApplication->getMqttMapper()->getMapping();
                current = current.patch(patchOps);

                JsonMappingReader::saveDraft(configApplication->getMappingFilename(), current);

                res->status(200).json({{"status", "patched"}, {"path", configApplication->getMappingFilename()}});
            } catch (const nlohmann::json::parse_error&) {
                res->status(400).json({{"error", "Invalid JSON body"}});
            } catch (const std::exception&) {
                res->status(422).json({{"error", "Patch application failed"}});
            }
        });

        api.post("/config", [configApplication] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                nlohmann::json replacement = nlohmann::json::parse(bodyStr);

                if (!replacement.is_object()) {
                    res->status(422).json({{"error", "Config replacement must be a JSON object"}});
                    return;
                }

                JsonMappingReader::saveDraft(configApplication->getMappingFilename(), replacement);

                res->status(200).json({{"status", "replaced"}, {"path", configApplication->getMappingFilename()}});
            } catch (const nlohmann::json::parse_error&) {
                res->status(400).json({{"error", "Invalid JSON body"}});
            } catch (const std::exception&) {
                res->status(422).json({{"error", "Config replacement failed"}});
            }
        });

        api.post("/config/deploy", [configApplication, onDeploy] APPLICATION(req, res) {
            try {
                nlohmann::json newMappingJson = JsonMappingReader::deployDraft(configApplication->getMappingFilename());

                bool mustReconnect = configApplication->getMqttMapper()->setMapping(newMappingJson);
                configApplication->persistMapping();

                if (onDeploy) {
                    ReloadResult reloadResult = onDeploy(mustReconnect);

                    res->status(200).json({{"status", "deploy-ack"},
                                           {"reload_mode", reloadResult.mode},
                                           {"instances", reloadResult.instances},
                                           {"subscribed", reloadResult.subscribed},
                                           {"unsubscribed", reloadResult.unsubscribed}});
                } else {
                    res->status(200).json(
                        {{"status", "deploy-ack"}, {"reload_mode", "none"}, {"instances", 0}, {"subscribed", 0}, {"unsubscribed", 0}});
                }
            } catch (const std::exception&) {
                res->status(500).json({{"error", "Deploy failed"}});
            }
        });

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
            } catch (const std::exception&) {
                res->status(400).json({{"error", "Validation exception"}});
            }
        });

        api.get("/config/validateDraft", [configApplication] APPLICATION(req, res) {
            try {
                const std::string draftPath = JsonMappingReader::getDraftPath(configApplication->getMappingFilename());

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
            } catch (const std::exception&) {
                res->status(400).json({{"valid", false}, {"error", "Draft validation exception"}});
            }
        });

        api.post("/config/rollback", [configApplication, onDeploy] APPLICATION(req, res) {
            try {
                const std::string bodyStr(req->body.begin(), req->body.end());
                auto jsonBody = nlohmann::json::parse(bodyStr);

                if (!jsonBody.contains("version_id")) {
                    res->status(400).json({{"error", "Missing version_id"}});
                    return;
                }

                std::string versionId = jsonBody["version_id"];

                nlohmann::json rolledbackMappingJson = JsonMappingReader::rollbackTo(configApplication->getMappingFilename(), versionId);

                bool mustReconnect = configApplication->getMqttMapper()->setMapping(rolledbackMappingJson);
                configApplication->persistMapping();

                ReloadResult reloadResult;
                if (onDeploy) {
                    reloadResult = onDeploy(mustReconnect);
                }

                res->status(200).json({{"status", "deploy-ack"},
                                       {"reload_mode", reloadResult.mode},
                                       {"instances", reloadResult.instances},
                                       {"subscribed", reloadResult.subscribed},
                                       {"unsubscribed", reloadResult.unsubscribed}});
            } catch (const std::exception&) {
                res->status(500).json({{"error", "Rollback failed"}});
            }
        });

        api.get("/config/history", [configApplication] APPLICATION(req, res) {
            try {
                auto history = JsonMappingReader::getHistory(configApplication->getMappingFilename());
                nlohmann::json list = nlohmann::json::array();
                for (const auto& h : history) {
                    list.push_back({{"id", h.id}, {"comment", h.comment}, {"date", h.date}});
                }
                res->status(200).json(list);
            } catch (const std::exception&) {
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

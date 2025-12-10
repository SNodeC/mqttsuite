#include "AdminRouter.h"

#include "MappingStore.h"
#include "express/Router.h"
#include "express/middleware/BasicAuthentication.h"
#include "express/middleware/JsonMiddleware.h"
#include "lib/JsonMappingReader.h"
#include "lib/Mqtt.h"

#include <iostream>
#include <nlohmann/json.hpp>
using nlohmann::json;

express::Router makeAdminRouter(std::shared_ptr<MappingStore> store, const AdminOptions& opt) {
    express::Router api;

    api.use(express::middleware::JsonMiddleware());
    api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

    // POST /config/deploy
    api.post("/config/deploy", [store] APPLICATION(req, res) {
        mqtt::lib::JsonMappingReader::invalidate(store->path());
        mqtt::mqttintegrator::lib::Mqtt::reloadAll();
        res->status(200).json({{"status", "deploy-ack"}, {"note", "hot-reload triggered"}});
    });

    // GET /config
    api.get("/config", [store] APPLICATION(req, res) {
        try {
            res->status(200).json(store->load());
        } catch (const std::exception& e) {
            res->status(500).json({{"error", "Failed to load configuration"}, {"details", e.what()}});
        }
    });

    // PATCH /config
    api.patch("/config", [store] APPLICATION(req, res) {
        try {
            const std::string bodyStr(req->body.begin(), req->body.end());
            json patchOps = json::parse(bodyStr);

            store->modify([&patchOps](json& current) {
                current = current.patch(patchOps);
            });
            std::cout << "Applied patch: " << patchOps.dump() << std::endl;

            res->status(200).json({{"status", "patched"}, {"path", store->path()}});
        } catch (const json::parse_error& e) {
            res->status(400).json({{"error", "Invalid JSON body"}, {"details", e.what()}});
        } catch (const std::exception& e) {
            // This catches invalid patches (e.g., path not found, test op failed)
            res->status(422).json({{"error", "Patch application failed"}, {"details", e.what()}});
        }
    });

    return api;
}
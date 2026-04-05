#include "lib/Engine.h"

#include <core/SNodeC.h>
#include <express/legacy/in/Server.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/middleware/StaticMiddleware.h>
#include <express/tls/in/Server.h>

#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

express::Router getRouter(const std::string& webRoot, std::shared_ptr<mqtt::orchestrator::lib::Engine> engine) {
    const express::Router& jsonRouter = express::middleware::JsonMiddleware();

    jsonRouter.use("/api/orchestrator", [] MIDDLEWARE(req, res, next) {
        (void) req;
        res->set({{"Access-Control-Allow-Origin", "*"},
                  {"Access-Control-Allow-Headers", "Content-Type"},
                  {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"}});
        next();
    });

    jsonRouter.get("/api/orchestrator/blocks", [engine] APPLICATION(req, res) {
        (void) req;
        res->json(engine->getBlockCatalog());
    });

    jsonRouter.get("/api/orchestrator/scenarios", [engine] APPLICATION(req, res) {
        (void) req;
        res->json(engine->getScenarios());
    });

    jsonRouter.get("/api/orchestrator/scenarios/:id", [engine] APPLICATION(req, res) {
        const auto scenario = engine->getScenario(req->params["id"]);
        if (scenario.has_value()) {
            res->json(scenario.value());
        } else {
            res->status(404).json({{"error", "Scenario not found"}});
        }
    });

    jsonRouter.post("/api/orchestrator/scenarios", [engine] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [engine, &res](nlohmann::json& json) {
                try {
                    const nlohmann::json created = engine->createScenario(json);
                    res->status(201).json(created);
                } catch (const std::invalid_argument& ex) {
                    res->status(400).json({{"error", ex.what()}});
                }
            },
            [&res](const std::string& key) { res->status(400).json({{"error", "Invalid JSON body"}, {"detail", key}}); });
    });


    jsonRouter.post("/api/orchestrator/scenarios/:id/execute", [engine] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [engine, req, &res](nlohmann::json& json) {
                try {
                    res->json(engine->executeScenario(req->params["id"], json));
                } catch (const std::invalid_argument& ex) {
                    res->status(404).json({{"error", ex.what()}});
                }
            },
            [&res](const std::string& key) { res->status(400).json({{"error", "Invalid JSON body"}, {"detail", key}}); });
    });

    jsonRouter.get("/api/orchestrator/scenarios/:id/telemetry", [engine] APPLICATION(req, res) {
        res->json(engine->getTelemetry(req->params["id"]));
    });

    jsonRouter.del("/api/orchestrator/scenarios/:id", [engine] APPLICATION(req, res) {
        if (engine->deleteScenario(req->params["id"])) {
            res->status(204).end();
        } else {
            res->status(404).json({{"error", "Scenario not found"}});
        }
    });

    const express::Router router(jsonRouter);
    router.use("/", express::middleware::StaticMiddleware(webRoot));

    return router;
}

} // namespace

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const auto reportState = []([[maybe_unused]] const std::string& instanceName,
                                [[maybe_unused]] const core::socket::SocketAddress& socketAddress,
                                [[maybe_unused]] core::socket::State state) {
    };

    auto engine = std::make_shared<mqtt::orchestrator::lib::Engine>();

    const std::string webRoot = "/var/www/mqttsuite/mqttorchestrator";
    const express::Router router = getRouter(webRoot, engine);

    express::legacy::in::Server("in-http", router, reportState, [](net::in::stream::legacy::config::ConfigSocketServer* config) {
        config->setPort(8180);
        config->setRetry();
    });

    express::tls::in::Server("in-https", router, reportState, [](net::in::stream::tls::config::ConfigSocketServer* config) {
        config->setPort(8181);
        config->setRetry();
    });

    std::cout << "mqttorchestrator listening on http://0.0.0.0:8180 and https://0.0.0.0:8181" << std::endl;

    return core::SNodeC::start();
}

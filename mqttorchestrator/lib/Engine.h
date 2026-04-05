#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace mqtt::orchestrator::lib {

class Engine {
public:
    using json = nlohmann::json;

    Engine();

    [[nodiscard]] json getBlockCatalog() const;
    [[nodiscard]] json getScenarios() const;
    [[nodiscard]] std::optional<json> getScenario(const std::string& id) const;

    json createScenario(const json& candidate);
    [[nodiscard]] bool deleteScenario(const std::string& id);

    json executeScenario(const std::string& id, const json& executionRequest);
    [[nodiscard]] json getTelemetry(const std::string& id) const;

private:
    static bool hasValidNodeReferences(const json& scenario);
    static bool hasValidRoutes(const json& scenario);

    static json makeBlockCatalog();
    static json evaluateNode(const json& node, const json& inputs, json& telemetryStore);
    static double asNumber(const json& value, double fallback);

    json blockCatalog;
    std::unordered_map<std::string, json> scenarios;
    std::unordered_map<std::string, json> runtimeTelemetry;
    std::unordered_map<std::string, json> runtimeState;
};

} // namespace mqtt::orchestrator::lib

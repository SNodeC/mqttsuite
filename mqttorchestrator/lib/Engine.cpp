#include "Engine.h"

#include <cmath>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace mqtt::orchestrator::lib {

Engine::Engine()
    : blockCatalog(makeBlockCatalog()) {
}

nlohmann::json Engine::getBlockCatalog() const {
    return blockCatalog;
}

nlohmann::json Engine::getScenarios() const {
    nlohmann::json scenarioList = nlohmann::json::array();

    for (const auto& [id, scenario] : scenarios) {
        (void) id;
        scenarioList.push_back(scenario);
    }

    return {{"scenarios", scenarioList}};
}

std::optional<nlohmann::json> Engine::getScenario(const std::string& id) const {
    if (const auto it = scenarios.find(id); it != scenarios.end()) {
        return it->second;
    }

    return std::nullopt;
}

nlohmann::json Engine::createScenario(const nlohmann::json& candidate) {
    if (!candidate.contains("id") || !candidate["id"].is_string()) {
        throw std::invalid_argument("Scenario must contain a string field 'id'");
    }

    if (!candidate.contains("nodes") || !candidate["nodes"].is_array()) {
        throw std::invalid_argument("Scenario must contain an array field 'nodes'");
    }

    if (!candidate.contains("routes") || !candidate["routes"].is_array()) {
        throw std::invalid_argument("Scenario must contain an array field 'routes'");
    }

    if (!hasValidNodeReferences(candidate) || !hasValidRoutes(candidate)) {
        throw std::invalid_argument("Scenario contains invalid node or route references");
    }

    const std::string id = candidate["id"].get<std::string>();
    scenarios[id] = candidate;

    if (!runtimeTelemetry.contains(id)) {
        runtimeTelemetry[id] = nlohmann::json::object();
    }
    if (!runtimeState.contains(id)) {
        runtimeState[id] = nlohmann::json::object();
    }

    return scenarios[id];
}

bool Engine::deleteScenario(const std::string& id) {
    runtimeTelemetry.erase(id);
    runtimeState.erase(id);
    return scenarios.erase(id) == 1;
}

nlohmann::json Engine::executeScenario(const std::string& id, const nlohmann::json& executionRequest) {
    if (!scenarios.contains(id)) {
        throw std::invalid_argument("Scenario not found");
    }

    const nlohmann::json& scenario = scenarios[id];
    nlohmann::json& telemetryByNode = runtimeTelemetry[id];
    nlohmann::json& stateByNode = runtimeState[id];

    std::unordered_map<std::string, nlohmann::json> nodeById;
    for (const auto& node : scenario["nodes"]) {
        nodeById[node["id"].get<std::string>()] = node;
    }

    std::unordered_map<std::string, nlohmann::json> nodeInputs;
    if (executionRequest.contains("inputs") && executionRequest["inputs"].is_array()) {
        for (const auto& input : executionRequest["inputs"]) {
            if (input.contains("nodeId") && input.contains("input") && input.contains("value") && input["nodeId"].is_string() &&
                input["input"].is_string()) {
                nodeInputs[input["nodeId"].get<std::string>()][input["input"].get<std::string>()] = input["value"];
            }
        }
    }

    std::unordered_set<std::string> touchedNodes;
    for (const auto& [nodeId, values] : nodeInputs) {
        (void) values;
        touchedNodes.insert(nodeId);
    }

    nlohmann::json outputs = nlohmann::json::array();
    nlohmann::json trace = nlohmann::json::array();

    const std::size_t maxSteps = scenario["nodes"].size() * 8;
    std::size_t step = 0;
    bool changed = true;

    while (changed && step < maxSteps) {
        changed = false;
        ++step;

        for (const auto& node : scenario["nodes"]) {
            const std::string nodeId = node["id"].get<std::string>();
            const bool hasInputs = nodeInputs.contains(nodeId) && !nodeInputs[nodeId].empty();

            if (!hasInputs && touchedNodes.contains(nodeId)) {
                continue;
            }
            if (!hasInputs && !node.value("alwaysEvaluate", false)) {
                continue;
            }

            nlohmann::json currentInputs = hasInputs ? nodeInputs[nodeId] : nlohmann::json::object();
            nodeInputs[nodeId] = nlohmann::json::object();
            touchedNodes.insert(nodeId);

            nlohmann::json& nodeTelemetry = telemetryByNode[nodeId];
            nlohmann::json nodeOutputs = evaluateNode(node, currentInputs, nodeTelemetry);
            stateByNode[nodeId]["lastOutputs"] = nodeOutputs;

            trace.push_back({{"step", step}, {"nodeId", nodeId}, {"inputs", currentInputs}, {"outputs", nodeOutputs}});

            for (auto it = nodeOutputs.begin(); it != nodeOutputs.end(); ++it) {
                outputs.push_back({{"nodeId", nodeId}, {"output", it.key()}, {"value", it.value()}});
            }

            for (const auto& route : scenario["routes"]) {
                const std::string fromNode = route.value("fromNode", "");
                const std::string toNode = route.value("toNode", "");
                const std::string fromOutput = route.value("fromOutput", "");
                const std::string toInput = route.value("toInput", "");

                if (fromNode == nodeId && nodeOutputs.contains(fromOutput) && nodeById.contains(toNode)) {
                    nodeInputs[toNode][toInput] = nodeOutputs[fromOutput];
                    changed = true;
                }
            }
        }
    }

    return {{"scenarioId", id},
            {"steps", step},
            {"maxStepsReached", step >= maxSteps},
            {"outputs", outputs},
            {"trace", trace},
            {"telemetry", telemetryByNode}};
}

nlohmann::json Engine::getTelemetry(const std::string& id) const {
    if (const auto it = runtimeTelemetry.find(id); it != runtimeTelemetry.end()) {
        return {{"scenarioId", id}, {"telemetry", it->second}};
    }

    return {{"scenarioId", id}, {"telemetry", nlohmann::json::object()}};
}

bool Engine::hasValidNodeReferences(const nlohmann::json& scenario) {
    std::set<std::string> nodeIds;

    for (const auto& node : scenario["nodes"]) {
        if (!node.contains("id") || !node["id"].is_string() || !node.contains("type") || !node["type"].is_string()) {
            return false;
        }

        nodeIds.insert(node["id"].get<std::string>());
    }

    return nodeIds.size() == scenario["nodes"].size();
}

bool Engine::hasValidRoutes(const nlohmann::json& scenario) {
    std::set<std::string> nodeIds;

    for (const auto& node : scenario["nodes"]) {
        nodeIds.insert(node["id"].get<std::string>());
    }

    for (const auto& route : scenario["routes"]) {
        if (!route.contains("fromNode") || !route.contains("toNode") || !route.contains("fromOutput") || !route.contains("toInput") ||
            !route["fromNode"].is_string() || !route["toNode"].is_string() || !route["fromOutput"].is_string() || !route["toInput"].is_string()) {
            return false;
        }

        if (!nodeIds.contains(route["fromNode"].get<std::string>()) || !nodeIds.contains(route["toNode"].get<std::string>())) {
            return false;
        }
    }

    return true;
}

double Engine::asNumber(const nlohmann::json& value, const double fallback) {
    if (value.is_number()) {
        return value.get<double>();
    }

    if (value.is_string()) {
        try {
            return std::stod(value.get<std::string>());
        } catch (...) {
            return fallback;
        }
    }

    if (value.is_boolean()) {
        return value.get<bool>() ? 1.0 : 0.0;
    }

    return fallback;
}

nlohmann::json Engine::evaluateNode(const nlohmann::json& node, const nlohmann::json& inputs, nlohmann::json& telemetryStore) {
    const std::string type = node.value("type", "unknown");
    const nlohmann::json config = node.value("config", nlohmann::json::object());

    telemetryStore["execution_count"] = telemetryStore.value("execution_count", 0) + 1;
    telemetryStore["last_type"] = type;

    if (type == "mqtt.source") {
        return {{"payload", inputs.value("payload", nlohmann::json::object())}, {"topic", inputs.value("topic", "")}};
    }

    if (type == "mqtt.sink") {
        telemetryStore["tx_messages"] = telemetryStore.value("tx_messages", 0) + 1;
        return {{"published", true},
                {"topic", inputs.value("topic", config.value("defaultTopic", "orchestrator/out"))},
                {"payload", inputs.value("payload", nlohmann::json::object())}};
    }

    if (type == "logic.threshold") {
        const double value = asNumber(inputs.value("value", 0.0), 0.0);
        const double threshold = asNumber(config.value("threshold", 0.0), 0.0);
        const bool isAbove = value > threshold;
        return {{"isAbove", isAbove}, {"band", isAbove ? "high" : "low"}, {"value", value}};
    }

    if (type == "logic.and") {
        const bool a = inputs.value("a", false);
        const bool b = inputs.value("b", false);
        return {{"result", a && b}};
    }

    if (type == "logic.or") {
        const bool a = inputs.value("a", false);
        const bool b = inputs.value("b", false);
        return {{"result", a || b}};
    }

    if (type == "logic.not") {
        const bool value = inputs.value("value", false);
        return {{"result", !value}};
    }

    if (type == "logic.switch") {
        const bool condition = inputs.value("condition", false);
        return {{condition ? "true" : "false", inputs.value("value", nlohmann::json::object())}};
    }

    if (type == "transform.scale") {
        const double value = asNumber(inputs.value("value", 0.0), 0.0);
        const double factor = asNumber(config.value("factor", 1.0), 1.0);
        const double offset = asNumber(config.value("offset", 0.0), 0.0);
        return {{"result", value * factor + offset}};
    }

    if (type == "transform.concat") {
        const std::string lhs = inputs.value("lhs", "");
        const std::string rhs = inputs.value("rhs", "");
        const std::string separator = config.value("separator", "");
        return {{"result", lhs + separator + rhs}};
    }

    if (type == "transform.jsonpath") {
        return {{"result", inputs.value("payload", nlohmann::json::object())}};
    }

    if (type == "storage.memory") {
        return {{"value", inputs.value("value", nlohmann::json::object())}, {"updated", true}};
    }

    if (type == "time.schedule") {
        return {{"trigger", true}, {"profile", config.value("profile", "default")}};
    }

    if (type == "home.light") {
        const bool on = inputs.value("switch", false);
        const int brightness = static_cast<int>(std::round(asNumber(inputs.value("brightness", 100), 100)));
        return {{"state", {{"on", on}, {"brightness", brightness}, {"room", config.value("room", "unknown")}}}};
    }

    if (type == "home.thermostat") {
        const double target = asNumber(inputs.value("target", config.value("defaultTarget", 21.0)), 21.0);
        return {{"state", {{"target", target}, {"mode", inputs.value("mode", "auto")}}}};
    }

    if (type == "home.scene") {
        return {{"scene", inputs.value("name", config.value("name", "default-scene"))}, {"activated", true}};
    }

    if (type == "notify.webhook") {
        telemetryStore["notify_count"] = telemetryStore.value("notify_count", 0) + 1;
        return {{"sent", true}, {"target", config.value("url", "")}, {"payload", inputs.value("payload", nlohmann::json::object())}};
    }

    return {{"passthrough", inputs}};
}

nlohmann::json Engine::makeBlockCatalog() {
    const nlohmann::json sharedObjects = {
      {"inputSchema",
       {{{"id", "string"}, {"label", "string"}, {"datatype", "enum[number|string|bool|json]"}, {"required", "bool"}, {"description", "string"}}}},
      {"outputSchema",
       {{{"id", "string"}, {"label", "string"}, {"datatype", "enum[number|string|bool|json]"}, {"retain", "bool"}, {"description", "string"}}}},
      {"telemetrySchema",
       {{{"key", "string"}, {"description", "string"}, {"type", "enum[counter|gauge|state|histogram]"}, {"unit", "string"}}}}
    };

    const auto block = [](const std::string& type,
                          const std::string& category,
                          const std::string& description,
                          const nlohmann::json& inputs,
                          const nlohmann::json& outputs,
                          const nlohmann::json& telemetry) {
        return nlohmann::json{{"type", type},
                              {"category", category},
                              {"description", description},
                              {"inputs", inputs},
                              {"outputs", outputs},
                              {"telemetry", telemetry}};
    };

    const nlohmann::json blocks = nlohmann::json::array({
      block("mqtt.source",
            "connectors",
            "Subscribes to MQTT topics and emits payload/message metadata.",
            {{{"id", "payload"}, {"label", "Payload"}, {"datatype", "json"}, {"required", false}, {"description", "Injected payload for execution"}},
             {{"id", "topic"}, {"label", "Topic"}, {"datatype", "string"}, {"required", false}, {"description", "MQTT topic name"}}},
            {{{"id", "payload"}, {"label", "Payload"}, {"datatype", "json"}, {"retain", false}, {"description", "Message payload"}},
             {{"id", "topic"}, {"label", "Topic"}, {"datatype", "string"}, {"retain", false}, {"description", "Message topic"}}},
            {{{"key", "rx_messages"}, {"description", "Received messages"}, {"type", "counter"}, {"unit", "messages"}}}),
      block("mqtt.sink",
            "connectors",
            "Publishes outbound values to MQTT topics.",
            {{{"id", "payload"}, {"label", "Payload"}, {"datatype", "json"}, {"required", true}, {"description", "Payload to publish"}},
             {{"id", "topic"}, {"label", "Topic"}, {"datatype", "string"}, {"required", false}, {"description", "Target topic"}}},
            {{{"id", "published"}, {"label", "Published"}, {"datatype", "bool"}, {"retain", false}, {"description", "Publish success flag"}}},
            {{{"key", "tx_messages"}, {"description", "Published messages"}, {"type", "counter"}, {"unit", "messages"}}}),
      block("logic.threshold",
            "logic",
            "Compares numeric values against configured threshold.",
            {{{"id", "value"}, {"label", "Value"}, {"datatype", "number"}, {"required", true}, {"description", "Input value"}}},
            {{{"id", "isAbove"}, {"label", "Is Above"}, {"datatype", "bool"}, {"retain", false}, {"description", "Threshold result"}},
             {{"id", "band"}, {"label", "Band"}, {"datatype", "string"}, {"retain", false}, {"description", "high|low"}}},
            {{{"key", "evaluation_count"}, {"description", "Total evaluations"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("logic.and", "logic", "Boolean AND.", {{{"id", "a"}, {"label", "A"}, {"datatype", "bool"}, {"required", true}, {"description", "Operand A"}}, {{"id", "b"}, {"label", "B"}, {"datatype", "bool"}, {"required", true}, {"description", "Operand B"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "bool"}, {"retain", false}, {"description", "A && B"}}}, {{{"key", "evaluation_count"}, {"description", "Total evaluations"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("logic.or", "logic", "Boolean OR.", {{{"id", "a"}, {"label", "A"}, {"datatype", "bool"}, {"required", true}, {"description", "Operand A"}}, {{"id", "b"}, {"label", "B"}, {"datatype", "bool"}, {"required", true}, {"description", "Operand B"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "bool"}, {"retain", false}, {"description", "A || B"}}}, {{{"key", "evaluation_count"}, {"description", "Total evaluations"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("logic.not", "logic", "Boolean inversion.", {{{"id", "value"}, {"label", "Value"}, {"datatype", "bool"}, {"required", true}, {"description", "Input value"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "bool"}, {"retain", false}, {"description", "!value"}}}, {{{"key", "evaluation_count"}, {"description", "Total evaluations"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("logic.switch", "logic", "Routes value to true/false output.", {{{"id", "condition"}, {"label", "Condition"}, {"datatype", "bool"}, {"required", true}, {"description", "Condition selector"}}, {{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"required", true}, {"description", "Value to route"}}}, {{{"id", "true"}, {"label", "True"}, {"datatype", "json"}, {"retain", false}, {"description", "Condition true"}}, {{"id", "false"}, {"label", "False"}, {"datatype", "json"}, {"retain", false}, {"description", "Condition false"}}}, {{{"key", "route_count"}, {"description", "Routed messages"}, {"type", "counter"}, {"unit", "messages"}}}),
      block("transform.scale", "transform", "Linear scaling with factor and offset.", {{{"id", "value"}, {"label", "Value"}, {"datatype", "number"}, {"required", true}, {"description", "Input number"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "number"}, {"retain", false}, {"description", "Scaled number"}}}, {{{"key", "transform_count"}, {"description", "Transforms"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("transform.concat", "transform", "Concatenate two strings.", {{{"id", "lhs"}, {"label", "LHS"}, {"datatype", "string"}, {"required", true}, {"description", "Left string"}}, {{"id", "rhs"}, {"label", "RHS"}, {"datatype", "string"}, {"required", true}, {"description", "Right string"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "string"}, {"retain", false}, {"description", "Concatenated value"}}}, {{{"key", "transform_count"}, {"description", "Transforms"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("transform.jsonpath", "transform", "Extract values from JSON payload (planned full JSONPath parser).", {{{"id", "payload"}, {"label", "Payload"}, {"datatype", "json"}, {"required", true}, {"description", "JSON payload"}}}, {{{"id", "result"}, {"label", "Result"}, {"datatype", "json"}, {"retain", false}, {"description", "Extracted value"}}}, {{{"key", "transform_count"}, {"description", "Transforms"}, {"type", "counter"}, {"unit", "runs"}}}),
      block("time.schedule", "time", "Evaluates schedule profile and emits trigger.", nlohmann::json::array(), {{{"id", "trigger"}, {"label", "Trigger"}, {"datatype", "bool"}, {"retain", false}, {"description", "Schedule trigger"}}}, {{{"key", "next_fire_epoch"}, {"description", "Next schedule time"}, {"type", "state"}, {"unit", "epoch"}}}),
      block("storage.memory", "storage", "In-memory key-value storage node.", {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"required", true}, {"description", "Value to persist"}}}, {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"retain", true}, {"description", "Stored value"}}}, {{{"key", "updates"}, {"description", "Update counter"}, {"type", "counter"}, {"unit", "updates"}}}),
      block("notify.webhook", "notification", "Sends payload to webhook endpoint.", {{{"id", "payload"}, {"label", "Payload"}, {"datatype", "json"}, {"required", true}, {"description", "Notification payload"}}}, {{{"id", "sent"}, {"label", "Sent"}, {"datatype", "bool"}, {"retain", false}, {"description", "Webhook send status"}}}, {{{"key", "notify_count"}, {"description", "Notifications sent"}, {"type", "counter"}, {"unit", "messages"}}}),
      block("notify.email", "notification", "Dispatches email alert (adapter).", {{{"id", "subject"}, {"label", "Subject"}, {"datatype", "string"}, {"required", true}, {"description", "Mail subject"}}, {{"id", "body"}, {"label", "Body"}, {"datatype", "string"}, {"required", true}, {"description", "Mail body"}}}, {{{"id", "queued"}, {"label", "Queued"}, {"datatype", "bool"}, {"retain", false}, {"description", "Queue status"}}}, {{{"key", "notify_count"}, {"description", "Notifications queued"}, {"type", "counter"}, {"unit", "messages"}}}),
      block("home.light", "home-automation", "Controls smart lights.", {{{"id", "switch"}, {"label", "Switch"}, {"datatype", "bool"}, {"required", true}, {"description", "On/off"}}, {{"id", "brightness"}, {"label", "Brightness"}, {"datatype", "number"}, {"required", false}, {"description", "Brightness 0..100"}}}, {{{"id", "state"}, {"label", "State"}, {"datatype", "json"}, {"retain", true}, {"description", "Light state"}}}, {{{"key", "command_count"}, {"description", "Commands"}, {"type", "counter"}, {"unit", "commands"}}}),
      block("home.thermostat", "home-automation", "Controls HVAC target temperature.", {{{"id", "target"}, {"label", "Target"}, {"datatype", "number"}, {"required", false}, {"description", "Target temperature"}}, {{"id", "mode"}, {"label", "Mode"}, {"datatype", "string"}, {"required", false}, {"description", "heat|cool|auto|off"}}}, {{{"id", "state"}, {"label", "State"}, {"datatype", "json"}, {"retain", true}, {"description", "Thermostat state"}}}, {{{"key", "command_count"}, {"description", "Commands"}, {"type", "counter"}, {"unit", "commands"}}}),
      block("home.blind", "home-automation", "Controls smart blinds position.", {{{"id", "position"}, {"label", "Position"}, {"datatype", "number"}, {"required", true}, {"description", "0..100"}}}, {{{"id", "state"}, {"label", "State"}, {"datatype", "json"}, {"retain", true}, {"description", "Blind state"}}}, {{{"key", "command_count"}, {"description", "Commands"}, {"type", "counter"}, {"unit", "commands"}}}),
      block("home.lock", "home-automation", "Controls smart lock state.", {{{"id", "lock"}, {"label", "Lock"}, {"datatype", "bool"}, {"required", true}, {"description", "Lock/unlock"}}}, {{{"id", "state"}, {"label", "State"}, {"datatype", "json"}, {"retain", true}, {"description", "Lock state"}}}, {{{"key", "command_count"}, {"description", "Commands"}, {"type", "counter"}, {"unit", "commands"}}}),
      block("home.scene", "home-automation", "Activates a scene.", {{{"id", "name"}, {"label", "Name"}, {"datatype", "string"}, {"required", true}, {"description", "Scene name"}}}, {{{"id", "activated"}, {"label", "Activated"}, {"datatype", "bool"}, {"retain", false}, {"description", "Scene activation result"}}}, {{{"key", "activations"}, {"description", "Scene activations"}, {"type", "counter"}, {"unit", "activations"}}}),
      block("sensor.temperature", "sensors", "Virtual temperature source adapter.", nlohmann::json::array(), {{{"id", "value"}, {"label", "Value"}, {"datatype", "number"}, {"retain", false}, {"description", "Temperature value"}}}, {{{"key", "reads"}, {"description", "Sensor reads"}, {"type", "counter"}, {"unit", "reads"}}}),
      block("sensor.humidity", "sensors", "Virtual humidity source adapter.", nlohmann::json::array(), {{{"id", "value"}, {"label", "Value"}, {"datatype", "number"}, {"retain", false}, {"description", "Humidity value"}}}, {{{"key", "reads"}, {"description", "Sensor reads"}, {"type", "counter"}, {"unit", "reads"}}}),
      block("system.delay", "control", "Delays messages (runtime queue planned).", {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"required", true}, {"description", "Value to delay"}}}, {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"retain", false}, {"description", "Delayed value"}}}, {{{"key", "queued"}, {"description", "Queued items"}, {"type", "gauge"}, {"unit", "items"}}}),
      block("system.rate-limit", "control", "Rate limits message throughput.", {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"required", true}, {"description", "Value to pass"}}}, {{{"id", "value"}, {"label", "Value"}, {"datatype", "json"}, {"retain", false}, {"description", "Rate-limited value"}}}, {{{"key", "dropped"}, {"description", "Dropped messages"}, {"type", "counter"}, {"unit", "messages"}}})
    });

    return {{"schemaVersion", "2.0"}, {"sharedObjects", sharedObjects}, {"blocks", blocks}};
}

} // namespace mqtt::orchestrator::lib

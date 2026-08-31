#include "mqttstore/lib/StoragePlan.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <string>

namespace {

    void testValidProjection() {
        const nlohmann::json configuration = {
            {"projections",
             nlohmann::json::array({{{"name", "temperature"},
                                      {"topic", "normalized/+/temperature"},
                                      {"table", "sensor_measurements"},
                                      {"columns",
                                       {{"device_id", {{"topic_level", 1}, {"required", true}}},
                                        {"value", {{"json_pointer", "/value"}, {"required", true}}},
                                        {"unit", "/unit"}}}}})}};

        const auto plan = mqtt::mqttstore::lib::StoragePlan::fromJson(configuration);
        assert(plan.getProjections().size() == 1);
        assert(plan.match("normalized/node-01/temperature").size() == 1);
        assert(plan.match("normalized/node-01/humidity").empty());
    }

    void testSchemaInvalidProjectionFails() {
        bool rejected = false;
        try {
            static_cast<void>(mqtt::mqttstore::lib::StoragePlan::fromJson(
                {{"projections", nlohmann::json::array({{{"topic", "normalized/#"}, {"table", "bad-name!"}, {"columns", {{"value", "/value"}}}}})}}));
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        assert(rejected);
    }

} // namespace

int main() {
    testValidProjection();
    testSchemaInvalidProjectionFails();
    return 0;
}

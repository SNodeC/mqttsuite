#include "lib/MqttMapper.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Publish.h>
#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <string>
#include <tuple>

namespace {

    nlohmann::json valueSubscription(const std::string& mappedTopic) {
        return {{"value", {{"mapped_topic", mappedTopic}, {"mapping_template", "{{ message }}"}}}};
    }

    nlohmann::json wildcardMapping() {
        return {
            {"mapping",
             {{"topic_level",
               nlohmann::json::array(
                   {{{"name", "sensors"},
                     {"topic_level",
                      nlohmann::json::array({{{"name", "literal"}, {"subscription", valueSubscription("literal/out")}},
                                             {{"name", "+"}, {"subscription", valueSubscription("plus/out")}},
                                             {{"name", "#"}, {"subscription", valueSubscription("hash/out")}}})}}})}}}};
    }

    std::string mappedTopic(mqtt::lib::MqttMapper& mapper, const std::string& topic) {
        const iot::mqtt::packets::Publish publish(0, topic, "payload", 0, false, false);
        auto mapped = mapper.getMappings(publish);
        const auto& immediate = std::get<0>(mapped);
        return immediate.empty() ? std::string{} : immediate.front().getTopic();
    }

    void testWildcardSemanticsAndPrecedence() {
        mqtt::lib::MqttMapper mapper;
        mapper.setMapping(wildcardMapping());

        assert(mappedTopic(mapper, "sensors/literal") == "literal/out");
        assert(mappedTopic(mapper, "sensors/other") == "plus/out");
        assert(mappedTopic(mapper, "sensors/other/deep") == "hash/out");
        assert(mappedTopic(mapper, "sensors") == "hash/out");

        const auto subscriptions = mapper.extractSubscriptions();
        bool sawHash = false;
        for (const auto& subscription : subscriptions) {
            if (subscription.getName() == "sensors/#") {
                sawHash = true;
            }
        }
        assert(sawHash);
    }

    void testPlusIsSingleLevelOnly() {
        mqtt::lib::MqttMapper mapper;
        mapper.setMapping({{"mapping",
                            {{"topic_level",
                              {{{"name", "devices"},
                                {"topic_level", {{{"name", "+"}, {"subscription", valueSubscription("plus-only/out")}}}}}}}}}}});

        assert(mappedTopic(mapper, "devices/node") == "plus-only/out");
        assert(mappedTopic(mapper, "devices/node/deep").empty());
    }

    void testIllegalHashPlacementRejectedWithoutSecretEcho() {
        mqtt::lib::MqttMapper mapper;
        const std::string sentinel = "SENTINEL-MAPPING-PASSWORD";
        nlohmann::json mapping = {
            {"connection", {{"password", sentinel}}},
            {"mapping",
             {{"topic_level",
               {{{"name", "#"},
                 {"topic_level", {{{"name", "child"}, {"subscription", valueSubscription("invalid/out")}}}}}}}}}};

        bool rejected = false;
        try {
            mapper.setMapping(mapping);
        } catch (const std::runtime_error& error) {
            rejected = true;
            assert(std::string(error.what()).find(sentinel) == std::string::npos);
        }
        assert(rejected);
    }

} // namespace

int main() {
    testWildcardSemanticsAndPrecedence();
    testPlusIsSingleLevelOnly();
    testIllegalHashPlacementRejectedWithoutSecretEcho();
    return 0;
}

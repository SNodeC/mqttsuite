#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <list>
#include <log/Logger.h>
#include <nlohmann/json.hpp>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttintegrator::lib {

    std::set<Mqtt*> Mqtt::instances;

    Mqtt::Mqtt(const std::string& connectionName,
               const nlohmann::json& connectionJson,
               mqtt::lib::MqttMapper* mqttMapper,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, //
                                  connectionJson["client_id"],
                                  connectionJson["keep_alive"],
                                  sessionStoreFileName)
        , connectionJson(connectionJson)
        , mqttMapper(mqttMapper)
        , delayedQueue(this) {
        instances.insert(this);
    }

    Mqtt::~Mqtt() {
        instances.erase(this);
    }

    bool Mqtt::EarlierFirst::operator()(const ScheduledPublish& a, const ScheduledPublish& b) const {
        if (a.when != b.when) {
            return a.when > b.when;
        }

        return a.seq > b.seq;
    }

    Mqtt::DelayedQueue::DelayedQueue(Mqtt* mqtt)
        : mqtt(mqtt) {
    }

    Mqtt::DelayedQueue::~DelayedQueue() {
        delayTimer.cancel();
    }

    void Mqtt::DelayedQueue::processDue() {
        const auto now = utils::Timeval::currentTime();

        while (!empty() && top().when <= now) {
            const iot::mqtt::packets::Publish duePublish = top().publish;
            mqtt->sendPublish(duePublish.getTopic(), duePublish.getMessage(), duePublish.getQoS(), duePublish.getRetain());

            mqtt::lib::MqttMapper::MappedPublishes mappedPublishes = mqtt->mqttMapper->publishMappings(duePublish);
            for (const iot::mqtt::packets::Publish& mappedPublish : mappedPublishes.first) {
                mqtt->sendPublish(mappedPublish.getTopic(), mappedPublish.getMessage(), mappedPublish.getQoS(), mappedPublish.getRetain());
            }
            for (const mqtt::lib::MqttMapper::ScheduledPublish& delayedPublish : mappedPublishes.second) {
                delayPublish(delayedPublish.delay, delayedPublish.publish);
            }

            pop();
        }
    }

    void Mqtt::DelayedQueue::armDelayTimer() {
        delayTimer.cancel();

        auto delay = top().when - utils::Timeval::currentTime();
        if (delay < utils::Timeval{}) {
            delay = utils::Timeval{};
        }

        delayTimer = core::timer::Timer::singleshotTimer(
            [this]() {
                processDue();

                if (!empty()) {
                    armDelayTimer();
                }
            },
            delay);
    }

    void Mqtt::DelayedQueue::delayPublish(const utils::Timeval& delay, const iot::mqtt::packets::Publish& publish) {
        minHeap.push({utils::Timeval::currentTime() + delay, nextSeq++, publish, delay});
        armDelayTimer();
    }

    bool Mqtt::DelayedQueue::empty() const {
        return minHeap.empty();
    }

    Mqtt::ScheduledPublish const& Mqtt::DelayedQueue::top() const {
        return minHeap.top();
    }

    void Mqtt::DelayedQueue::pop() {
        minHeap.pop();
    }

    void Mqtt::reloadAll() {
        for (auto* instance : instances) {
            instance->sendDisconnect();
        }
    }

    void Mqtt::onConnected() {
        sendConnect(connectionJson["clean_session"],
                    connectionJson["will_topic"],
                    connectionJson["will_message"],
                    connectionJson["will_qos"],
                    connectionJson["will_retain"],
                    connectionJson["username"],
                    connectionJson["password"]);
    }

    bool Mqtt::onSignal(int signum) {
        sendDisconnect();
        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendPublish("snode.c/_cfg_/connection", connectionJson.dump(), 0, true);

            const std::list<iot::mqtt::Topic> topicList = mqttMapper->extractSubscriptions();
            sendSubscribe(topicList);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        mqtt::lib::MqttMapper::MappedPublishes mappedPublishes = mqttMapper->publishMappings(publish);

        for (const iot::mqtt::packets::Publish& mappedPublish : mappedPublishes.first) {
            sendPublish(mappedPublish.getTopic(), mappedPublish.getMessage(), mappedPublish.getQoS(), mappedPublish.getRetain());
        }

        for (const mqtt::lib::MqttMapper::ScheduledPublish& delayedPublish : mappedPublishes.second) {
            delayedQueue.delayPublish(delayedPublish.delay, delayedPublish.publish);
        }
    }

} // namespace mqtt::mqttintegrator::lib

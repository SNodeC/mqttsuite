#include "Mqtt.h"

#include "mqttbroker/lib/MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Subscribe.h>
#include <iot/mqtt/packets/Unsubscribe.h>
#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <log/Logger.h>

#endif

namespace mqtt::mqttbroker::lib {

    Mqtt::Mqtt(const std::string& connectionName,
               const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
               mqtt::lib::MqttMapper* mqttMapper)
        : iot::mqtt::server::Mqtt(connectionName, broker)
        , mqttMapper(mqttMapper)
        , delayedQueue(this) {
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

            mqtt->broker->publish(
                mqtt->clientId, duePublish.getTopic(), duePublish.getMessage(), duePublish.getQoS(), duePublish.getRetain());

            mqtt::lib::MqttMapper::MappedPublishes mappedPublishes = mqtt->mqttMapper->publishMappings(duePublish);
            for (const iot::mqtt::packets::Publish& mappedPublish : mappedPublishes.first) {
                mqtt->broker->publish(mqtt->clientId,
                                      mappedPublish.getTopic(),
                                      mappedPublish.getMessage(),
                                      mappedPublish.getQoS(),
                                      mappedPublish.getRetain());
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

    void Mqtt::subscribe(const std::string& topic, uint8_t qoS) {
        broker->subscribe(clientId, topic, qoS);
        onSubscribe(iot::mqtt::packets::Subscribe(0, {{topic, qoS}}));
    }

    void Mqtt::unsubscribe(const std::string& topic) {
        broker->unsubscribe(clientId, topic);
        onUnsubscribe(iot::mqtt::packets::Unsubscribe(0, {{topic, 0}}));
    }

    void Mqtt::onConnect([[maybe_unused]] const iot::mqtt::packets::Connect& connect) {
        MqttModel::instance().connectClient(this);
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        MqttModel::instance().publishMessage(publish.getTopic(), publish.getMessage(), publish.getQoS(), publish.getRetain());

        if (mqttMapper != nullptr) {
            mqtt::lib::MqttMapper::MappedPublishes mappedPublishes = mqttMapper->publishMappings(publish);

            for (const iot::mqtt::packets::Publish& mappedPublish : mappedPublishes.first) {
                broker->publish(
                    clientId, mappedPublish.getTopic(), mappedPublish.getMessage(), mappedPublish.getQoS(), mappedPublish.getRetain());

                mqtt::lib::MqttMapper::MappedPublishes recursiveMappedPublishes = mqttMapper->publishMappings(mappedPublish);
                for (const iot::mqtt::packets::Publish& recursiveMappedPublish : recursiveMappedPublishes.first) {
                    broker->publish(clientId,
                                    recursiveMappedPublish.getTopic(),
                                    recursiveMappedPublish.getMessage(),
                                    recursiveMappedPublish.getQoS(),
                                    recursiveMappedPublish.getRetain());
                }
                for (const mqtt::lib::MqttMapper::ScheduledPublish& delayedPublish : recursiveMappedPublishes.second) {
                    delayedQueue.delayPublish(delayedPublish.delay, delayedPublish.publish);
                }
            }

            for (const mqtt::lib::MqttMapper::ScheduledPublish& delayedPublish : mappedPublishes.second) {
                delayedQueue.delayPublish(delayedPublish.delay, delayedPublish.publish);
            }
        }
    }

    void Mqtt::onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) {
        for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
            MqttModel::instance().subscribeClient(clientId, topic.getName(), topic.getQoS());
        }
    }

    void Mqtt::onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) {
        for (const std::string& topic : unsubscribe.getTopics()) {
            MqttModel::instance().unsubscribeClient(clientId, topic);
        }
    }

    void Mqtt::onDisconnected() {
        MqttModel::instance().disconnectClient(clientId);
    }

} // namespace mqtt::mqttbroker::lib

/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
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

#include "Mqtt.h"

#include "lib/MqttMapper.h"
#include "mqttbroker/lib/MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Subscribe.h>
#include <iot/mqtt/packets/Unsubscribe.h>
#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>

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

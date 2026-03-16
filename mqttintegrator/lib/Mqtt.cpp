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

#include <iot/mqtt/Topic.h> // IWYU pragma: keep
#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <nlohmann/json_fwd.hpp>

#endif

namespace mqtt::mqttintegrator::lib {

    std::set<Mqtt*> Mqtt::instances;

    Mqtt::Mqtt(const std::string& connectionName,
               std::shared_ptr<mqtt::lib::MqttMapper> mqttMapper,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, //
                                  mqttMapper->getConnection()["client_id"],
                                  mqttMapper->getConnection()["keep_alive"],
                                  sessionStoreFileName)
        , mqttMapper(mqttMapper)
        , currentSubscriptions(mqttMapper->extractSubscriptions())
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
            pop();

            mqtt->sendPublish(duePublish.getTopic(), duePublish.getMessage(), duePublish.getQoS(), duePublish.getRetain());

            mqtt->onPublish(duePublish);
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
        minHeap.emplace(utils::Timeval::currentTime() + delay, nextSeq++, publish, delay);
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

    void Mqtt::reloadSubscriptions() {
        for (auto* instance : instances) {
            instance->resubscribe();
        }
    }

    void Mqtt::onConnected() {
        const nlohmann::json& connection = mqttMapper->getConnection();

        sendConnect(connection["clean_session"],
                    connection["will_topic"],
                    connection["will_message"],
                    connection["will_qos"],
                    connection["will_retain"],
                    connection["username"],
                    connection["password"]);
    }

    bool Mqtt::onSignal(int signum) {
        sendDisconnect();
        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendSubscribe(currentSubscriptions);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        mqtt::lib::MqttMapper::MappedPublishes mappedPublishes = mqttMapper->publishMappings(publish);

        for (const mqtt::lib::MqttMapper::ScheduledPublish& delayedPublish : mappedPublishes.second) {
            delayedQueue.delayPublish(delayedPublish.delay, delayedPublish.publish);
        }

        for (const iot::mqtt::packets::Publish& mappedPublish : mappedPublishes.first) {
            sendPublish(mappedPublish.getTopic(), mappedPublish.getMessage(), mappedPublish.getQoS(), mappedPublish.getRetain());

            onPublish(mappedPublish);
        }
    }

    void Mqtt::resubscribe() {
        std::list<iot::mqtt::Topic> newSubscriptions = mqttMapper->extractSubscriptions();

        std::list<std::string> topicsToUnsubscribe;
        for (const auto& currentTopic : currentSubscriptions) {
            const bool existsInNew = std::any_of(newSubscriptions.begin(), newSubscriptions.end(), [&](const auto& newTopic) {
                return currentTopic.getName() == newTopic.getName() && currentTopic.getQoS() == newTopic.getQoS();
            });

            if (!existsInNew) {
                topicsToUnsubscribe.push_back(currentTopic.getName());
            }
        }

        if (!topicsToUnsubscribe.empty()) {
            sendUnsubscribe(topicsToUnsubscribe);
        }

        std::list<iot::mqtt::Topic> topicsToSubscribe;
        for (const auto& newTopic : newSubscriptions) {
            const bool existsInOld = std::any_of(currentSubscriptions.begin(), currentSubscriptions.end(), [&](const auto& currentTopic) {
                return currentTopic.getName() == newTopic.getName() && currentTopic.getQoS() == newTopic.getQoS();
            });

            if (!existsInOld) {
                topicsToSubscribe.push_back(newTopic);
            }
        }

        if (!topicsToSubscribe.empty()) {
            sendSubscribe(topicsToSubscribe);
        }

        currentSubscriptions = newSubscriptions;
    }

} // namespace mqtt::mqttintegrator::lib

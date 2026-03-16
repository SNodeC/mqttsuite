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

#ifndef APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H

#include <iot/mqtt/client/Mqtt.h>
#include <iot/mqtt/packets/Publish.h>

namespace mqtt::lib {
    class MqttMapper;
} // namespace mqtt::lib

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

#endif

namespace mqtt::mqttintegrator::lib {

    class Mqtt : public iot::mqtt::client::Mqtt {
    public:
        explicit Mqtt(const std::string& connectionName,
                      std::shared_ptr<mqtt::lib::MqttMapper> mqttMapper,
                      const std::string& sessionStoreFileName);

        ~Mqtt() override;
        static void reloadAll();

    private:
        using Super = iot::mqtt::client::Mqtt;

        struct ScheduledPublish {
            utils::Timeval when = 0;
            std::size_t seq = 0;
            iot::mqtt::packets::Publish publish;
            utils::Timeval delay;
        };

        struct EarlierFirst {
            bool operator()(const ScheduledPublish& a, const ScheduledPublish& b) const;
        };

        class DelayedQueue {
        public:
            explicit DelayedQueue(Mqtt* mqtt);
            ~DelayedQueue();

            void delayPublish(const utils::Timeval& delay, const iot::mqtt::packets::Publish& publish);

            bool empty() const;
            ScheduledPublish const& top() const;
            void pop();

        private:
            Mqtt* mqtt;
            std::size_t nextSeq = 0;
            std::priority_queue<ScheduledPublish, std::vector<ScheduledPublish>, EarlierFirst> minHeap;

            core::timer::Timer delayTimer;
            void processDue();
            void armDelayTimer();
        };

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;

        std::shared_ptr<mqtt::lib::MqttMapper> mqttMapper;
        DelayedQueue delayedQueue;

        static std::set<Mqtt*> instances;
    };

} // namespace mqtt::mqttintegrator::lib

#endif // APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H

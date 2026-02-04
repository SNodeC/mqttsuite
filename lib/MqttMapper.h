/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
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

#ifndef MQTTBROKER_LIB_MQTTMAPPER_H
#define MQTTBROKER_LIB_MQTTMAPPER_H

namespace iot::mqtt {
    class Topic;
    namespace packets {
        class Publish;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace inja {
    class Environment;
}

#include <core/timer/Timer.h>
#include <cstddef>
#include <cstdint>
#include <list>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <queue>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib {

    class MqttMapper {
    public:
        MqttMapper(const nlohmann::json& mappingJson);
        MqttMapper(const MqttMapper&) = delete;
        MqttMapper& operator=(const MqttMapper&) = delete;

        virtual ~MqttMapper();

    protected:
        std::string dump();

        std::list<iot::mqtt::Topic> extractSubscriptions();
        void publishMappings(const iot::mqtt::packets::Publish& publish);

    private:
        virtual void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) = 0;
        void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain, double delay);

        static void
        extractSubscription(const nlohmann::json& topicLevelJson, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);
        static void
        extractSubscriptions(const nlohmann::json& mappingJson, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);

        nlohmann::json findMatchingTopicLevel(const nlohmann::json& topicLevel, const std::string& topic);

        void publishMappedTemplate(const nlohmann::json& templateMapping, nlohmann::json& json);
        void
        publishMappedTemplates(const nlohmann::json& templateMapping, nlohmann::json& json, const iot::mqtt::packets::Publish& publish);

        void publishMappedMessage(const std::string& topic, const std::string& message, uint8_t qoS, bool retain, double delay);
        void publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish);
        void publishMappedMessages(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish);

        const nlohmann::json& mappingJson;

        std::list<void*> pluginHandles;

        inja::Environment* injaEnvironment;

        struct ScheduledPublish {
            utils::Timeval when;
            utils::Timeval delay;
            std::size_t seq; // tie-breaker (strict ordering)
            std::string topic;
            std::string message;
            uint8_t qoS;
            bool retain;
        };

        // min-heap by time (earliest on top), then seq
        struct EarlierFirst {
            bool operator()(ScheduledPublish const& a, ScheduledPublish const& b) const;
        };

        class DelayedQueue {
        public:
            DelayedQueue(MqttMapper* mqttMapper);

            ~DelayedQueue();

            void
            delayAbsolute(const utils::Timeval& timeval, const std::string& topic, const std::string& message, uint8_t qoS, bool retain);

            void
            delayPublish(const utils::Timeval& timeval, const std::string& topic, const std::string& message, uint8_t qoS, bool retain);

            bool empty() const;
            std::size_t size() const;

            ScheduledPublish const& top() const;
            void pop();

        private:
            MqttMapper* mqttMapper;
            std::size_t nextSeq = 0;
            std::priority_queue<ScheduledPublish, std::vector<ScheduledPublish>, EarlierFirst> minHeap;

            core::timer::Timer delayTimer;
            void processDue();
            void armDelayTimer();
        };

        DelayedQueue delayedQueue;
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_MQTTMAPPER_H

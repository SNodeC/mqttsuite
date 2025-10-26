/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
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

#ifndef MQTTBROKER_LIB_MQTTMODEL_H
#define MQTTBROKER_LIB_MQTTMODEL_H

namespace mqtt::mqttbroker::lib {
    class Mqtt;
}

namespace express {
    class Response;
}

namespace iot::mqtt::packets {
    class Publish;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>

#endif

namespace mqtt::mqttbroker::lib {

    class MqttModel {
    private:
        class MqttModelEntry {
        public:
            MqttModelEntry() = default;
            MqttModelEntry(const Mqtt* mqtt);

            const Mqtt* getMqtt() const;

            std::string onlineSince() const;
            std::string onlineDuration() const;

        private:
            const Mqtt* mqtt = nullptr;
        };

    private:
        MqttModel();

    public:
        static MqttModel& instance();

        void addClient(const std::string& clientId, Mqtt* mqtt);
        void delClient(const std::string& clientId);

        std::map<std::string, MqttModelEntry>& getClients();

        const Mqtt* getMqtt(const std::string& clientId);

        std::string onlineSince();
        std::string onlineDuration();

        void addEventReceiver(const std::shared_ptr<express::Response>& response, int lastEventId);

        void publish(const iot::mqtt::packets::Publish& publish);

    private:
        void sendEvent(const std::string& data, const std::string& event = "message", const std::string& id = "");

        static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
        static std::string
        durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                         const std::chrono::time_point<std::chrono::system_clock>& later = std::chrono::system_clock::now());

    protected:
        std::map<std::string, MqttModelEntry> modelMap;
        std::list<std::shared_ptr<express::Response>> eventReceiverList;

        std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;

        uint64_t id = 0;
    };

} // namespace mqtt::mqttbroker::lib

#endif // MQTTBROKER_LIB_MQTTMODEL_H

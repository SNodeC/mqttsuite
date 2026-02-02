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

#ifndef MQTTBRIDGE_LIB_SSEDISTRIBUTOR_H
#define MQTTBRIDGE_LIB_SSEDISTRIBUTOR_H

namespace express {
    class Response;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <chrono>
#include <core/timer/Timer.h>
#include <cstdint>
#include <list>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class SSEDistributor {
    private:
        class EventReceiver {
        public:
            EventReceiver(const std::shared_ptr<express::Response>& response);

            ~EventReceiver();

            std::weak_ptr<express::Response> response;

            bool operator==(const EventReceiver& other);

            core::timer::Timer heartbeatTimer;
        };

        SSEDistributor();

    public:
        static SSEDistributor& instance();

        SSEDistributor(const SSEDistributor&) = delete;
        SSEDistributor& operator=(const SSEDistributor&) = delete;

        SSEDistributor(SSEDistributor&&) = delete;
        SSEDistributor& operator=(SSEDistributor&&) = delete;

        ~SSEDistributor() = default;

        void addEventReceiver(const std::shared_ptr<express::Response>& response, const std::string& lastEventId);

        void bridgesStarting();
        void bridgesStarted();

        void bridgesStopping();
        void bridgesStopped();

        void bridgeDisabled(const std::string& bridgeName);
        void bridgeStarting(const std::string& bridgeName);
        void bridgeStarted(const std::string& bridgeName);

        void bridgeStopping(const std::string& bridgeName);
        void bridgeStopped(const std::string& bridgeName);

        void brokerDisabled(const std::string& bridgeName, const std::string& instanceName);
        void brokerConnecting(const std::string& bridgeName, const std::string& instanceName);
        void brokerConnected(const std::string& bridgeName, const std::string& instanceName);

        void brokerDisconnecting(const std::string& bridgeName, const std::string& instanceName);
        void brokerDisconnected(const std::string& bridgeName, const std::string& instanceName);

    private:
        static void sendEvent(const std::shared_ptr<express::Response>& response,
                              const std::string& data,
                              const std::string& event,
                              const std::string& id);

        static void sendJsonEvent(const std::shared_ptr<express::Response>& response,
                                  const nlohmann::json& json,
                                  const std::string& event = "",
                                  const std::string& id = "");
        void sendEvent(const std::string& data, const std::string& event = "", const std::string& id = "") const;
        void sendJsonEvent(const nlohmann::json& json, const std::string& event = "", const std::string& id = "") const;

        static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
        static std::string
        durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                         const std::chrono::time_point<std::chrono::system_clock>& later = std::chrono::system_clock::now());

        std::string bridgesStartedAt() const;

        std::list<EventReceiver> eventReceiverList;
        std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;
        std::chrono::time_point<std::chrono::system_clock> bridgesStartTimePoint;
        uint64_t id = 0;
    };

} // namespace mqtt::bridge::lib

#endif // MQTTBRIDGE_LIB_SSEDISTRIBUTOR_H

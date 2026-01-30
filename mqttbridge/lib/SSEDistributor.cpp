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

#include "SSEDistributor.h"

#include <ctime>
#include <express/Response.h>
#include <functional>
#include <iomanip>
#include <log/Logger.h>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <web/http/server/SocketContext.h>
struct tm;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    SSEDistributor::SSEDistributor()
        : onlineSinceTimePoint(std::chrono::system_clock::now()) {
    }

    SSEDistributor* SSEDistributor::instance() {
        static SSEDistributor sseDistributor;

        return &sseDistributor;
    }

    void SSEDistributor::addEventReceiver(const std::shared_ptr<express::Response>& response,
                                          [[maybe_unused]] const std::string& lastEventId) {
        auto& eventReceiver = eventReceiverList.emplace_back(response);

        response->getSocketContext()->onDisconnected([this, &eventReceiver]() {
            eventReceiverList.remove(eventReceiver);
        });

        sendJsonEvent({{"at", timePointToString(bridgesStartTimePoint)}}, "bridge-start", std::to_string(id++));
    }

    void SSEDistributor::sendEvent(const std::shared_ptr<express::Response>& response,
                                   const std::string& data,
                                   const std::string& event,
                                   const std::string& id) {
        if (response->isConnected()) {
            if (!event.empty()) {
                response->sendFragment("event:" + event);
            }
            if (!id.empty()) {
                response->sendFragment("id:" + id);
            }
            response->sendFragment("data:" + data);
            response->sendFragment();
        }
    }

    void SSEDistributor::sendJsonEvent(const std::shared_ptr<express::Response>& response,
                                       const nlohmann::json& json,
                                       const std::string& event,
                                       const std::string& id) {
        sendEvent(response, json.dump(), event, id);
    }

    void SSEDistributor::sendEvent(const std::string& data, const std::string& event, const std::string& id) const {
        VLOG(0) << "Server sent event: " << event << "\n" << data;

        for (auto& eventReceiver : eventReceiverList) {
            if (const auto& response = eventReceiver.response.lock()) {
                sendEvent(response, data, event, id);
            }
        }
    }

    void SSEDistributor::sendJsonEvent(const nlohmann::json& json, const std::string& event, const std::string& id) const {
        sendEvent(json.dump(), event, id);
    }

    void SSEDistributor::bridgesStarting() {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}}, "bridges_starting", std::to_string(id++));
    }

    void SSEDistributor::bridgesStarted() {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}}, "bridges_started", std::to_string(id++));
    }

    void SSEDistributor::bridgesStopping() {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}}, "bridges_stopping", std::to_string(id++));
    }

    void SSEDistributor::bridgesStopped() {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}}, "bridges_stopped", std::to_string(id++));
    }

    void SSEDistributor::bridgeDisabled(const std::string& bridgeName) {
        sendJsonEvent(
            {{"at", timePointToString(std::chrono::system_clock::now())}, {"name", bridgeName}}, "bridge_disabled", std::to_string(id++));
    }

    void SSEDistributor::bridgeStarting(const std::string& bridgeName) {
        sendJsonEvent(
            {{"at", timePointToString(std::chrono::system_clock::now())}, {"name", bridgeName}}, "bridge_starting", std::to_string(id++));
    }

    void SSEDistributor::bridgeStarted(const std::string& bridgeName) {
        sendJsonEvent(
            {{"at", timePointToString(std::chrono::system_clock::now())}, {"name", bridgeName}}, "bridge_started", std::to_string(id++));
    }

    void SSEDistributor::bridgeStopping(const std::string& bridgeName) {
        sendJsonEvent(
            {{"at", timePointToString(std::chrono::system_clock::now())}, {"name", bridgeName}}, "bridge_stopping", std::to_string(id++));
    }

    void SSEDistributor::bridgeStopped(const std::string& bridgeName) {
        sendJsonEvent(
            {{"at", timePointToString(std::chrono::system_clock::now())}, {"name", bridgeName}}, "bridge_stopped", std::to_string(id++));
    }

    void SSEDistributor::brokerDisabled(const std::string& bridgeName, const std::string& instanceName) {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}, {"bridge", bridgeName}, {"instance", instanceName}},
                      "broker_disabled",
                      std::to_string(id++));
    }

    void SSEDistributor::brokerConnecting(const std::string& bridgeName, const std::string& instanceName) {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}, {"bridge", bridgeName}, {"instance", instanceName}},
                      "broker_connecting",
                      std::to_string(id++));
    }

    void SSEDistributor::brokerConnected(const std::string& bridgeName, const std::string& instanceName) {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}, {"bridge", bridgeName}, {"instance", instanceName}},
                      "broker_connected",
                      std::to_string(id++));
    }

    void SSEDistributor::brokerDisconnecting(const std::string& bridgeName, const std::string& instanceName) {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}, {"bridge", bridgeName}, {"instance", instanceName}},
                      "broker_disconnecting",
                      std::to_string(id++));
    }

    void SSEDistributor::brokerDisconnected(const std::string& bridgeName, const std::string& instanceName) {
        sendJsonEvent({{"at", timePointToString(std::chrono::system_clock::now())}, {"bridge", bridgeName}, {"instance", instanceName}},
                      "broker_disconnected",
                      std::to_string(id++));
    }

    std::string SSEDistributor::bridgesStartedAt() const {
        return timePointToString(bridgesStartTimePoint);
    }

    std::string SSEDistributor::timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        std::tm* tm_ptr = std::gmtime(&time);

        char buffer[100];
        std::string onlineSince = "Formatting error";

        // Format: "2025-02-02 14:30:00"
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr)) {
            onlineSince = std::string(buffer) + " UTC";
        }

        return onlineSince;
    }

    std::string SSEDistributor::durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                                                 const std::chrono::time_point<std::chrono::system_clock>& later) {
        using seconds_duration_type = std::chrono::duration<std::chrono::seconds::rep>::rep;

        seconds_duration_type totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(later - bevore).count();

        // Compute days, hours, minutes, and seconds
        seconds_duration_type days = totalSeconds / 86400; // 86400 seconds in a day
        seconds_duration_type remainder = totalSeconds % 86400;
        seconds_duration_type hours = remainder / 3600;
        remainder = remainder % 3600;
        seconds_duration_type minutes = remainder / 60;
        seconds_duration_type seconds = remainder % 60;

        // Format the components into a string using stringstream
        std::ostringstream oss;
        if (days > 0) {
            oss << days << " day" << (days == 1 ? "" : "s") << ", ";
        }
        oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
            << std::setfill('0') << seconds;

        return oss.str();
    }

    SSEDistributor::EventReceiver::EventReceiver(const std::shared_ptr<express::Response>& response)
        : response(response)
        , heartbeatTimer(core::timer::Timer::intervalTimer(
              [response] {
                  response->sendFragment(":keep-alive");
                  response->sendFragment();
              },
              39)) {
    }

    SSEDistributor::EventReceiver::~EventReceiver() {
        heartbeatTimer.cancel();
    }

    bool SSEDistributor::EventReceiver::operator==(const EventReceiver& other) {
        return response.lock() == other.response.lock();
    }

} // namespace mqtt::bridge::lib

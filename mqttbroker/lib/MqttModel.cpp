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

#include "MqttModel.h"

#include "Mqtt.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cctype>
#include <core/socket/SocketAddress.h>
#include <core/socket/stream/SocketConnection.h>
#include <express/Response.h>
#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/packets/Publish.h>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <web/http/server/SocketContext.h>

//

#include <ctime>
#include <functional>
#include <iomanip>
#include <sstream>

struct tm;

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    MqttModel::MqttModel()
        : onlineSinceTimePoint(std::chrono::system_clock::now()) {
    }

    MqttModel& MqttModel::instance() {
        static MqttModel mqttModel;

        return mqttModel;
    }

    static std::string href(const std::string& text, const std::string& url, const std::string& windowId, uint16_t width, uint16_t height) {
        return "<a href=\"#\" onClick=\""
               "let key = '" +
               windowId +
               "'; "
               "if (!localStorage.getItem(key)) "
               "  localStorage.setItem(key, key + '-' + Math.random().toString(36).substr(2, 6)); "
               "let uniqueId = localStorage.getItem(key); "
               "if (!window._openWindows) window._openWindows = {}; "
               "if (!window._openWindows[uniqueId] || window._openWindows[uniqueId].closed) { "
               "  window._openWindows[uniqueId] = window.open('" +
               url + "', uniqueId, 'width=" + std::to_string(width) + ", height=" + std::to_string(height) +
               ",location=no, menubar=no, status=no, toolbar=no'); "
               "} else { "
               "  window._openWindows[uniqueId].focus(); "
               "} return false;\" "
               "style=\"color:inherit;\">" +
               text + "</a>";
    }

    void MqttModel::addClient(const std::string& clientId, Mqtt* mqtt) {
        MqttModelEntry mqttModelEntry(mqtt);

        std::ostringstream windowId("window");
        for (char ch : mqtt->getClientId()) {
            if (std::isalnum(static_cast<unsigned char>(ch))) {
                windowId << ch;
            } else {
                windowId << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                         << static_cast<int>(static_cast<unsigned char>(ch));
            }
        }

        nlohmann::json json({href(mqtt->getClientId(), "/client?" + mqtt->getClientId(), windowId.str(), 450, 900),
                             mqttModelEntry.onlineSince(),
                             "<duration>" + mqttModelEntry.onlineDuration() + "</duration>",
                             mqtt->getConnectionName(),
                             mqtt->getMqttContext()->getSocketConnection()->getLocalAddress().toString(),
                             mqtt->getMqttContext()->getSocketConnection()->getRemoteAddress().toString(),
                             "<button class=\"red-btn\" onClick=\"disconnectClient('" + mqtt->getClientId() + "')\">Disconnect</button>"});

        sendEvent(json.dump(), "connect", std::to_string(id++));

        modelMap.emplace(clientId, mqttModelEntry);
    }

    void MqttModel::delClient(const std::string& clientId) {
        sendEvent(clientId, "disconnect", std::to_string(id++));

        modelMap.erase(clientId);
    }

    std::map<std::string, MqttModel::MqttModelEntry>& MqttModel::getClients() {
        return modelMap;
    }

    const Mqtt* MqttModel::getMqtt(const std::string& clientId) {
        const Mqtt* mqtt = nullptr;

        if (modelMap.contains(clientId)) {
            mqtt = modelMap[clientId].getMqtt();
        }

        return mqtt;
    }

    std::string MqttModel::onlineSince() {
        return timePointToString(onlineSinceTimePoint);
    }

    std::string MqttModel::onlineDuration() {
        return durationToString(onlineSinceTimePoint);
    }

    void MqttModel::addEventReceiver(const std::shared_ptr<express::Response>& response, [[maybe_unused]] int lastEventId) {
        auto it = eventReceiverList.insert(eventReceiverList.end(), response);

        response->getSocketContext()->onDisconnected([this, it]() {
            eventReceiverList.erase(it);
        });
    }

    void MqttModel::publish(const iot::mqtt::packets::Publish& publish) {
        sendEvent(publish.getTopic() + " : " + publish.getMessage(), "publish", std::to_string(id++));
    }

    MqttModel::MqttModelEntry::MqttModelEntry(const Mqtt* mqtt)
        : mqtt(mqtt) {
    }

    std::string MqttModel::MqttModelEntry::onlineSince() const {
        return mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineSince();
    }

    std::string MqttModel::MqttModelEntry::onlineDuration() const {
        return mqtt->getMqttContext()->getSocketConnection()->getSocketContext()->getOnlineDuration();
    }

    const Mqtt* MqttModel::MqttModelEntry::getMqtt() const {
        return mqtt;
    }

    void MqttModel::sendEvent(const std::string& data, const std::string& event, const std::string& id) {
        for (auto& eventReceiver : eventReceiverList) {
            if (eventReceiver->isConnected()) {
                if (!event.empty()) {
                    eventReceiver->sendFragment("event:" + event);
                }
                if (!id.empty()) {
                    eventReceiver->sendFragment("id:" + id);
                }
                eventReceiver->sendFragment("data:" + data);
                eventReceiver->sendFragment("");
            }
        }
    }

    std::string MqttModel::timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
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

    std::string MqttModel::durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
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

} // namespace mqtt::mqttbroker::lib

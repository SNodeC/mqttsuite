/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MqttModel.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <ctime>
#include <iomanip>
#include <sstream>

struct tm;

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    MqttModel& MqttModel::instance() {
        static MqttModel mqttModel;

        return mqttModel;
    }

    void MqttModel::addClient(const std::string& connectionId, Mqtt* mqtt, const iot::mqtt::packets::Connect& connect) {
        modelMap[connectionId] = MqttModelEntry{.mqtt = mqtt, .connectPacket = connect, .onlineSince = std::chrono::system_clock::now()};
    }

    void MqttModel::delClient(const std::string& connectionId) {
        modelMap.erase(connectionId);
    }

    std::map<std::string, MqttModelEntry>& MqttModel::getClients() {
        return modelMap;
    }

    Mqtt* MqttModel::getMqtt(const std::string& connectionId) {
        Mqtt* mqtt = nullptr;

        if (modelMap.contains(connectionId)) {
            mqtt = modelMap[connectionId].mqtt;
        }

        return mqtt;
    }

    const std::string MqttModel::onlineSince(const MqttModelEntry& mqttModelEntry) {
        std::time_t time = std::chrono::system_clock::to_time_t(mqttModelEntry.onlineSince);
        std::tm* tm_ptr = std::gmtime(&time);

        char buffer[100];
        std::string onlineSince = "Formatting error";

        // Format: "2025-02-02 14:30:00"
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr)) {
            onlineSince = std::string(buffer) + " UTC";
        }

        return onlineSince;
    }

    const std::string MqttModel::onlineDuration(const MqttModelEntry& mqttModelEntry) {
        using seconds_duration_type = std::chrono::duration<std::chrono::seconds::rep>::rep;

        seconds_duration_type totalSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - mqttModelEntry.onlineSince).count();

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

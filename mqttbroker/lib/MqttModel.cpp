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

namespace mqtt::mqttbroker::lib {

    MqttModel& MqttModel::instance() {
        static MqttModel mqttModel;

        return mqttModel;
    }

    void MqttModel::addClient(const std::string& connectionId, Mqtt* mqtt, const iot::mqtt::packets::Connect& connect) {
        modelMap[connectionId] = MqttModelEntry{.mqtt = mqtt, .connectPacket = connect};
    }

    void MqttModel::delClient(const std::string& connectionId) {
        modelMap.erase(connectionId);
    }

    std::map<std::string, MqttModelEntry> &MqttModel::getClients() {
        return modelMap;
    }

} // namespace mqtt::mqttbroker::lib

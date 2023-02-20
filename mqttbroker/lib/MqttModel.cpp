/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

    void MqttModel::addConnectedClient(mqtt::mqttbroker::lib::Mqtt* mqtt, const iot::mqtt::packets::Connect& connect) {
        connectedClients[mqtt] = connect;
    }

    void MqttModel::delDisconnectedClient(mqtt::mqttbroker::lib::Mqtt* mqtt) {
        connectedClients.erase(mqtt);
    }

    const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& MqttModel::getConnectedClinets() const {
        return connectedClients;
    }

} // namespace mqtt::mqttbroker::lib

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

#ifndef MQTTBROKER_LIB_MQTTMODEL_H
#define MQTTBROKER_LIB_MQTTMODEL_H

namespace mqtt::mqttbroker::lib {
    class Mqtt;
}

#include <iot/mqtt/packets/Connect.h> // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <chrono> // IWYU pragma: export
#include <map>
#include <string>

#endif

namespace mqtt::mqttbroker::lib {

    class MqttModel {
    private:
        class MqttModelEntry {
        public:
            MqttModelEntry() = default;
            MqttModelEntry(const Mqtt* mqtt, const iot::mqtt::packets::Connect& connect);

            const std::string onlineSince() const;
            const std::string onlineDuration() const;

            const Mqtt* getMqtt() const;

            const iot::mqtt::packets::Connect& getConnectPacket() const;

        private:
            const Mqtt* mqtt = nullptr;
            iot::mqtt::packets::Connect connectPacket;
            std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;
        };

    private:
        MqttModel() = default;

    public:
        static MqttModel& instance();

        void addClient(const std::string& connectionId, Mqtt* mqtt, const iot::mqtt::packets::Connect& connect);
        void delClient(const std::string& connectionId);

        std::map<std::string, MqttModelEntry>& getClients();

        const Mqtt* getMqtt(const std::string& connectionId);

    protected:
        std::map<std::string, MqttModelEntry> modelMap;
    };

} // namespace mqtt::mqttbroker::lib

#endif // MQTTBROKER_LIB_MQTTMODEL_H

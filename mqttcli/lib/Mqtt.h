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

#ifndef APPS_MQTTBROKER_MQTT_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTT_SOCKETCONTEXT_H

#include <database/mariadb/MariaDBClient.h>
#include <iot/mqtt/client/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>

#endif

namespace mqtt::mqttcli::lib {

    class Mqtt : public iot::mqtt::client::Mqtt {
    public:
        explicit Mqtt(const std::string& connectionName,
                      const std::string& clientId,
                      uint8_t qoSDefault,
                      uint16_t keepAlive,
                      bool cleanSession,
                      const std::string& willTopic,
                      const std::string& willMessage,
                      uint8_t willQoS,
                      bool willRetain,
                      const std::string& username,
                      const std::string& password,
                      const std::list<std::string>& subTopics,
                      const std::string& pubTopic,
                      const std::string& pubMessage,
                      bool pubRetain,
                      const std::string& database,
                      const std::string& usernameDb,
                      const std::string& passwordDb,
                      const std::string& host,
                      uint16_t port,
                      const std::string& socket,
                      uint32_t flags,
                      const std::string& sessionStoreFileName = "");

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onSuback(const iot::mqtt::packets::Suback& suback) final;
        void onPuback(const iot::mqtt::packets::Puback& puback) final;
        void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) final;

        void recreateMariaDBClient();
        void execMariaDB(const std::string& sql,
                         const std::function<void(void)>& onExec,
                         const std::function<void(const std::string&, unsigned int)>& onError,
                         bool retryOnConnectionLoss = true);

        database::mariadb::MariaDBConnectionDetails mariaDBConnectionDetails;
        std::unique_ptr<database::mariadb::MariaDBClient> mariaDB;

        const uint8_t qoSDefault;
        const bool cleanSession;

        const std::string willTopic;
        const std::string willMessage;
        const uint8_t willQoS;
        const bool willRetain;
        const std::string username;
        const std::string password;

        const std::list<std::string> subTopics;
        const std::string pubTopic;
        const std::string pubMessage;
        const bool pubRetain;
    };

} // namespace mqtt::mqttcli::lib

#endif // APPS_MQTTBROKER_MQTTSUB_SOCKETCONTEXT_H

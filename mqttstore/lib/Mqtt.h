/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#ifndef MQTTSTORE_LIB_MQTT_H
#define MQTTSTORE_LIB_MQTT_H

#include "MariaDbRawStore.h"

#include <iot/mqtt/client/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

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
                      const MariaDbRawStore::DatabaseConfig& databaseConfig,
                      const MariaDbRawStore::StorageConfig& storageConfig,
                      const std::string& sessionStoreFileName = "");

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onSuback(const iot::mqtt::packets::Suback& suback) final;

        [[nodiscard]] RawMessage makeRawMessage(const iot::mqtt::packets::Publish& publish) const;

        MariaDbRawStore rawStore;

        const std::string clientId;
        const uint8_t qoSDefault;
        const bool cleanSession;
        const std::string willTopic;
        const std::string willMessage;
        const uint8_t willQoS;
        const bool willRetain;
        const std::string username;
        const std::string password;
        const std::list<std::string> subTopics;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MQTT_H

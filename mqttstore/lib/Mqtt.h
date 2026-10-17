/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#ifndef MQTTSTORE_LIB_MQTT_H
#define MQTTSTORE_LIB_MQTT_H

#include "MariaDbStorage.h"

namespace mqtt::mqttstore::lib {
    class StoragePlan;
}

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
                      std::uint8_t qoSDefault,
                      std::uint16_t keepAlive,
                      bool cleanSession,
                      const std::string& willTopic,
                      const std::string& willMessage,
                      std::uint8_t willQoS,
                      bool willRetain,
                      const std::string& username,
                      const std::string& password,
                      const std::list<std::string>& subTopics,
                      MariaDbStorage::ConnectionConfig connectionConfig,
                      const std::string& rawTable,
                      bool autoCreateRawTable,
                      StoragePlan storagePlan,
                      const std::string& sessionStoreFileName = "");

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onSuback(const iot::mqtt::packets::Suback& suback) final;

        [[nodiscard]] static std::uint8_t getQos(const std::string& qoSString);

        MariaDbStorage storage;
        const std::uint8_t qoSDefault;
        const bool cleanSession;
        const std::string willTopic;
        const std::string willMessage;
        const std::uint8_t willQoS;
        const bool willRetain;
        const std::string username;
        const std::string password;
        const std::list<std::string> subTopics;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MQTT_H

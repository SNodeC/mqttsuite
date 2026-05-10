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

#include "Storage.h"

#include <iot/mqtt/client/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
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
                      const std::list<std::string>& storeTopics,
                      const std::string& database,
                      const std::string& usernameDb,
                      const std::string& passwordDb,
                      const std::string& host,
                      uint16_t port,
                      const std::string& socket,
                      uint32_t flags,
                      const std::string& rawTable,
                      const std::string& fieldTable,
                      bool autoCreate,
                      bool flattenJson,
                      std::size_t maxPayloadBytes,
                      const std::string& sessionStoreFileName = "");

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onSuback(const iot::mqtt::packets::Suback& suback) final;

        [[nodiscard]] static uint8_t getQoS(const std::string& qoSString);

        MariaDbStorage storage;

        const uint8_t qoSDefault;
        const bool cleanSession;
        const std::string willTopic;
        const std::string willMessage;
        const uint8_t willQoS;
        const bool willRetain;
        const std::string username;
        const std::string password;
        const std::list<std::string> storeTopics;
        const std::size_t maxPayloadBytes;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MQTT_H

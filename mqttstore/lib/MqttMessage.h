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

#ifndef MQTTSTORE_LIB_MQTTMESSAGE_H
#define MQTTSTORE_LIB_MQTTMESSAGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

    struct MqttMessage {
        std::string messageId;
        std::string connectionName;
        std::string topic;
        std::string payload;
        uint8_t qoS = 0;
        bool retain = false;
        bool dup = false;
        uint16_t packetIdentifier = 0;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_MQTTMESSAGE_H

/*
 * MQTTSuite - A lightweight MQTT Integration System
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#ifndef MQTTSTORE_LIB_RAWMESSAGE_H
#define MQTTSTORE_LIB_RAWMESSAGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif

namespace mqtt::mqttstore::lib {

    struct RawMessage {
        std::string sourceInstance;
        std::string clientId;
        std::string topic;
        std::string payload;
        uint8_t qoS = 0;
        bool retain = false;
        bool dup = false;
        uint16_t packetIdentifier = 0;
        bool payloadIsJson = false;
    };

} // namespace mqtt::mqttstore::lib

#endif // MQTTSTORE_LIB_RAWMESSAGE_H

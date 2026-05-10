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

#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>
#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Suback.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <iterator>
#include <log/Logger.h>
#include <stdexcept>
#include <string>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttstore::lib {

    Mqtt::Mqtt(const std::string& connectionName,
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
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, keepAlive, sessionStoreFileName)
        , storage(connectionName,
                  database,
                  usernameDb,
                  passwordDb,
                  host,
                  port,
                  socket,
                  flags,
                  rawTable,
                  fieldTable,
                  autoCreate,
                  flattenJson,
                  maxPayloadBytes)
        , qoSDefault(qoSDefault)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password)
        , storeTopics(storeTopics)
        , maxPayloadBytes(maxPayloadBytes) {
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "  Keep Alive: " << keepAlive;
        VLOG(1) << "  Clean Session: " << cleanSession;
        VLOG(1) << "  Will Topic: " << willTopic;
        VLOG(1) << "  Will QoS: " << static_cast<uint16_t>(willQoS);
        VLOG(1) << "  Will Retain " << willRetain;
        VLOG(1) << "  Username: " << username;
        VLOG(1) << "  Store Topics: " << storeTopics.size();
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating store session";

        storage.ensureSchema();
        sendConnect(cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();
        return Super::onSignal(signum);
    }

    uint8_t Mqtt::getQoS(const std::string& qoSString) {
        unsigned long qoS = std::stoul(qoSString);

        if (qoS > 2) {
            throw std::out_of_range("qos " + qoSString + " not in range [0..2]");
        }

        return static_cast<uint8_t>(qoS);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() != 0) {
            sendDisconnect();
            return;
        }

        try {
            std::list<iot::mqtt::Topic> topicList;
            std::transform(storeTopics.begin(),
                           storeTopics.end(),
                           std::back_inserter(topicList),
                           [qoSDefault = this->qoSDefault](const std::string& compositeTopic) -> iot::mqtt::Topic {
                               std::size_t pos = compositeTopic.rfind("##");
                               const std::string topic = compositeTopic.substr(0, pos);
                               uint8_t qoS = qoSDefault;

                               if (pos != std::string::npos) {
                                   try {
                                       qoS = getQoS(compositeTopic.substr(pos + 2));
                                   } catch (const std::logic_error& error) {
                                       VLOG(0) << "MQTT Store malformed composite topic: " << compositeTopic << " : " << error.what();
                                       throw;
                                   }
                               }

                               VLOG(0) << "MQTT Store Subscribe: " << topic << " | QoS " << static_cast<int>(qoS);
                               return iot::mqtt::Topic(topic, qoS);
                           });

            sendSubscribe(topicList);
        } catch (const std::logic_error& error) {
            VLOG(0) << "MQTT Store subscription failed: " << error.what();
            sendDisconnect();
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        VLOG(0) << "MQTT Store Publish: " << publish.getTopic() << " | QoS " << static_cast<uint16_t>(publish.getQoS()) << " | Retain "
                << (publish.getRetain() != 0 ? "true" : "false") << " | Dup " << (publish.getDup() != 0 ? "true" : "false") << " | Bytes "
                << publish.getMessage().size();

        storage.store(makeEnvelope(connectionName, publish, maxPayloadBytes));
    }

    void Mqtt::onSuback(const iot::mqtt::packets::Suback& suback) {
        VLOG(1) << "MQTT Store Suback";

        for (auto returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  r: " << static_cast<int>(returnCode);
        }
    }

} // namespace mqtt::mqttstore::lib

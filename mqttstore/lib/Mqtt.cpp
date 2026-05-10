/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *               Tobias Pfeil
 *               2025, 2026
 *
 * SPDX-License-Identifier: MIT OR GPL-3.0-or-later
 */

#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>
#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Suback.h>
#include <nlohmann/json.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <iterator>
#include <log/Logger.h>
#include <stdexcept>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttstore::lib {

    namespace {

        [[nodiscard]] uint8_t getQos(const std::string& qoSString) {
            const unsigned long qoS = std::stoul(qoSString);

            if (qoS > 2) {
                throw std::out_of_range("qos " + qoSString + " not in range [0..2]");
            }

            return static_cast<uint8_t>(qoS);
        }

        [[nodiscard]] bool isJsonPayload(const std::string& payload) {
            try {
                static_cast<void>(nlohmann::json::parse(payload));
                return true;
            } catch (const nlohmann::json::parse_error&) {
                return false;
            }
        }

    } // namespace

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
               const std::list<std::string>& subTopics,
               const MariaDbRawStore::DatabaseConfig& databaseConfig,
               const MariaDbRawStore::StorageConfig& storageConfig,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, keepAlive, sessionStoreFileName)
        , rawStore(databaseConfig, storageConfig)
        , clientId(clientId)
        , qoSDefault(qoSDefault)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password)
        , subTopics(subTopics) {
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "  Keep Alive: " << keepAlive;
        VLOG(1) << "  Clean Session: " << cleanSession;
        VLOG(1) << "  Will Topic: " << willTopic;
        VLOG(1) << "  Will Message: " << willMessage;
        VLOG(1) << "  Will QoS: " << static_cast<uint16_t>(willQoS);
        VLOG(1) << "  Will Retain " << willRetain;
        VLOG(1) << "  Username: " << username;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: initiating storage session";
        sendConnect(cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: on exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";
        sendDisconnect();
        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() != 0) {
            sendDisconnect();
            return;
        }

        rawStore.ensureSchema();

        try {
            std::list<iot::mqtt::Topic> topicList;
            std::transform(subTopics.begin(),
                           subTopics.end(),
                           std::back_inserter(topicList),
                           [qoSDefault = this->qoSDefault](const std::string& compositeTopic) -> iot::mqtt::Topic {
                               const std::size_t pos = compositeTopic.rfind("##");
                               const std::string topic = compositeTopic.substr(0, pos);
                               uint8_t qoS = qoSDefault;

                               if (pos != std::string::npos) {
                                   qoS = getQos(compositeTopic.substr(pos + 2));
                               }

                               VLOG(0) << "  subscribe: " << static_cast<int>(qoS) << " | " << topic;
                               return iot::mqtt::Topic(topic, qoS);
                           });

            sendSubscribe(topicList);
        } catch (const std::logic_error& error) {
            VLOG(0) << "MQTT: subscription configuration failed: " << error.what();
            sendDisconnect();
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        VLOG(0) << "MQTT Store Publish: " << publish.getTopic() << " | QoS: " << static_cast<uint16_t>(publish.getQoS())
                << " | Retain: " << (publish.getRetain() != 0 ? "true" : "false")
                << " | Dup: " << (publish.getDup() != 0 ? "true" : "false") << " | Bytes: " << publish.getMessage().size();

        rawStore.store(makeRawMessage(publish));
    }

    void Mqtt::onSuback(const iot::mqtt::packets::Suback& suback) {
        VLOG(1) << "MQTT Suback";

        for (auto returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  r: " << static_cast<int>(returnCode);
        }
    }

    RawMessage Mqtt::makeRawMessage(const iot::mqtt::packets::Publish& publish) const {
        return RawMessage{.sourceInstance = connectionName,
                          .clientId = clientId,
                          .topic = publish.getTopic(),
                          .payload = publish.getMessage(),
                          .qoS = publish.getQoS(),
                          .retain = publish.getRetain() != 0,
                          .dup = publish.getDup() != 0,
                          .packetIdentifier = publish.getPacketIdentifier(),
                          .payloadIsJson = isJsonPayload(publish.getMessage())};
    }

} // namespace mqtt::mqttstore::lib

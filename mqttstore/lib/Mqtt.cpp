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
#include <utility>
#include <utils/system/signal.h>

#endif

namespace mqtt::mqttstore::lib {

    Mqtt::Mqtt(const std::string& connectionName,
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
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, keepAlive, sessionStoreFileName)
        , storage(connectionName, connectionConfig, rawTable, autoCreateRawTable, std::move(storagePlan))
        , qoSDefault(qoSDefault)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password)
        , subTopics(subTopics) {
        VLOG(1) << "MQTTStore client id: " << clientId;
        VLOG(1) << "  Keep Alive: " << keepAlive;
        VLOG(1) << "  Clean Session: " << cleanSession;
        VLOG(1) << "  Will Topic: " << willTopic;
        VLOG(1) << "  Will QoS: " << static_cast<std::uint16_t>(willQoS);
        VLOG(1) << "  Will Retain: " << willRetain;
        VLOG(1) << "  Username configured: " << (!username.empty());
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTTStore: initiating MQTT session";
        sendConnect(cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTTStore: exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";
        sendDisconnect();

        return Super::onSignal(signum);
    }

    std::uint8_t Mqtt::getQos(const std::string& qoSString) {
        const unsigned long qoS = std::stoul(qoSString);

        if (qoS > 2) {
            throw std::out_of_range("qos " + qoSString + " not in range [0..2]");
        }

        return static_cast<std::uint8_t>(qoS);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() != 0) {
            VLOG(0) << connectionName << " MQTTStore: broker rejected connection with return code "
                    << static_cast<int>(connack.getReturnCode());
            sendDisconnect();
            return;
        }

        if (subTopics.empty()) {
            VLOG(0) << connectionName << " MQTTStore: no subscriptions configured";
            sendDisconnect();
            return;
        }

        try {
            std::list<iot::mqtt::Topic> topicList;
            std::transform(subTopics.begin(),
                           subTopics.end(),
                           std::back_inserter(topicList),
                           [qoSDefault = this->qoSDefault](const std::string& compositeTopic) {
                               const std::size_t pos = compositeTopic.rfind("##");
                               const std::string topic = compositeTopic.substr(0, pos);
                               std::uint8_t qoS = qoSDefault;

                               if (pos != std::string::npos) {
                                   qoS = getQos(compositeTopic.substr(pos + 2));
                               }

                               VLOG(0) << "MQTTStore subscribe: QoS " << static_cast<int>(qoS) << " | " << topic;
                               return iot::mqtt::Topic(topic, qoS);
                           });

            sendSubscribe(topicList);
        } catch (const std::logic_error& error) {
            VLOG(0) << connectionName << " MQTTStore subscription failed: " << error.what();
            sendDisconnect();
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        VLOG(1) << connectionName << " MQTTStore received publish: topic='" << publish.getTopic()
                << "' qos=" << static_cast<std::uint16_t>(publish.getQoS()) << " retain=" << (publish.getRetain() != 0)
                << " dup=" << (publish.getDup() != 0);

        storage.store({.connectionName = connectionName,
                       .topic = publish.getTopic(),
                       .payload = publish.getMessage(),
                       .qoS = publish.getQoS(),
                       .retain = publish.getRetain() != 0,
                       .dup = publish.getDup() != 0,
                       .packetIdentifier = publish.getPacketIdentifier()});
    }

    void Mqtt::onSuback(const iot::mqtt::packets::Suback& suback) {
        VLOG(1) << "MQTTStore Suback";

        for (auto returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  r: " << static_cast<int>(returnCode);
        }
    }

} // namespace mqtt::mqttstore::lib

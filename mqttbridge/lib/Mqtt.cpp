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

#include "lib/BridgeStore.h"

#include <iot/mqtt/packets/Connack.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "lib/SemanticLog.h"

#include <cstdint>
#include <cstring>
#include <utils/system/signal.h>

#endif

namespace mqtt::bridge::lib {

    Mqtt::Mqtt(const std::string& connectionName, Broker& broker)
        : iot::mqtt::client::Mqtt(connectionName,
                                  broker.getClientId(),
                                  broker.getKeepAlive(),
                                  broker.getSessionStoreFileName())
        , broker(broker) {
        mqttsuite::semantic::bridgeLog().debug() << "Client Id: " << clientId;
        mqttsuite::semantic::bridgeLog().debug() << "  Keep Alive: " << keepAlive;
        mqttsuite::semantic::bridgeLog().debug() << "  Prefix: " << broker.getPrefix();
        mqttsuite::semantic::bridgeLog().debug() << "  Clean Session: " << broker.getCleanSession();
        mqttsuite::semantic::bridgeLog().debug() << "  Will Topic: " << broker.getWillTopic();
        mqttsuite::semantic::bridgeLog().debug() << "  Will Message: " << broker.getWillMessage();
        mqttsuite::semantic::bridgeLog().debug() << "  Will QoS: " << static_cast<uint16_t>(broker.getWillQoS());
        mqttsuite::semantic::bridgeLog().debug() << "  Will Retain " << broker.getWillRetain();
        mqttsuite::semantic::bridgeLog().debug() << "  Username: " << broker.getUsername();
        mqttsuite::semantic::bridgeLog().debug() << "  Password configured: " << !broker.getPassword().empty();
        mqttsuite::semantic::bridgeLog().debug() << "  Loop Prevention: " << broker.getLoopPrevention();
    }

    const Broker& Mqtt::getBroker() const {
        return broker;
    }

    void Mqtt::onConnected() {
        mqttsuite::semantic::bridgeLog().debug() << "MQTT: Initiating Session";

        sendConnect(broker.getCleanSession(),
                    broker.getWillTopic(),
                    broker.getWillMessage(),
                    broker.getWillQoS(),
                    broker.getWillRetain(),
                    broker.getUsername(),
                    broker.getPassword(),
                    broker.getLoopPrevention());
    }

    void Mqtt::onDisconnected() {
        mqtt::bridge::lib::BridgeStore::instance().mqttDisconnected(broker, this);
        mqttsuite::semantic::bridgeLog().debug() << "MQTT: Disconnected";
    }

    bool Mqtt::onSignal(int signum) {
        mqttsuite::semantic::bridgeLog().debug()
            << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum << ")";

        sendDisconnect();
        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0) {
            mqtt::bridge::lib::BridgeStore::instance().mqttConnected(broker, this);
            sendSubscribe(broker.getTopics());
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        broker.getBridge().publish(this, publish);
    }

} // namespace mqtt::bridge::lib

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

#include "SocketContextFactory.h"

#include "lib/ConfigSections.h"
#include "lib/Mqtt.h"
#include "lib/StoragePlan.h"

#include <core/socket/stream/SocketConnection.h>
#include <iot/mqtt/SocketContext.h>
#include <net/config/ConfigInstance.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif

namespace {

    struct StorageOptions {
        std::string rawTable = "mqtt_messages";
        bool autoCreateRawTable = true;
        std::string projectionFile;
    };

    StorageOptions getStorageOptions(const mqtt::mqttstore::lib::ConfigStorage* configStorage) {
        if (configStorage == nullptr) {
            return {};
        }

        return {.rawTable = configStorage->getRawTable(),
                .autoCreateRawTable = configStorage->getAutoCreateRawTable(),
                .projectionFile = configStorage->getProjectionFile()};
    }

} // namespace

namespace mqtt::mqttstore {

    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        const net::config::ConfigInstance* configInstance = socketConnection->getConfigInstance();

        const lib::ConfigSession* configSession = configInstance->getSubCommand<lib::ConfigSession>();
        const lib::ConfigSubscribe* configSubscribe = configInstance->getSubCommand<lib::ConfigSubscribe>();
        const lib::ConfigDatabase* configDatabase = configInstance->getSubCommand<lib::ConfigDatabase>();
        const lib::ConfigStorage* configStorage = configDatabase->getSubCommand<lib::ConfigStorage>();
        const StorageOptions storageOptions = getStorageOptions(configStorage);

        return new iot::mqtt::SocketContext(socketConnection,
                                            new mqtt::mqttstore::lib::Mqtt(socketConnection->getConnectionName(),
                                                                           configSession->getClientId(),
                                                                           configSession->getQoS(),
                                                                           configSession->getKeepAlive(),
                                                                           !configSession->getRetainSession(),
                                                                           configSession->getWillTopic(),
                                                                           configSession->getWillMessage(),
                                                                           configSession->getWillQoS(),
                                                                           configSession->getWillRetain(),
                                                                           configSession->getUsername(),
                                                                           configSession->getPassword(),
                                                                           configSubscribe->getTopic(),
                                                                           {.database = configDatabase->getDatabase(),
                                                                            .username = configDatabase->getUsername(),
                                                                            .password = configDatabase->getPassword(),
                                                                            .host = configDatabase->getHost(),
                                                                            .port = configDatabase->getPort(),
                                                                            .socket = configDatabase->getSocket(),
                                                                            .flags = configDatabase->getFlags()},
                                                                           storageOptions.rawTable,
                                                                           storageOptions.autoCreateRawTable,
                                                                           lib::StoragePlan::fromFile(storageOptions.projectionFile),
                                                                           configSession->getSessionStore()));
    }

} // namespace mqtt::mqttstore

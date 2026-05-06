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

#ifndef MQTTCLI_LIB_TTNUPLINK_H
#define MQTTCLI_LIB_TTNUPLINK_H

#include <nlohmann/json_fwd.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <optional>
#include <string>

#endif

namespace mqtt::mqttcli::lib {

    struct ScalarMeasurement {
        std::string applicationId;
        std::string deviceId;
        std::string receivedAt;
        int fPort = 0;
        std::string fieldName;
        double value = 0.0;
    };

    struct GpsPosition {
        std::string applicationId;
        std::string deviceId;
        std::string receivedAt;
        double latitude = 0.0;
        double longitude = 0.0;
        std::optional<double> altitude;
        std::optional<double> hdop;
    };

    struct TtnUplink {
        std::string applicationId;
        std::string deviceId;
        std::string devEui;
        std::string receivedAt;
        int fPort = 0;
        std::uint64_t frameCounter = 0;
        std::optional<ScalarMeasurement> scalarMeasurement;
        std::optional<GpsPosition> gpsPosition;
    };

    [[nodiscard]] std::optional<TtnUplink> parseTtnUplink(const nlohmann::json& message, std::string& errorMessage);
    [[nodiscard]] nlohmann::json toJson(const ScalarMeasurement& measurement);
    [[nodiscard]] nlohmann::json toJson(const GpsPosition& position);
    [[nodiscard]] nlohmann::json toJson(const TtnUplink& uplink);
    [[nodiscard]] std::string toMariaDbTimestamp(const std::string& receivedAt);
    [[nodiscard]] std::string sqlQuote(const std::string& value);
    [[nodiscard]] std::string sqlNullableDouble(const std::optional<double>& value);

} // namespace mqtt::mqttcli::lib

#endif // MQTTCLI_LIB_TTNUPLINK_H

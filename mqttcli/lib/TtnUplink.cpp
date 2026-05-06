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

#include "TtnUplink.h"

#include <nlohmann/json.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <sstream>
#include <stdexcept>

#endif

namespace mqtt::mqttcli::lib {

    namespace {

        [[nodiscard]] bool getNumber(const nlohmann::json& json, const std::string& key, double& value) {
            if (!json.contains(key) || !json.at(key).is_number()) {
                return false;
            }

            value = json.at(key).get<double>();

            return true;
        }

        [[nodiscard]] std::optional<double> getOptionalNumber(const nlohmann::json& json, const std::string& key) {
            if (!json.contains(key) || json.at(key).is_null()) {
                return std::nullopt;
            }

            if (!json.at(key).is_number()) {
                return std::nullopt;
            }

            return json.at(key).get<double>();
        }

        [[nodiscard]] std::optional<std::string> scalarFieldForFPort(int fPort) {
            switch (fPort) {
                case 2:
                    return "temperature_c";
                case 3:
                    return "ph_level";
                case 4:
                    return "tds_ppm";
                case 5:
                    return "turbidity_ntu";
                case 6:
                    return "ph_board_temperature_c";
                default:
                    return std::nullopt;
            }
        }

    } // namespace

    std::optional<TtnUplink> parseTtnUplink(const nlohmann::json& message, std::string& errorMessage) {
        try {
            const nlohmann::json& endDeviceIds = message.at("end_device_ids");
            const nlohmann::json& uplinkMessage = message.at("uplink_message");
            const nlohmann::json& decodedPayload = uplinkMessage.at("decoded_payload");

            TtnUplink uplink;
            uplink.applicationId = endDeviceIds.at("application_ids").at("application_id").get<std::string>();
            uplink.deviceId = endDeviceIds.at("device_id").get<std::string>();
            uplink.devEui = endDeviceIds.value("dev_eui", "");
            uplink.receivedAt = message.value("received_at", "");
            uplink.fPort = uplinkMessage.at("f_port").get<int>();
            uplink.frameCounter = uplinkMessage.value("f_cnt", 0ULL);

            if (uplink.fPort == 1) {
                double latitude = 0.0;
                double longitude = 0.0;

                if (!getNumber(decodedPayload, "latitude", latitude) || !getNumber(decodedPayload, "longitude", longitude)) {
                    errorMessage = "GPS uplink missing numeric latitude or longitude";
                    return std::nullopt;
                }

                uplink.gpsPosition = GpsPosition{.applicationId = uplink.applicationId,
                                                 .deviceId = uplink.deviceId,
                                                 .receivedAt = uplink.receivedAt,
                                                 .latitude = latitude,
                                                 .longitude = longitude,
                                                 .altitude = getOptionalNumber(decodedPayload, "altitude"),
                                                 .hdop = getOptionalNumber(decodedPayload, "hdop")};
            } else if (const std::optional<std::string> fieldName = scalarFieldForFPort(uplink.fPort); fieldName.has_value()) {
                double value = 0.0;

                if (!getNumber(decodedPayload, *fieldName, value)) {
                    errorMessage = "Scalar uplink missing numeric decoded_payload." + *fieldName;
                    return std::nullopt;
                }

                uplink.scalarMeasurement = ScalarMeasurement{.applicationId = uplink.applicationId,
                                                             .deviceId = uplink.deviceId,
                                                             .receivedAt = uplink.receivedAt,
                                                             .fPort = uplink.fPort,
                                                             .fieldName = *fieldName,
                                                             .value = value};
            } else {
                errorMessage = "Unsupported fPort " + std::to_string(uplink.fPort);
                return std::nullopt;
            }

            return uplink;
        } catch (const nlohmann::json::exception& error) {
            errorMessage = error.what();
        } catch (const std::exception& error) {
            errorMessage = error.what();
        }

        return std::nullopt;
    }

    nlohmann::json toJson(const ScalarMeasurement& measurement) {
        return {{"application_id", measurement.applicationId},
                {"device_id", measurement.deviceId},
                {"received_at", measurement.receivedAt},
                {"f_port", measurement.fPort},
                {"field", measurement.fieldName},
                {"value", measurement.value}};
    }

    nlohmann::json toJson(const GpsPosition& position) {
        nlohmann::json json = {{"application_id", position.applicationId},
                               {"device_id", position.deviceId},
                               {"received_at", position.receivedAt},
                               {"latitude", position.latitude},
                               {"longitude", position.longitude}};

        json["altitude"] = position.altitude.has_value() ? nlohmann::json(*position.altitude) : nlohmann::json(nullptr);
        json["hdop"] = position.hdop.has_value() ? nlohmann::json(*position.hdop) : nlohmann::json(nullptr);

        return json;
    }

    nlohmann::json toJson(const TtnUplink& uplink) {
        nlohmann::json json = {{"application_id", uplink.applicationId},
                               {"device_id", uplink.deviceId},
                               {"dev_eui", uplink.devEui},
                               {"received_at", uplink.receivedAt},
                               {"f_port", uplink.fPort},
                               {"f_cnt", uplink.frameCounter}};

        if (uplink.scalarMeasurement.has_value()) {
            json["measurement"] = toJson(*uplink.scalarMeasurement);
        }

        if (uplink.gpsPosition.has_value()) {
            json["gps"] = toJson(*uplink.gpsPosition);
        }

        return json;
    }

    std::string toMariaDbTimestamp(const std::string& receivedAt) {
        if (receivedAt.size() < 19) {
            return receivedAt;
        }

        std::string timestamp = receivedAt.substr(0, 19);
        std::replace(timestamp.begin(), timestamp.end(), 'T', ' ');

        return timestamp;
    }

    std::string sqlQuote(const std::string& value) {
        std::string quoted;
        quoted.reserve(value.size() + 2);
        quoted.push_back('\'');

        for (const char character : value) {
            switch (character) {
                case '\0':
                    quoted += "\\0";
                    break;
                case '\n':
                    quoted += "\\n";
                    break;
                case '\r':
                    quoted += "\\r";
                    break;
                case '\\':
                    quoted += "\\\\";
                    break;
                case '\'':
                    quoted += "\\'";
                    break;
                case '"':
                    quoted += "\\\"";
                    break;
                case '\x1a':
                    quoted += "\\Z";
                    break;
                default:
                    quoted.push_back(character);
                    break;
            }
        }

        quoted.push_back('\'');

        return quoted;
    }

    std::string sqlNullableDouble(const std::optional<double>& value) {
        if (!value.has_value()) {
            return "NULL";
        }

        std::ostringstream valueStream;
        valueStream.precision(17);
        valueStream << *value;

        return valueStream.str();
    }

} // namespace mqtt::mqttcli::lib

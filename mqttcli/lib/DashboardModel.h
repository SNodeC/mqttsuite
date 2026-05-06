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

#ifndef MQTTCLI_LIB_DASHBOARDMODEL_H
#define MQTTCLI_LIB_DASHBOARDMODEL_H

#include "TtnUplink.h"

#include <core/timer/Timer.h>

namespace express {
    class Response;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

#endif

namespace mqtt::mqttcli::lib {

    class DashboardModel {
    private:
        class EventReceiver {
        public:
            EventReceiver(std::uint64_t id, const std::shared_ptr<express::Response>& response);
            ~EventReceiver();

            [[nodiscard]] std::uint64_t getId() const {
                return id;
            }

        private:
            std::uint64_t id;
            std::weak_ptr<express::Response> response;
            core::timer::Timer heartbeatTimer;

            friend class DashboardModel;
        };

    private:
        DashboardModel();

    public:
        static DashboardModel& instance();

        void addEventReceiver(const std::shared_ptr<express::Response>& response, const std::string& lastEventId);
        void publishScalarMeasurement(const ScalarMeasurement& measurement);
        void publishGpsPosition(const GpsPosition& position);
        void publishStorageError(const std::string& message, const std::optional<TtnUplink>& uplink = std::nullopt);

    private:
        static void sendEvent(const std::shared_ptr<express::Response>& response,
                              const std::string& data,
                              const std::string& event,
                              const std::string& id);
        static void sendJsonEvent(const std::shared_ptr<express::Response>& response,
                                  const nlohmann::json& json,
                                  const std::string& event = "",
                                  const std::string& id = "");
        void sendJsonEvent(const nlohmann::json& json, const std::string& event = "") const;

        std::list<EventReceiver> eventReceiverList;
        std::uint64_t nextEventReceiverId = 0;
        std::uint64_t nextEventId = 0;
        std::optional<ScalarMeasurement> latestTemperature;
        std::optional<ScalarMeasurement> latestPh;
        std::optional<ScalarMeasurement> latestTds;
        std::optional<ScalarMeasurement> latestTurbidity;
        std::optional<ScalarMeasurement> latestBoardTemperature;
        std::optional<GpsPosition> latestGps;
    };

} // namespace mqtt::mqttcli::lib

#endif // MQTTCLI_LIB_DASHBOARDMODEL_H

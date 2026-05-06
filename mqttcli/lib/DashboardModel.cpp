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

#include "DashboardModel.h"

#include <express/Response.h>
#include <nlohmann/json.hpp>
#include <web/http/server/SocketContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <log/Logger.h>
#include <utility>

#endif

namespace mqtt::mqttcli::lib {

    DashboardModel::EventReceiver::EventReceiver(std::uint64_t id, const std::shared_ptr<express::Response>& response)
        : id(id)
        , response(response)
        , heartbeatTimer(core::timer::Timer::intervalTimer(
              [response] {
                  if (response->isConnected()) {
                      response->sendFragment(":keep-alive");
                      response->sendFragment();
                  }
              },
              39)) {
    }

    DashboardModel::EventReceiver::~EventReceiver() {
        heartbeatTimer.cancel();
    }

    DashboardModel::DashboardModel() = default;

    DashboardModel& DashboardModel::instance() {
        static DashboardModel dashboardModel;

        return dashboardModel;
    }

    void DashboardModel::addEventReceiver(const std::shared_ptr<express::Response>& response, [[maybe_unused]] const std::string& lastEventId) {
        const std::uint64_t eventReceiverId = nextEventReceiverId++;

        eventReceiverList.emplace_back(eventReceiverId, response);

        response->getSocketContext()->onDisconnected([this, eventReceiverId]() {
            eventReceiverList.remove_if([eventReceiverId](const EventReceiver& eventReceiver) {
                return eventReceiver.getId() == eventReceiverId;
            });
        });

        nlohmann::json initialize = {{"title", "Water Buoy Dashboard"}, {"service", "mqttcli"}};

        if (latestTemperature.has_value()) {
            initialize["latest"]["temperature"] = toJson(*latestTemperature);
        }
        if (latestPh.has_value()) {
            initialize["latest"]["ph"] = toJson(*latestPh);
        }
        if (latestTds.has_value()) {
            initialize["latest"]["tds"] = toJson(*latestTds);
        }
        if (latestTurbidity.has_value()) {
            initialize["latest"]["turbidity"] = toJson(*latestTurbidity);
        }
        if (latestBoardTemperature.has_value()) {
            initialize["latest"]["board_temperature"] = toJson(*latestBoardTemperature);
        }
        if (latestGps.has_value()) {
            initialize["latest"]["gps"] = toJson(*latestGps);
        }

        sendJsonEvent(response, initialize, "ui-initialize", std::to_string(nextEventId++));
    }

    void DashboardModel::publishScalarMeasurement(const ScalarMeasurement& measurement) {
        switch (measurement.fPort) {
            case 2:
                latestTemperature = measurement;
                break;
            case 3:
                latestPh = measurement;
                break;
            case 4:
                latestTds = measurement;
                break;
            case 5:
                latestTurbidity = measurement;
                break;
            case 6:
                latestBoardTemperature = measurement;
                break;
            default:
                break;
        }

        sendJsonEvent(toJson(measurement), "scalar-measurement");
    }

    void DashboardModel::publishGpsPosition(const GpsPosition& position) {
        latestGps = position;

        sendJsonEvent(toJson(position), "gps-position");
    }

    void DashboardModel::publishStorageError(const std::string& message, const std::optional<TtnUplink>& uplink) {
        nlohmann::json error = {{"message", message}};

        if (uplink.has_value()) {
            error["uplink"] = toJson(*uplink);
        }

        sendJsonEvent(error, "storage-error");
    }

    void DashboardModel::sendEvent(const std::shared_ptr<express::Response>& response,
                                   const std::string& data,
                                   const std::string& event,
                                   const std::string& id) {
        if (response->isConnected()) {
            if (!event.empty()) {
                response->sendFragment("event:" + event);
            }
            if (!id.empty()) {
                response->sendFragment("id:" + id);
            }
            response->sendFragment("data:" + data);
            response->sendFragment();
        }
    }

    void DashboardModel::sendJsonEvent(const std::shared_ptr<express::Response>& response,
                                       const nlohmann::json& json,
                                       const std::string& event,
                                       const std::string& id) {
        sendEvent(response, json.dump(), event, id);
    }

    void DashboardModel::sendJsonEvent(const nlohmann::json& json, const std::string& event) const {
        VLOG(0) << "Water buoy server sent event: " << event << "\n" << json.dump(4);

        for (auto& eventReceiver : eventReceiverList) {
            if (const auto& response = eventReceiver.response.lock()) {
                sendJsonEvent(response, json, event, std::to_string(nextEventId));
            }
        }

        ++const_cast<DashboardModel*>(this)->nextEventId;
    }

} // namespace mqtt::mqttcli::lib

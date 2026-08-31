/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Tobias Pfeil
 *               2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef MQTTBROKER_LIB_MAPPINGADMINROUTER_H
#define MQTTBROKER_LIB_MAPPINGADMINROUTER_H

namespace mqtt::lib {
    class ConfigApplication;
}

#include <express/Router.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib::admin {

    struct AdminOptions {
        std::string user;
        std::string pass;
        std::string realm{"mqttsuite-admin"};
    };

    struct ReloadResult {
        std::string mode;
        std::size_t instances{0};
        std::size_t subscribed{0};
        std::size_t unsubscribed{0};
    };

    using ReloadCallback = std::function<ReloadResult(bool)>;

    express::Router makeMappingAdminRouter(ConfigApplication* configApplication, const AdminOptions& opt, ReloadCallback onDeploy = {});

} // namespace mqtt::lib::admin

#endif // MQTTBROKER_LIB_MAPPINGADMINROUTER_H

/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MQTTSUITE_LOG_H
#define MQTTSUITE_LOG_H

#include <Log.h> // IWYU pragma: export

namespace mqttsuite::log {

    inline snode::log::Logger mappingLog() {
        return snode::log::application("mqttsuite.mapping");
    }

    inline snode::log::Logger brokerLog() {
        return snode::log::application("mqttsuite.broker");
    }

    inline snode::log::Logger integratorLog() {
        return snode::log::application("mqttsuite.integrator");
    }

    inline snode::log::Logger bridgeLog() {
        return snode::log::application("mqttsuite.bridge");
    }

    inline snode::log::Logger cliLog() {
        return snode::log::application("mqttsuite.cli");
    }

    inline snode::log::Logger storeLog() {
        return snode::log::application("mqttsuite.store");
    }

} // namespace mqttsuite::log

#endif // MQTTSUITE_LOG_H

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

#ifndef MQTTSUITE_SEMANTICLOG_H
#define MQTTSUITE_SEMANTICLOG_H

#include "log/LogScopeOwner.h" // IWYU pragma: export
#include "log/Logger.h"        // IWYU pragma: export

namespace mqttsuite::semantic {

    inline const logger::LogScopeOwner& mappingLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.mapping");
        return scope;
    }

    inline const logger::LogScopeOwner& brokerLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.broker");
        return scope;
    }

    inline const logger::LogScopeOwner& integratorLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.integrator");
        return scope;
    }

    inline const logger::LogScopeOwner& bridgeLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.bridge");
        return scope;
    }

    inline const logger::LogScopeOwner& cliLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.cli");
        return scope;
    }

    inline const logger::LogScopeOwner& storeLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Application, logger::LogBoundary::Application, "mqttsuite.store");
        return scope;
    }

    inline logger::BoundaryLogger mappingLog() {
        return mappingLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger brokerLog() {
        return brokerLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger integratorLog() {
        return integratorLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger bridgeLog() {
        return bridgeLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger cliLog() {
        return cliLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger storeLog() {
        return storeLogScope().logger(logger::Logger::semanticSink());
    }

} // namespace mqttsuite::semantic

#endif // MQTTSUITE_SEMANTICLOG_H

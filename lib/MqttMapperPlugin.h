/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MQTT_LIB_MQTTMAPPERPLUGIN_H
#define MQTT_LIB_MQTTMAPPERPLUGIN_H

// #include "MqttMapper.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace inja {
    using Arguments = std::vector<const nlohmann::json*>;
    using json = nlohmann::json;
} // namespace inja

namespace mqtt::lib {
    struct FunctionBase {
        FunctionBase(const std::string& name, int numArgs)
            : name(name)
            , numArgs(numArgs) {
        }

        std::string name;
        int numArgs;
    };

    struct Function : FunctionBase {
        Function(const std::string& name, int numArgs, const std::function<inja::json(inja::Arguments&)>& function)
            : FunctionBase(name, numArgs)
            , function(function) {
        }

        std::function<inja::json(inja::Arguments&)> function;
    };

    struct VoidFunction : FunctionBase {
        VoidFunction(const std::string& name, int numArgs, const std::function<void(inja::Arguments&)>& function)
            : FunctionBase(name, numArgs)
            , function(function) {
        }

        std::function<void(inja::Arguments&)> function;
    };

} // namespace mqtt::lib

extern "C" std::vector<mqtt::lib::Function> functions;
extern "C" std::vector<mqtt::lib::VoidFunction> voidFunctions;

#endif // MQTT_LIB_MQTTMAPPERPLUGIN_H

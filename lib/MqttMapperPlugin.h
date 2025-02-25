/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Fre
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

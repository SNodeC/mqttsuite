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

#include "lib/MqttMapperPlugin.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "lib/inja.hpp"

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib::plugins::double_plugin {

    nlohmann::json myDouble(const inja::Arguments& args);
    nlohmann::json myDouble(const inja::Arguments& args) {
        const int number = args.at(0)->get<int>();
        return 2 * number;
    }

} // namespace mqtt::lib::plugins::double_plugin

extern "C" {
    std::vector<mqtt::lib::Function> functions{{"double", 1, mqtt::lib::plugins::double_plugin::myDouble}};
}

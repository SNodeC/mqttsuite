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

#include "MqttMapperPlugin.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wc++98-compat-pedantic")
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wexit-time-destructors")
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
#if __has_warning("-Wglobal-constructors")
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif
#if __has_warning("-Wreserved-macro-identifier")
#pragma GCC diagnostic ignored "-Wreserved-macro-identifier"
#endif
#if __has_warning("-Wswitch-enum")
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#endif
#endif
#include "inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

#include <map>
#include <nlohmann/json.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib {

    nlohmann::json myDouble(const inja::Arguments& args);
    nlohmann::json myDouble(const inja::Arguments& args) {
        int number = args.at(0)->get<int>();
        return 2 * number;
    }

    std::vector<Function> functions{{"double", 1, mqtt::lib::myDouble}, {"double", 2, mqtt::lib::myDouble}};

} // namespace mqtt::lib

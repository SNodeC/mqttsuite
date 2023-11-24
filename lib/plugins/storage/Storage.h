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

#ifndef MQTT_LIB_PLUGINS_STORAGE_PLUGIN_STORAGE_H
#define MQTT_LIB_PLUGINS_STORAGE_PLUGIN_STORAGE_H

#include "lib/inja.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib::plugins::storage_plugin {

    class Storage {
    public:
    private:
        Storage() = default;

    public:
        Storage(const Storage&) = delete;

        Storage& operator=(const Storage&) = delete;

        static Storage& instance();

        static void store(const inja::Arguments& args);

        static const std::string& recall(const inja::Arguments& args);
        static int recall_as_int(const inja::Arguments& args);
        static double recall_as_float(const inja::Arguments& args);

        static bool is_empty(const inja::Arguments& args);

        static bool exists(const inja::Arguments& args);

        ~Storage() = default;

    private:
        std::map<std::string, std::string> storage;
    };

} // namespace mqtt::lib::plugins::storage_plugin

#endif // MQTT_LIB_PLUGINS_STORAGE_PLUGIN_STORAGE_H

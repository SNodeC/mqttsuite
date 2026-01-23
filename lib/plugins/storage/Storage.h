/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
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

#ifndef MQTT_LIB_PLUGINS_STORAGE_PLUGIN_STORAGE_H
#define MQTT_LIB_PLUGINS_STORAGE_PLUGIN_STORAGE_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#endif
#endif
#include "lib/inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

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

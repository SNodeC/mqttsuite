/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025
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

#include "Storage.h"

#include "lib/MqttMapperPlugin.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::lib::plugins::storage_plugin {

    Storage& Storage::instance() {
        static Storage storage;

        return storage;
    }

    void Storage::store(const inja::Arguments& args) {
        instance().storage[args.at(0)->get<std::string>()] = args.at(1)->get<std::string>();
    }

    const std::string& Storage::recall(const inja::Arguments& args) {
        return instance().storage[args.at(0)->get<std::string>()];
    }

    int Storage::recall_as_int(const inja::Arguments& args) {
        int result = 0;

        if (exists(args)) {
            try {
                result = std::stoi(instance().storage[args.at(0)->get<std::string>()]);
            } catch (const std::logic_error&) {
                // no error if not exist - just return '0'
            }
        }

        return result;
    }

    double Storage::recall_as_float(const inja::Arguments& args) {
        double result = 0;

        if (exists(args)) {
            try {
                result = std::stod(instance().storage[args.at(0)->get<std::string>()]);
            } catch (const std::logic_error&) {
                // no error if not exist - just return '0'
            }
        }

        return result;
    }

    bool Storage::is_empty(const inja::Arguments& args) {
        bool result = true;

        if (exists(args)) {
            result = instance().storage[args.at(0)->get<std::string>()].empty();
        }

        return result;
    }

    bool Storage::exists(const inja::Arguments& args) {
        return instance().storage.contains(args.at(0)->get<std::string>());
    }

} // namespace mqtt::lib::plugins::storage_plugin

extern "C" {
    std::vector<mqtt::lib::Function> functions{{"recall", 1, mqtt::lib::plugins::storage_plugin::Storage::recall},
                                               {"recall_as_int", 1, mqtt::lib::plugins::storage_plugin::Storage::recall_as_int},
                                               {"recall_as_float", 1, mqtt::lib::plugins::storage_plugin::Storage::recall_as_float},
                                               {"is_empty", 1, mqtt::lib::plugins::storage_plugin::Storage::is_empty},
                                               {"exists", 1, mqtt::lib::plugins::storage_plugin::Storage::exists}};

    std::vector<mqtt::lib::VoidFunction> voidFunctions{{"store", 2, mqtt::lib::plugins::storage_plugin::Storage::store}};
}

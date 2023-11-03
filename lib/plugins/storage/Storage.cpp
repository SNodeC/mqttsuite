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

#include "Storage.h"

#include "lib/MqttMapperPlugin.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

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

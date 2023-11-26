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

#ifndef BRIDGECONFIGLOADER_H
#define BRIDGECONFIGLOADER_H

namespace mqtt::bridge::lib {
    class Bridge;
} // namespace mqtt::bridge::lib

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json.hpp> // IWYU pragma: keep
#include <string>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class BridgeConfigLoader {
    private:
        BridgeConfigLoader() = default;

        ~BridgeConfigLoader();

    public:
        static BridgeConfigLoader& instance();

        bool loadAndValidate(const std::string& fileName);

        Bridge* getBridge(const std::string& instanceName);
        const std::map<std::string, nlohmann::json>& getBrokers();

    private:
        static nlohmann::json bridgeJsonSchema;
        static const std::string bridgeJsonSchemaString;
        static nlohmann::json bridgeConfigJson;

        std::map<std::string, Bridge*> bridges;
        std::map<std::string, nlohmann::json> brokers;
    };

} // namespace mqtt::bridge::lib

#endif // BRIDGECONFIGLOADER_H

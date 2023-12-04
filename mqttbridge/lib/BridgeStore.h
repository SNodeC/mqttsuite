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

#include "lib/Bridge.h" // IWYU pragma: export
#include "lib/Broker.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class BridgeStore {
    private:
        BridgeStore() = default;

    public:
        static BridgeStore& instance();

        bool loadAndValidate(const std::string& fileName);

        const Broker& getBroker(const std::string& instanceName);
        const std::map<std::string, Broker>& getBrokers();

    private:
        std::list<Bridge> bridgeList;
        std::map<std::string, Broker> brokers;
    };

} // namespace mqtt::bridge::lib

#endif // BRIDGECONFIGLOADER_H

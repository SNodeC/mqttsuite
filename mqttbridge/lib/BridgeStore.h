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

#include "Broker.h"

namespace mqtt::bridge::lib {
    class Bridge;
} // namespace mqtt::bridge::lib

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class BridgeStore {
    private:
        BridgeStore() = default;

        ~BridgeStore();

    public:
        static BridgeStore& instance();

        bool loadAndValidate(const std::string& fileName);

        Bridge* getBridge(const std::string& instanceName);
        const Broker& getBroker(const std::string& instanceName);
        const std::map<std::string, Broker>& getBrokers();

    private:
        std::map<std::string, Bridge*> bridges;
        std::map<std::string, Broker> brokers;
    };

} // namespace mqtt::bridge::lib

#endif // BRIDGECONFIGLOADER_H

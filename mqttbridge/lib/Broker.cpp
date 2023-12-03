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

#include "Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iot/mqtt/Topic.h>
#include <map>
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    Broker::Broker(const nlohmann::json& brokerJsonConfig)
        : instanceName(brokerJsonConfig["instance_name"])
        , protocol(brokerJsonConfig["protocol"])
        , encryption(brokerJsonConfig["encryption"])
        , transport(brokerJsonConfig["transport"]) {
        for (const nlohmann::json& topicJson : brokerJsonConfig["topics"]) {
            topics.emplace_back(topicJson["topic"], topicJson["qos"]);
        }
    }

    Broker::~Broker() {
    }

    const std::string& Broker::getInstanceName() const {
        return instanceName;
    }

    const std::string& Broker::getProtocol() const {
        return protocol;
    }

    const std::string& Broker::getEncryption() const {
        return encryption;
    }

    const std::string& Broker::getTransport() const {
        return transport;
    }

    const std::list<iot::mqtt::Topic>& Broker::getTopics() const {
        return topics;
    }

} // namespace mqtt::bridge::lib

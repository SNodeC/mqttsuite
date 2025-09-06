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

#ifndef APPS_MQTTBROKER_MQTT_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTT_SOCKETCONTEXT_H

#include <iot/mqtt/client/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <string>

#endif

namespace mqtt::mqtt::lib {

    class Mqtt : public iot::mqtt::client::Mqtt {
    public:
        explicit Mqtt(const std::string& connectionName,
                      const std::string& clientId,
                      uint8_t qoSDefault,
                      uint16_t keepAlive,
                      bool cleanSession,
                      const std::string& willTopic,
                      const std::string& willMessage,
                      uint8_t willQoS,
                      bool willRetain,
                      const std::string& username,
                      const std::string& password,
                      const std::list<std::string>& subTopics,
                      const std::string& pubTopic,
                      const std::string& pubMessage,
                      bool pubRetain = false,
                      const std::string& sessionStoreFileName = "");

    private:
        using Super = iot::mqtt::client::Mqtt;

        void onConnected() final;
        [[nodiscard]] bool onSignal(int signum) final;

        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onConnack(const iot::mqtt::packets::Connack& connack) final;
        void onSuback(const iot::mqtt::packets::Suback& suback) final;
        void onPuback(const iot::mqtt::packets::Puback& puback) final;
        void onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) final;

        const uint8_t qoSDefault;
        const uint16_t keepAlive;
        const bool cleanSession;

        const std::string willTopic;
        const std::string willMessage;
        const uint8_t willQoS;
        const bool willRetain;
        const std::string username;
        const std::string password;

        const std::list<std::string> subTopics;
        const std::string pubTopic;
        const std::string pubMessage;
        const bool pubRetain;
    };

} // namespace mqtt::mqtt::lib

#endif // APPS_MQTTBROKER_MQTTSUB_SOCKETCONTEXT_H

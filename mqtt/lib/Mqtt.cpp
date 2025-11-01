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

#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>
#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Suback.h>
#include <utils/base64.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <tuple>
#include <unistd.h>
#include <utils/system/signal.h>
#include <vector>

#endif

// include the single‐header JSON library:
// https://github.com/nlohmann/json/releases
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// get current terminal width, fallback to 80
static int getTerminalWidth() {
    int termWidth = 80;

    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        termWidth = w.ws_col;
    }

    return termWidth;
}

// split one paragraph of text into lines of at most `width` characters
static std::vector<std::string> wrapParagraph(const std::string& text, std::size_t width) {
    std::istringstream words(text);
    std::string word, line;
    std::vector<std::string> lines;
    while (words >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= width) {
            line += ' ' + word;
        } else {
            lines.push_back(line);
            line = word;
        }
    }

    if (!line.empty()) {
        lines.push_back(line);
    }

    return lines;
}

///
/// Formats:
///   prefix ┬ headLine
///          ├ <first message line>
///          │ <middle lines>
///          └ <last message line>
///
/// If `message` parses as JSON, we pretty‐print it (indent=2).
/// Otherwise we wrap it to the terminal width.
///
/// Returns the whole formatted string (with trailing newline on each line).
///
std::vector<std::string> static myformat(const std::string& prefix,
                                         const std::string& headLine,
                                         const std::string& message,
                                         std::size_t initialPrefixLength = 0) {
    // how many spaces before the box‐drawing char on subsequent lines?
    const size_t prefixLen = prefix.size();
    const size_t indentCount = prefixLen + 1; // +1 for the space before ┬, +33 for easylogging++ prefix format
    const std::string indent(indentCount, ' ');

    std::vector<std::string> lines;

    const int termWidth = getTerminalWidth();

    size_t avail = (termWidth > int(indentCount + 2)) ? static_cast<std::size_t>(termWidth) - (indentCount + 2) : 20u;

    auto wrapped = wrapParagraph(prefix + " ┬ " + headLine, avail - (prefix.length() + 2));

    if (wrapped.empty()) {
        wrapped.push_back("");
    }

    //    lines.insert(lines.end(), wrapped.begin(), wrapped.end());

    bool first = true;
    for (const auto& line : wrapped) {
        lines.emplace_back((first ? "" : indent + "│ ") + line);
        first = false;
    }

    // try parsing as JSON
    try {
        auto j = json::parse(message);
        // pretty‐print with 2-space indent
        std::string pretty = j.dump(2);
        // split into lines
        std::istringstream prettyIStringStream(pretty);

        for (auto [line, lineNumnber] = std::tuple{std::string(""), 0}; std::getline(prettyIStringStream, line); lineNumnber++) {
            if (lineNumnber == 0 && !prettyIStringStream.eof()) {
                lines.push_back(indent + "├ " + line);
            } else if (prettyIStringStream.eof()) {
                lines.push_back(indent + "└ " + line);
            } else {
                lines.push_back(indent + "│ " + line);
            }
        }
    } catch (json::parse_error&) {
        // not JSON → wrap text

        // break original message on hard newlines and wrap each paragraph
        std::istringstream messageIStringStream(message);
        std::vector<std::string> allLines;
        for (std::string line; std::getline(messageIStringStream, line);) {
            wrapped = wrapParagraph(line, avail - initialPrefixLength);

            if (wrapped.empty()) {
                wrapped.push_back("");
            }

            allLines.insert(allLines.end(), wrapped.begin(), wrapped.end());
        }

        if (!allLines.empty() && allLines.back().empty()) {
            allLines.pop_back();
        }

        // emit with ├, │ and └
        for (std::size_t lineNumber = 0; lineNumber < allLines.size(); ++lineNumber) {
            if (lineNumber == 0 && lineNumber + 1 != allLines.size()) {
                lines.push_back(indent + "├ " + allLines[lineNumber]);
            } else if (lineNumber + 1 == allLines.size()) {
                lines.push_back(indent + "└ " + allLines[lineNumber]);
            } else {
                lines.push_back(indent + "│ " + allLines[lineNumber]);
            }
        }
    }

    return lines;
}

// 2025-05-28 17:46:11 0000000014358
static const std::string formatAsLogString(const std::string& prefix, const std::string& headLine, const std::string& message) {
    std::ostringstream formatAsLogStringStream;

    for (const std::string& line : myformat(prefix, headLine, message, 34)) {
        formatAsLogStringStream << (formatAsLogStringStream.view().empty() ? "" : "                                  ") << line << "\n";
    }

    std::string formatStr = formatAsLogStringStream.str();

    formatStr.pop_back();

    return formatStr;
}

namespace mqtt::mqtt::lib {

    Mqtt::Mqtt(const std::string& connectionName,
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
               bool pubRetain,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, keepAlive, sessionStoreFileName)
        , postgresDB(
              {
                  // Connection detail
                  .hostname = "localhost", // raspberrypi-itnh.local
                  .username = "itnh",
                  .password = "q66yg8StA7Fw", // Hi everyone! Please don't hack our DB :)
                  .database = "itnh",
                  .port = 50000,
              },
              5) // Pool size of 5 connections for parallel queries
        , qoSDefault(qoSDefault)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password)
        , subTopics(subTopics)
        , pubTopic(pubTopic)
        , pubMessage(pubMessage)
        , pubRetain(pubRetain) {
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "  Keep Alive: " << keepAlive;
        VLOG(1) << "  Clean Session: " << cleanSession;
        VLOG(1) << "  Will Topic: " << willTopic;
        VLOG(1) << "  Will Message: " << willMessage;
        VLOG(1) << "  Will QoS: " << static_cast<uint16_t>(willQoS);
        VLOG(1) << "  Will Retain " << willRetain;
        VLOG(1) << "  Username: " << username;
        VLOG(1) << "  Password: " << password;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();

        return Super::onSignal(signum);
    }

    static uint8_t getQos(const std::string& qoSString) {
        unsigned long qoS = std::stoul(qoSString);

        if (qoS > 2) {
            throw std::out_of_range("qos " + qoSString + " not in range [0..2]");
        }

        return static_cast<uint8_t>(qoS);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        bool sendDisconnectFlag = true;

        if (connack.getReturnCode() == 0) {
            if (!subTopics.empty()) {
                VLOG(0) << "MQTT Subscribe";

                try {
                    std::list<iot::mqtt::Topic> topicList;
                    std::transform(subTopics.begin(),
                                   subTopics.end(),
                                   std::back_inserter(topicList),
                                   [qoSDefault = this->qoSDefault](const std::string& compositTopic) -> iot::mqtt::Topic {
                                       std::size_t pos = compositTopic.rfind("##");

                                       const std::string topic = compositTopic.substr(0, pos);
                                       uint8_t qoS = qoSDefault;

                                       if (pos != std::string::npos) {
                                           try {
                                               qoS = getQos(compositTopic.substr(pos + 2));
                                           } catch (const std::logic_error& error) {
                                               VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                                                       << "] Malformed composit topic: " << compositTopic << "\n"
                                                       << error.what();
                                               throw;
                                           }
                                       }
                                       VLOG(0) << "  t: " << static_cast<int>(qoS) << " | " << topic;
                                       return iot::mqtt::Topic(topic, qoS);
                                   });
                    sendSubscribe(topicList);

                    sendDisconnectFlag = false;
                } catch (const std::logic_error&) {
                }
            }

            if (!pubTopic.empty()) {
                VLOG(0) << "MQTT Publish";

                std::size_t pos = pubTopic.rfind("##");

                const std::string topic = pubTopic.substr(0, pos);

                uint8_t qoS = qoSDefault;

                try {
                    if (pos != std::string::npos) {
                        try {
                            qoS = getQos(pubTopic.substr(pos + 2));
                        } catch (const std::logic_error& error) {
                            VLOG(0) << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                                    << "] Malformed composit topic: " << pubTopic << "\n"
                                    << error.what();
                            throw;
                        }
                    }
                    sendPublish(topic, pubMessage, qoS, pubRetain);

                    sendDisconnectFlag = qoS > 0 ? false : sendDisconnectFlag;
                } catch (const std::logic_error&) {
                }
            }
            if (sendDisconnectFlag) {
                sendDisconnect();
            }
        } else {
            sendDisconnect();
        }
    }

    void Mqtt::onSuback(const iot::mqtt::packets::Suback& suback) {
        VLOG(1) << "MQTT Suback";

        for (auto returnCode : suback.getReturnCodes()) {
            VLOG(0) << "  r: " << static_cast<int>(returnCode);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        std::string prefix = "MQTT Publish";
        std::string headLine = publish.getTopic() + " │ QoS: " + std::to_string(static_cast<uint16_t>(publish.getQoS())) +
                               " │ Retain: " + (publish.getRetain() != 0 ? "true" : "false") +
                               " │ Dup: " + (publish.getDup() != 0 ? "true" : "false");

        VLOG(0) << formatAsLogString(prefix, headLine, publish.getMessage());

        nlohmann::json messageAsJSON;
        try {
            messageAsJSON = nlohmann::json::parse(publish.getMessage());
        } catch (const nlohmann::json::parse_error& e) {
            VLOG(0) << "Failed to parse JSON: " << e.what();
            return;
        }

        auto safeGet = [](const nlohmann::json& j, const std::vector<std::string>& path) -> nlohmann::json {
            const nlohmann::json* cur = &j;
            for (const auto& p : path) {
                if (cur->contains(p)) {
                    cur = &(*cur)[p];
                } else {
                    return nullptr;
                }
            }
            return *cur;
        };

        std::string device_id = safeGet(messageAsJSON, {"end_device_ids", "device_id"}).get<std::string>();
        std::string ts = safeGet(messageAsJSON, {"received_at"}).get<std::string>();
        auto uplink = messageAsJSON["uplink_message"];
        // int f_cnt = uplink["f_cnt"].get<int>();
        // int f_port = uplink["f_port"].get<int>();
        auto decoded = uplink.value("decoded_payload", nlohmann::json::object());
        auto rx_metadata = uplink.value("rx_metadata", nlohmann::json::array());

        std::optional<double> temperature =
            decoded.contains("temperature") ? std::make_optional(decoded["temperature"].get<double>()) : std::nullopt;
        std::optional<double> ph = decoded.contains("ph") ? std::make_optional(decoded["ph"].get<double>()) : std::nullopt;
        std::optional<double> dts = decoded.contains("dts") ? std::make_optional(decoded["dts"].get<double>()) : std::nullopt;
        std::optional<double> lat, lon, alt;
        if (!rx_metadata.empty() && rx_metadata[0].contains("location")) {
            auto loc = rx_metadata[0]["location"];
            lat = loc.value("latitude", 0.0);
            lon = loc.value("longitude", 0.0);
            alt = loc.value("altitude", 0.0);
        }

        // we call this auto function after we have the sensor_id which we get below from the sensors table
        // then we run insertMeasurements(sensor_id) to insert the measurements
        auto insertMeasurements = [this, ts, temperature, ph, dts, lat, lon, alt](int sensor_id) {
            if (temperature) {
                postgresDB.exec(
                    "INSERT INTO \"TemperatureReading\" (\"sensorId\", value, \"createdAt\") VALUES ($1, $2, $3)",
                    [sensor_id]([[maybe_unused]] nlohmann::json result) {
                        VLOG(2) << "Inserted temperature for sensor " << sensor_id;
                    },
                    [](const std::string& err, [[maybe_unused]] int code) {
                        VLOG(0) << "Error inserting temperature: " << err;
                    },
                    std::vector<nlohmann::json>{sensor_id, *temperature, ts});
            }
            if (ph) {
                postgresDB.exec(
                    "INSERT INTO \"PhReading\" (\"sensorId\", value, \"createdAt\") VALUES ($1, $2, $3)",
                    [sensor_id]([[maybe_unused]] nlohmann::json result) {
                        VLOG(2) << "Inserted ph for sensor " << sensor_id;
                    },
                    [](const std::string& err, [[maybe_unused]] int code) {
                        VLOG(0) << "Error inserting ph: " << err;
                    },
                    std::vector<nlohmann::json>{sensor_id, *ph, ts});
            }
            if (dts) {
                postgresDB.exec(
                    "INSERT INTO \"DtsReading\" (\"sensorId\", value, \"createdAt\") VALUES ($1, $2, $3)",
                    [sensor_id]([[maybe_unused]] nlohmann::json result) {
                        VLOG(2) << "Inserted dts for sensor " << sensor_id;
                    },
                    [](const std::string& err, [[maybe_unused]] int code) {
                        VLOG(0) << "Error inserting dts: " << err;
                    },
                    std::vector<nlohmann::json>{sensor_id, std::to_string(*dts), ts});
            }
            if (lat && lon) {
                postgresDB.exec(
                    "INSERT INTO \"GpsReading\" (\"sensorId\", lat, lon, alt, \"createdAt\") VALUES ($1, $2, $3, $4, $5)",
                    [sensor_id]([[maybe_unused]] nlohmann::json result) {
                        VLOG(2) << "Inserted GPS for sensor " << sensor_id;
                    },
                    [](const std::string& err, [[maybe_unused]] int code) {
                        VLOG(0) << "Error inserting GPS: " << err;
                    },
                    std::vector<nlohmann::json>{sensor_id, *lat, *lon, alt.value_or(0.0), ts});
            }
        };

        // Check if the sensor exists
        postgresDB.exec(
            "SELECT id FROM \"Sensor\" WHERE \"deviceId\" = $1",
            [this, device_id, insertMeasurements](nlohmann::json result) {
                if (!result.empty()) {
                    int sensor_id = result[0]["id"].get<int>();
                    // insert for found sensor
                    insertMeasurements(sensor_id);
                } else {
                    // Sensor doesn't exist, insert it first
                    postgresDB.exec(
                        "INSERT INTO \"Sensor\" (\"deviceId\") VALUES ($1) RETURNING id",
                        [this, insertMeasurements](nlohmann::json insertResult) {
                            if (!insertResult.empty()) {
                                int sensor_id = insertResult[0]["id"].get<int>();
                                // insert for new created sensor
                                insertMeasurements(sensor_id);
                            }
                        },
                        [](const std::string& err, [[maybe_unused]] int code) {
                            VLOG(0) << "Error inserting sensor: " << err;
                        },
                        std::vector<nlohmann::json>{device_id});
                }
            },
            [](const std::string& err, [[maybe_unused]] int code) {
                VLOG(0) << "Error selecting sensor: " << err;
            },
            std::vector<nlohmann::json>{device_id});
    }
    
    void Mqtt::onPuback([[maybe_unused]] const iot::mqtt::packets::Puback& puback) {
        if (subTopics.empty()) {
            sendDisconnect();
        }
    }

    void Mqtt::onPubcomp([[maybe_unused]] const iot::mqtt::packets::Pubcomp& pubcomp) {
        if (subTopics.empty()) {
            sendDisconnect();
        }
    }

} // namespace mqtt::mqtt::lib
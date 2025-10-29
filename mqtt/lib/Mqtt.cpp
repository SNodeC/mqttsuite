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
#include <mysql.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <tuple>
#include <unistd.h>
#include <utils/system/signal.h>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>  // for iter_impl
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>                    // for json

#endif

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
        , mariaDB( // Connection detail
              {
                  .connectionName = connectionName,
                  .hostname = "localhost",
                  .username = "snodec",
                  .password = "pentium5",
                  .database = "snodec",
                  .port = 3306,
                  .socket = "/run/mysqld/mysqld.sock",
                  .flags = 0,
              },
              [&connectionName = this->connectionName](const database::mariadb::MariaDBState& state) {
                  if (state.connected) {
                      VLOG(0) << connectionName << " MariaDB: Connected";
                  } else if (state.error != 0) {
                      VLOG(0) << connectionName << " MariaDB: " << state.errorMessage << " [" << state.error << "]";
                  } else {
                      VLOG(0) << connectionName << " MariaDB: Lost connection";
                  }
              })
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

        nlohmann::json messageAsJSON = nlohmann::json::parse(publish.getMessage());

        // Here you can analyse and retrieve data from the json (j
        // and decide on base of the fPort what to do with the data
        // Maybe store it into a particular db table ...
        // E.g.:

        VLOG(0) << "ApplicationId: " << messageAsJSON["end_device_ids"]["application_ids"]["application_id"];

        VLOG(0) << "Uplink message field\n" << messageAsJSON["uplink_message"].dump(4);
        VLOG(0) << "Decoded payload field\n" << messageAsJSON["uplink_message"]["decoded_payload"].dump(4);

        VLOG(0) << "DeviceID: " << messageAsJSON["end_device_ids"]["device_id"];
        VLOG(0) << "DeviceEUI: " << messageAsJSON["end_device_ids"]["dev_eui"];
        VLOG(0) << "Received at: " << messageAsJSON["received_at"];
        for (const auto& rx_metadata : messageAsJSON["uplink_message"]["rx_metadata"]) {
            VLOG(0) << "Received via GW: " << rx_metadata["gateway_ids"]["gateway_id"];
        }

        VLOG(0) << "MessageCnt: " << messageAsJSON["uplink_message"]["f_cnt"];
        VLOG(0) << "F-Port field: " << messageAsJSON["uplink_message"]["f_port"];
        VLOG(0) << "Frm payload field: " << messageAsJSON["uplink_message"]["frm_payload"];
        VLOG(0) << "Frm payload base64 decoded: " << base64::base64_decode(messageAsJSON["uplink_message"]["frm_payload"]);

        // This insert is just a dummy insert ...

        mariaDB.exec(
            "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','" + publish.getMessage() + "')",
            [&mariaDB = this->mariaDB, &connectionName = this->connectionName](void) -> void {
                VLOG(0) << connectionName << " MariaDB: Query completed";
                mariaDB.affectedRows(
                    [](my_ulonglong affectedRows) -> void {
                        VLOG(0) << "  query affected rows: " << affectedRows;
                    },
                    [](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "  query affected rows failed: " << errorString << " : " << errorNumber;
                    });
            },
            [&connectionName = this->connectionName](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << connectionName << " MariaDB: Query failed: " << errorString << " : " << errorNumber;
            });
        // End of dummy insert
    };

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

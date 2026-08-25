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

#include "Mqtt.h"

#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>
#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/packets/Suback.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "lib/SemanticLog.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <list>
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

#include <nlohmann/json.hpp>

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
        auto j = nlohmann::json::parse(message);
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
    } catch (nlohmann::json::parse_error&) {
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

namespace mqtt::mqttcli::lib {

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
        mqttsuite::semantic::cliLog().debug() << "Client Id: " << clientId;
        mqttsuite::semantic::cliLog().debug() << "  Keep Alive: " << keepAlive;
        mqttsuite::semantic::cliLog().debug() << "  Clean Session: " << cleanSession;
        mqttsuite::semantic::cliLog().debug() << "  Will Topic: " << willTopic;
        mqttsuite::semantic::cliLog().debug() << "  Will Message: " << willMessage;
        mqttsuite::semantic::cliLog().debug() << "  Will QoS: " << static_cast<uint16_t>(willQoS);
        mqttsuite::semantic::cliLog().debug() << "  Will Retain " << willRetain;
        mqttsuite::semantic::cliLog().debug() << "  Username: " << username;
        mqttsuite::semantic::cliLog().debug() << "  Password: " << password;
    }

    void Mqtt::onConnected() {
        mqttsuite::semantic::cliLog().debug() << "MQTT: Initiating Session";

        sendConnect(cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        mqttsuite::semantic::cliLog().debug() << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG"
                                              << utils::system::sigabbrev_np(signum) << " = " << signum << ")";

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
        if (connack.getReturnCode() == 0) {
            bool sendDisconnectFlag = true;

            if (!subTopics.empty()) {
                mqttsuite::semantic::cliLog().info() << "MQTT Subscribe";

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
                                               mqttsuite::semantic::cliLog().info()
                                                   << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
                                                   << "] Malformed composit topic: " << compositTopic << "\n"
                                                   << error.what();
                                               throw;
                                           }
                                       }
                                       mqttsuite::semantic::cliLog().info() << "  t: " << static_cast<int>(qoS) << " | " << topic;
                                       return iot::mqtt::Topic(topic, qoS);
                                   });
                    sendSubscribe(topicList);

                    sendDisconnectFlag = false;
                } catch (const std::logic_error&) {
                }
            }

            if (!pubTopic.empty()) {
                mqttsuite::semantic::cliLog().info() << "MQTT Publish";

                std::size_t pos = pubTopic.rfind("##");

                const std::string topic = pubTopic.substr(0, pos);

                uint8_t qoS = qoSDefault;

                try {
                    if (pos != std::string::npos) {
                        try {
                            qoS = getQos(pubTopic.substr(pos + 2));
                        } catch (const std::logic_error& error) {
                            mqttsuite::semantic::cliLog().info() << "[" << Color::Code::FG_RED << "Error" << Color::Code::FG_DEFAULT
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
        mqttsuite::semantic::cliLog().debug() << "MQTT Suback";

        for (auto returnCode : suback.getReturnCodes()) {
            mqttsuite::semantic::cliLog().info() << "  r: " << static_cast<int>(returnCode);
        }
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        std::string prefix = "MQTT Publish";
        std::string headLine = publish.getTopic() + " │ QoS: " + std::to_string(static_cast<uint16_t>(publish.getQoS())) +
                               " │ Retain: " + (publish.getRetain() != 0 ? "true" : "false") +
                               " │ Dup: " + (publish.getDup() != 0 ? "true" : "false");

        mqttsuite::semantic::cliLog().info() << formatAsLogString(prefix, headLine, publish.getMessage());
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

} // namespace mqtt::mqttcli::lib

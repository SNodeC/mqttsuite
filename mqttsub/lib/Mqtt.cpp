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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <iterator>
#include <list>
#include <log/Logger.h>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
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
///             ├ <first message line>
///             │ <middle lines>
///             └ <last message line>
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

namespace mqtt::mqttsub::lib {

    Mqtt::Mqtt(const std::string& connectionName,
               const std::string& clientId,
               const std::list<std::string>& topics,
               uint8_t qoS,
               uint16_t keepAlive,
               bool cleanSession,
               const std::string& willTopic,
               const std::string& willMessage,
               uint8_t willQoS,
               bool willRetain,
               const std::string& username,
               const std::string& password,
               const std::string& sessionStoreFileName)
        : iot::mqtt::client::Mqtt(connectionName, clientId, sessionStoreFileName)
        , topics(topics)
        , qoS(qoS)
        , keepAlive(keepAlive)
        , cleanSession(cleanSession)
        , willTopic(willTopic)
        , willMessage(willMessage)
        , willQoS(willQoS)
        , willRetain(willRetain)
        , username(username)
        , password(password) {
        VLOG(1) << "Keep Alive: " << keepAlive;
        VLOG(1) << "Client Id: " << clientId;
        VLOG(1) << "Clean Session: " << cleanSession;
        VLOG(1) << "Will Topic: " << willTopic;
        VLOG(1) << "Will Message: " << willMessage;
        VLOG(1) << "Will QoS: " << static_cast<uint16_t>(willQoS);
        VLOG(1) << "Will Retain " << willRetain;
        VLOG(1) << "Username: " << username;
        VLOG(1) << "Password: " << password;
    }

    void Mqtt::onConnected() {
        VLOG(1) << "MQTT: Initiating Session";

        sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);
    }

    bool Mqtt::onSignal(int signum) {
        VLOG(1) << "MQTT: On Exit due to '" << strsignal(signum) << "' (SIG" << utils::system::sigabbrev_np(signum) << " = " << signum
                << ")";

        sendDisconnect();

        return Super::onSignal(signum);
    }

    void Mqtt::onConnack(const iot::mqtt::packets::Connack& connack) {
        VLOG(0) << "MQTT Subscribe";
        if (connack.getReturnCode() == 0) {
            std::list<iot::mqtt::Topic> topicList;
            std::transform(topics.begin(),
                           topics.end(),
                           std::back_inserter(topicList),
                           [qoS = this->qoS](const std::string& topic) -> iot::mqtt::Topic {
                               VLOG(0) << "  t: " << static_cast<int>(qoS) << " | " << topic;
                               return iot::mqtt::Topic(topic, qoS);
                           });
            sendSubscribe(topicList);
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
    }

} // namespace mqtt::mqttsub::lib

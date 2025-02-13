/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "SharedSocketContextFactory.h"
#include "lib/Mqtt.h"
#include "lib/MqttModel.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
#include <iot/mqtt/MqttContext.h>
//
#include <express/legacy/in/WebApp.h>
#include <express/legacy/in6/WebApp.h>
#include <express/middleware/JsonMiddleware.h>
#include <express/tls/in/WebApp.h>
#include <express/tls/in6/WebApp.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/in/stream/tls/SocketServer.h>
#include <net/un/stream/legacy/SocketServer.h>
#include <net/un/stream/tls/SocketServer.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
//
#include <cstdlib>
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#endif

static void upgrade APPLICATION(req, res) {
    if (req->get("sec-websocket-protocol").find("mqtt") != std::string::npos) {
        res->upgrade(req, [req, res](const std::string& name) {
            if (!name.empty()) {
                VLOG(1) << "Successful upgrade to '" << name << "'  requested: " << req->get("upgrade");
            } else {
                VLOG(1) << "Can not upgrade to any of '" << req->get("upgrade") << "'";
            }
            res->end();
        });
    } else {
        VLOG(1) << "Not supporting any of: " << req->get("sec-websocket-protocol");

        res->sendStatus(404);
    }
}

static std::string href(const std::string& text, const std::string& link) {
    return "<a href=\"" + link + "\" style=\"color:inherit;\">" + text + "</a>";
}

static std::string href(const std::string& text, const std::string& url, const std::string& windowId, uint16_t width, uint16_t height) {
    return "<a href=\"#\" onClick=\"" + windowId + "=window.open('" + url + "', '" + windowId + "', 'width=" + std::to_string(width) +
           ", height=" + std::to_string(height) +
           ",location=no, menubar=no, status=no, toolbar=no'); return false;\"  \" style=\"color:inherit;\">" + text + "</a>";
}

static std::string getHTMLClientTable(mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    static constexpr std::string_view htmlClientTable = R""(
        <tr>
          <td>{client_id}</td>
          <td>{online_since}</td>
          <td align="right"><duration>{online_duration}</duration></td>
          <td>{connection_name}</td>
          <td>{local_address}</td>
          <td>{remote_address}</td>
          <td align="center"><button onclick="disconnectClient('{connection_name}')">Disconnect</button></td>
        </tr>)"";

    std::string table;

    for (const auto& [connectionName, mqttModelEntry] : mqttModel.getClients()) {
        const mqtt::mqttbroker::lib::Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        std::string windowId = "window" + std::to_string(reinterpret_cast<unsigned long long>(mqtt));

        table += fmt::format(htmlClientTable,
                             fmt::arg("client_id", href(mqtt->getClientId(), "/client/?" + mqtt->getConnectionName(), windowId, 450, 900)),
                             fmt::arg("online_since", mqttModelEntry.onlineSince()),
                             fmt::arg("online_duration", mqttModelEntry.onlineDuration()),
                             fmt::arg("connection_name", mqtt->getConnectionName()),
                             fmt::arg("local_address", socketConnection->getLocalAddress().toString()),
                             fmt::arg("remote_address", socketConnection->getRemoteAddress().toString()));
    }

    return R""(<table>
      <thead>
        <tr>
          <th>Client ID</th>
          <th>Online Since</th>
          <th>Duration</th>
          <th>Connection</th>
          <th>Locale Address</th>
          <th>Remote Address</th>
          <th>Action</th>
        </tr>
      </thead>
      <tbody>)"" +
           table + R""(
      </tbody>
    </table>)"";
}

static std::string getHTMLPageClientTable(mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    static constexpr std::string_view htmlPageClientTable = R""(<!DOCTYPE html>
<html>
<head>
  <style>
    * {{
      font-family: Arial, sans-serif;
    }}
    html, body {{
      height: 100%;
      margin: 0;
      overflow: hidden;
    }}
    body {{
      display: flex;
      flex-direction: column;
    }}
    header {{
      background: #e0e0e0;
      text-align: center;
    }}
    footer {{
      background: #e0e0e0;
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px;
      box-sizing: border-box;
    }}
    main {{
      flex: 1 1 auto;
      overflow: hidden;
      box-sizing: border-box;
      padding-left: 10px;
      padding-right:  10px;
      padding-top: 20px;
      padding-bottom: 20px;
    }}
    .tableFixHead {{
      overflow: auto;
      height: 100%;
      table {{
        width: 100%;
        border-collapse: collapse;
      }}
      th {{
        position: sticky;
        top: 0;
        z-index: 1;
        background-color:#e0e0e0;
      }}
      tr:nth-child(even) {{
        background-color: #f9f9f9;
      }}
      tr:hover {{
        background-color: #e0e0e0;
      }}
      th, td {{
        padding: 12px;
        box-shadow: inset 0px 0px 0px 1px #ccc, inset 0px 0px 0px 0px #ccc;
      }}
      td:nth-child(1),
      td:nth-child(2),
      td:nth-child(3),
      td:nth-child(4) {{
        white-space: nowrap;
      }}
    }}
  </style>
  <script>
    function disconnectClient(connectionName) {{
      fetch("/clients/", {{
        method: "POST",
        body: JSON.stringify({{
          connection_name: connectionName
        }}),
        headers: {{
          "Content-type": "application/json; charset=UTF-8"
        }}
      }})
      .then(response => {{
        return response.text().then(body => {{
          return {{ status: response.status, body: body, ok: response.ok }};
        }});
      }})
      .then(result => {{
        if (!result.ok) {{
          throw new Error("Network response was not ok\n" + result.status + ": " + result.body);
        }}
        return result.body;
      }})
      .then(body => {{
        console.log("Data received:", body);
        window.location.reload();
      }})
      .catch(error => {{
        console.error("There was a problem with the fetch operation:", error);
        alert(error);
        window.location.reload();
      }});
    }}
    function parseDuration(durationStr) {{
      var days = 0;
      var timeStr = durationStr;
      if (durationStr.indexOf(",") !== -1) {{
        var parts = durationStr.split(",");
        var dayPart = parts[0].trim();
        days = parseInt(dayPart.split(" ")[0], 10);
        timeStr = parts[1].trim();
      }}
      var timeParts = timeStr.split(":");
      var hours = parseInt(timeParts[0], 10);
      var minutes = parseInt(timeParts[1], 10);
      var seconds = parseInt(timeParts[2], 10);
      return days * 86400 + hours * 3600 + minutes * 60 + seconds;
    }}
    function formatDuration(totalSeconds) {{
      var days = Math.floor(totalSeconds / 86400);
      var remainder = totalSeconds % 86400;
      var hours = Math.floor(remainder / 3600);
      remainder %= 3600;
      var minutes = Math.floor(remainder / 60);
      var seconds = remainder % 60;

      var hh = (hours < 10 ? "0" : "") + hours;
      var mm = (minutes < 10 ? "0" : "") + minutes;
      var ss = (seconds < 10 ? "0" : "") + seconds;

      if (days > 0) {{
        var dayStr = days + " " + (days === 1 ? "day" : "days");
        return dayStr + ", " + hh + ":" + mm + ":" + ss;
      }} else {{
        return hh + ":" + mm + ":" + ss;
      }}
    }}
    function updateClock() {{
      document.querySelectorAll("duration").forEach(duration => {{
        var totalSeconds = parseDuration(duration.textContent);
        totalSeconds++;
        duration.textContent = formatDuration(totalSeconds);
      }});
    }}
    setInterval(updateClock, 1000);
  </script>
  <title>{title}</title>
</head>
<body>
  <header>
    <h1>{title}</h1>
  </header>
  <main>
    <div class="tableFixHead">
      {client_table}
    </div>
  </main>
  <footer>
    <left>&copy; {me} | {broker} | {suite} | {snodec}</left>
    <right>Online since: {since} | Elapsed: <duration>{duration}</duration></right>
  </footer>
</body>
</html>
)"";

    return fmt::format(htmlPageClientTable,
                       fmt::arg("title", "MQTTBroker | Active Clients"),
                       fmt::arg("client_table", getHTMLClientTable(mqttModel)),
                       fmt::arg("me", href("Volker Christian", "https://github.com/VolkerChristian/")),
                       fmt::arg("broker", href("MQTTBroker", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker")),
                       fmt::arg("suite", href("MQTTSuite", "https://github.com/SNodeC/mqttsuite")),
                       fmt::arg("snodec", "Powered by " + href("SNode.C", "https://github.com/SNodeC/snode.c")),
                       fmt::arg("since", mqttModel.onlineSince()),
                       fmt::arg("duration", mqttModel.onlineDuration()));
}

static std::string urlDecode(const std::string& encoded) {
    std::string decoded;
    size_t i = 0;

    while (i < encoded.length()) {
        char ch = encoded[i];
        if (ch == '%') {
            // Make sure there are at least two characters after '%'
            if (i + 2 < encoded.length() && std::isxdigit(encoded[i + 1]) && std::isxdigit(encoded[i + 2])) {
                // Convert the two hex digits to a character
                std::string hexValue = encoded.substr(i + 1, 2);
                char decodedChar = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                decoded.push_back(decodedChar);
                i += 3; // Skip over the % and the two hex digits
            } else {
                // Malformed encoding, just add the '%' as is.
                decoded.push_back(ch);
                ++i;
            }
        } else if (ch == '+') {
            // Convert '+' to space (common in URL encoding)
            decoded.push_back(' ');
            ++i;
        } else {
            // Regular character, just append it.
            decoded.push_back(ch);
            ++i;
        }
    }

    return decoded;
}

static express::Router getRouter() {
    const express::Router router;

    router.get("/clients", [] APPLICATION(req, res) {
        res->send(getHTMLPageClientTable(mqtt::mqttbroker::lib::MqttModel::instance()));
    });

    router.get("/client", [] APPLICATION(req, res) {
        std::string responseString;
        int responseStatus = 200;

        if (req->queries.size() == 1) {
            const mqtt::mqttbroker::lib::Mqtt* mqtt =
                mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(urlDecode(req->queries.begin()->first));

            if (mqtt != nullptr) {
                static constexpr std::string_view clientInformation = R""(<!DOCTYPE html>
<html>
  <head>
    <style>
      * {{
        font-family: Arial, sans-serif;
      }}
      html, body {{
        height: 100%;
        margin: 0;
        overflow: hidden;
      }}
      body {{
        display: flex;
        flex-direction: column;
      }}
      header {{
        background: #e0e0e0;
        text-align: center;
      }}
      footer {{
        background: #e0e0e0;
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px;
        box-sizing: border-box;
      }}
      main {{
        flex: 1 1 auto;
        overflow: hidden;
        box-sizing: border-box;
        padding-left: 10px;
        padding-right:  10px;
        padding-top: 20px;
        padding-bottom: 20px;
      }}
      .tableFixHead {{
        overflow: auto;
        height: 100%;
        table {{
          width: 100%;
          table-layout: fixed;
          border-collapse: collapse;
        }}
        th {{
          position: sticky;
          top: 0;
          z-index: 1;
          background-color:#e0e0e0;
        }}
        tr:nth-child(even) {{
          background-color: #f9f9f9;
        }}
        tr:hover {{
          background-color: #e0e0e0;
        }}
        th, td {{
          padding: 12px;
          box-shadow: inset 0px 0px 0px 1px #ccc, inset 0px 0px 0px 0px #ccc;
        }}
        td:nth-child(1),
        td:nth-child(2) {{
          white-space: nowrap;
        }}
      }}
    </style>
      <title>{title}</title>
  </head>
  <body>
    <header>
      <h1>
        {title}
    </h1>
    </header>
    <main>
      <div class="tableFixHead">
        <table>
          <tr>
            <th>Attribute</th>
            <th>Value</th>
          </tr>
          <tr>
            <td>Client ID</td>
            <td>{client_id}</td>
          </tr>
          <tr>
            <td>Connection</td>
            <td>{connection}</td>
          </tr>
          <tr>
            <td>Clean Session</td>
            <td>{clean_session}</td>
          </tr>
          <tr>
            <td>Connect Flags</td>
            <td>{connect_flags}</td>
          </tr>
          <tr>
            <td>Username</td>
            <td>{username}</td>
          </tr>
          <tr>
            <td>Username Flag</td>
            <td>{username_flag}</td>
          </tr>
          <tr>
            <td>Password</td>
            <td>{password}</td>
          </tr>
          <tr>
            <td>Password Flag</td>
            <td>{password_flag}</td>
          </tr>
          <tr>
            <td>Keep Alive</td>
            <td>{keep_alive}</td>
          </tr>
          <tr>
            <td>Protocol</td>
            <td>{protocol}</td>
          </tr>
          <tr>
            <td>Protocol Level</td>
            <td>{protocol_level}</td>
          </tr>
          <tr>
            <td>Loop Prevention</td>
            <td>{loop_prevention}</td>
          </tr>
          <tr>
            <td>Will Message</td>
            <td>{will_message}</td>
          </tr>
          <tr>
            <td>Will Topic</td>
            <td>{will_topic}</td>
          </tr>
          <tr>
            <td>Will QoS</td>
            <td>{will_qos}</td>
          </tr>
          <tr>
            <td>Will Flag</td>
            <td>{will_flag}</td>
          </tr>
          <tr>
            <td>Will Retain</td>
            <td>{will_retain}</td>
          </tr>
        </table>
      </div>
    </main>
  </body>
</html>)"";

                responseString = fmt::format(clientInformation,
                                             fmt::arg("title", mqtt->getClientId()),
                                             fmt::arg("client_id", mqtt->getClientId()),
                                             fmt::arg("connection", mqtt->getConnectionName()),
                                             fmt::arg("clean_session", mqtt->getCleanSession()),
                                             fmt::arg("connect_flags", mqtt->getConnectFlags()),
                                             fmt::arg("username", mqtt->getUsername()),
                                             fmt::arg("username_flag", mqtt->getUsernameFlag()),
                                             fmt::arg("password", mqtt->getPassword()),
                                             fmt::arg("password_flag", mqtt->getPasswordFlag()),
                                             fmt::arg("keep_alive", mqtt->getKeepAlive()),
                                             fmt::arg("protocol", mqtt->getProtocol()),
                                             fmt::arg("protocol_level", mqtt->getLevel()),
                                             fmt::arg("loop_prevention", !mqtt->getReflect()),
                                             fmt::arg("will_message", mqtt->getWillMessage()),
                                             fmt::arg("will_topic", mqtt->getWillTopic()),
                                             fmt::arg("will_qos", mqtt->getWillQoS()),
                                             fmt::arg("will_flag", mqtt->getWillFlag()),
                                             fmt::arg("will_retain", mqtt->getWillRetain()));
            } else {
                responseStatus = 404;
                responseString = "Not Found: " + urlDecode(req->queries.begin()->first);
            }
        } else {
            responseStatus = 400;
            responseString = "Bad Request: No Client requested";
        }

        res->status(responseStatus).send(responseString);
    });

    const express::Router& jsonRouter = express::middleware::JsonMiddleware();

    jsonRouter.post([] APPLICATION(req, res) {
        VLOG(0) << "-----------------------------\n" //
                << std::string(req->body.begin(), req->body.end());
        VLOG(0) << "-----------------------------";

        req->getAttribute<nlohmann::json>(
            [&res](nlohmann::json& json) {
                std::string jsonString = json.dump(4);
                VLOG(0) << "Application received JSON body\n" << jsonString;

                std::string connectionName = json["connection_name"].get<std::string>();
                const mqtt::mqttbroker::lib::Mqtt* mqtt = mqtt::mqttbroker::lib::MqttModel::instance().getMqtt(connectionName);

                if (mqtt != nullptr) {
                    mqtt->getMqttContext()->getSocketConnection()->close();
                    res->send(jsonString);
                } else {
                    res->status(404).send("MQTT client has already gone away: " + json["connection_name"].get<std::string>());
                }
            },
            [&res](const std::string& key) {
                VLOG(0) << "Attribute type not found: " << key;

                res->status(400).send("Attribute type not found: " + key);
            });
        VLOG(0) << "-----------------------------";
    });

    router.use("/clients", jsonRouter);

    router.get("/ws/", [] APPLICATION(req, res) {
        if (req->headers.contains("upgrade")) {
            upgrade(req, res);
        } else {
            res->redirect("/clients");
        }
    });

    router.get("/", [] APPLICATION(req, res) {
        if (req->headers.contains("upgrade")) {
            upgrade(req, res);
        } else {
            res->redirect("/clients");
        }
    });

    return router;
}

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
            break;
        case core::socket::State::DISABLED:
            VLOG(1) << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
        case core::socket::State::FATAL:
            VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
          typename SocketAddress = typename Server::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startServer(const std::string& instanceName,
                 const std::function<void(typename Server::Config&)>& configurator,
                 SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    const Server server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(server.getConfig());

    server.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketServer,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Server = SocketServer<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
          typename SocketAddress = typename Server::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<
              std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>, typename Server::Config&>>,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketServer<SocketContextFactory>::Config&>>

          >
void startServer(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    Server(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...)
        .listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
            reportState(instanceName, socketAddress, state);
        });
}

template <typename HttpExpressServer>
void startServer(const std::string& instanceName, const std::function<void(typename HttpExpressServer::Config&)>& configurator = nullptr) {
    using SocketAddress = typename HttpExpressServer::SocketAddress;

    const HttpExpressServer httpExpressServer(instanceName, getRouter());

    if (configurator != nullptr) {
        configurator(httpExpressServer.getConfig());
    }

    httpExpressServer.listen([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) {
        reportState(instanceName, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
    VLOG(0) << fmt::format("{x} + {xb}\n", fmt::arg("x", 3), fmt::arg("xb", 5));
    utils::Config::addStringOption("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::addStringOption("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::getStringOptionValue("--mqtt-session-store").data(), 0);

    startServer<net::in::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();
    });

    startServer<net::in::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();
    });

    startServer<net::in6::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtt", [](auto& config) {
        config.setPort(1883);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<net::in6::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("in6-mqtts", [](auto& config) {
        config.setPort(8883);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<net::un::stream::legacy::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("un-mqtt", [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
        config.setRetry();
    });

    startServer<net::un::stream::tls::SocketServer, mqtt::mqttbroker::SharedSocketContextFactory>("un-mqtts", [](auto& config) {
        config.setSunPath("/tmp/" + utils::Config::getApplicationName() + "-" + config.getInstanceName());
        config.setRetry();
    });

    startServer<express::legacy::in::WebApp>("in-http", [](auto& config) {
        config.setPort(8080);
        config.setRetry();
    });

    startServer<express::tls::in::WebApp>("in-https", [](auto& config) {
        config.setPort(8088);
        config.setRetry();
    });

    startServer<express::legacy::in6::WebApp>("in6-http", [](auto& config) {
        config.setPort(8080);
        config.setRetry();

        config.setIPv6Only();
    });

    startServer<express::tls::in6::WebApp>("in6-https", [](auto& config) {
        config.setPort(8088);
        config.setRetry();

        config.setIPv6Only();
    });

    return core::SNodeC::start();
}

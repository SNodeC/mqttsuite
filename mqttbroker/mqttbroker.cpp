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
#include <string>
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

static std::string getMqttClientTable(mqtt::mqttbroker::lib::MqttModel& mqttModel) {
    std::string table;

    for (const auto& [connectionName, mqttModelEntry] : mqttModel.getClients()) {
        const mqtt::mqttbroker::lib::Mqtt* mqtt = mqttModelEntry.getMqtt();
        const core::socket::stream::SocketConnection* socketConnection = mqtt->getMqttContext()->getSocketConnection();

        table += std::string("<tr>\n") +                                                                 //
                 "  <td>" + mqtt->getClientId() + "</td>\n" +                                            //
                 "  <td>" + mqttModelEntry.onlineSince() + "</td>\n" +                                   //
                 "  <td><duration-cell>" + mqttModelEntry.onlineDuration() + "</duration-cell></td>\n" + //
                 "  <td>" + mqtt->getConnectionName() + "</td>\n" +                                      //
                 "  <td>" + socketConnection->getLocalAddress().toString() + "</td>\n" +                 //
                 "  <td>" + socketConnection->getRemoteAddress().toString() + "</td>\n" +                //
                 "  <td>\n" +                                                                            //
                 "    <button onclick=\"executeCode('" + mqtt->getConnectionName() + "')\">" +           //
                 "      Disconnect" +                                                                    //
                 "    </button>\n"                                                                       //
                 "  </td>\n" +                                                                           //
                 "</tr>";
    }

    return table;
}

static express::Router getRouter() {
    const express::Router router;

    router.get("/clients", [] APPLICATION(req, res) {
        mqtt::mqttbroker::lib::MqttModel& mqttModel = mqtt::mqttbroker::lib::MqttModel::instance();

        std::string responseString = R"(
<html>
  <style>
    html, body {
      height: 100%;
      margin: 0;
      overflow: hidden; /* Prevent outer scrollbars */
    }

    /* Use the body as a flex container with column direction */
    body {
      display: flex;
      flex-direction: column;
    }

    /* The main content area grows to fill available space and scrolls if needed */
    main {
      flex: 1 1 auto;
      overflow-y: auto;
      padding: 10px;
      box-sizing: border-box;
    }

    /* The footer’s height is determined by its content */
    footer {
      background: #e0e0e0;
      font-family: Arial, sans-serif;
      display: flex;                /* Layout inner content with flexbox */
      justify-content: space-between; /* Space out left/right parts */
      align-items: center;          /* Vertically center the footer content */
      padding: 10px;                /* Padding can be adjusted as needed */
      box-sizing: border-box;
    }

    /* Additional styling for headings and tables */
    h1 {
      font-family: Arial, sans-serif;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin: 20px 0;
      font-family: Arial, sans-serif;
    }

    duration-cell {
    }

    th, td {
      padding: 12px;
      border: 1px solid #ccc;
      text-align: left;
    }

    th {
      background-color: #f4f4f4;
    }

    td:nth-child(1),
    td:nth-child(2),
    td:nth-child(3),
    td:nth-child(4) {
      white-space: nowrap;
    }

    tr:nth-child(even) {
      background-color: #f9f9f9;
    }

    tr:hover {
      background-color: #e0e0e0;
    }
  </style>
  <script>
    function executeCode(connectionName) {
      fetch("/clients/", {
        method: "POST",
        body: JSON.stringify({
          connection_name: connectionName
        }),
        headers: {
          "Content-type": "application/json; charset=UTF-8"
        }
      })
      .then(response => {
        return response.text().then(body => {
          return { status: response.status, body: body, ok: response.ok };
        });
      })
      .then(result => {
        if (!result.ok) {
          throw new Error("Network response was not ok\n" + result.status + ": " + result.body);
        }
        return result.body;
      })
      .then(body => {
        console.log("Data received:", body);
        window.location.reload();
      })
      .catch(error => {
        console.error("There was a problem with the fetch operation:", error);
        alert(error);
        window.location.reload();
      });
    }

    // Example: initial duration string. Replace this with the actual value from mqttModel.onlineDuration()
    // Examples:
    // "00:00:00" (no days)
    // "1 day, 12:34:56" (one day)
    // "2 days, 01:02:03" (multiple days)
    var initialDurationStr = ")";

        responseString += mqttModel.onlineDuration();
        responseString += R"(";
    // Parse a duration string into total seconds.
    // The format may be either "hh:mm:ss" or "x day(s), hh:mm:ss".
    function parseDuration(durationStr) {
      var days = 0;
      var timeStr = durationStr;
      if (durationStr.indexOf(",") !== -1) {
        // Split on the comma to separate the day part and the time part.
        var parts = durationStr.split(",");
        // Expecting something like "1 day" or "2 days"
        var dayPart = parts[0].trim();
        // Extract the number (assumes the first token is the day count)
        days = parseInt(dayPart.split(" ")[0], 10);
        timeStr = parts[1].trim();
      }
      var timeParts = timeStr.split(":");
      var hours = parseInt(timeParts[0], 10);
      var minutes = parseInt(timeParts[1], 10);
      var seconds = parseInt(timeParts[2], 10);
      return days * 86400 + hours * 3600 + minutes * 60 + seconds;
    }

    // Format total seconds into a string.
    // If the total seconds include one or more days, prepend "x day" or "x days, " to the time.
    function formatDuration(totalSeconds) {
      var days = Math.floor(totalSeconds / 86400);
      var remainder = totalSeconds % 86400;
      var hours = Math.floor(remainder / 3600);
      remainder %= 3600;
      var minutes = Math.floor(remainder / 60);
      var seconds = remainder % 60;

      // Format hours, minutes, and seconds with two digits.
      var hh = (hours < 10 ? "0" : "") + hours;
      var mm = (minutes < 10 ? "0" : "") + minutes;
      var ss = (seconds < 10 ? "0" : "") + seconds;

      if (days > 0) {
        // Choose singular or plural based on the day count.
        var dayStr = days + " " + (days === 1 ? "day" : "days");
        return dayStr + ", " + hh + ":" + mm + ":" + ss;
      } else {
        return hh + ":" + mm + ":" + ss;
      }
    }


    // Function to update the clock display every second.
    function updateClock() {
      // Convert the initial duration string to total seconds.
      var totalSeconds = parseDuration(document.getElementById("elapsedClock").textContent);
      totalSeconds++; // Increment the total seconds by one.
      document.getElementById("elapsedClock").textContent = formatDuration(totalSeconds);

      document.querySelectorAll("duration-cell").forEach(durationCell => {
        var totalSeconds = parseDuration(durationCell.textContent);
        totalSeconds++; // Increment the total seconds by one.
        durationCell.textContent = formatDuration(totalSeconds);
      });
    }

    // Update the clock every 1000 milliseconds (1 second).
    setInterval(updateClock, 1000);
  </script>
</head>
<body>
  <main>
    <h1>List of all connected MQTT Clients</h1>
    <table>
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
      <tbody>
)";

        // Append dynamic table rows.
        responseString += getMqttClientTable(mqttModel);

        responseString += R"(
      </tbody>
    </table>
  </main>
  <footer>
    <left>
      &copy; )";

        // Append dynamic href links.
        responseString += href("Volker Christian", "https://github.com/VolkerChristian/");
        responseString += R"( | )";
        responseString += href("MQTTBroker", "https://github.com/SNodeC/mqttsuite/tree/master/mqttbroker");
        responseString += R"( | )";
        responseString += href("MQTTSuite", "https://github.com/SNodeC/mqttsuite");
        responseString += R"( | Powered by )";
        responseString += href("SNode.C", "https://github.com/SNodeC/snode.c");
        responseString += R"(
    </left>
    <right>
      Online since: )";

        responseString += mqttModel.onlineSince();
        responseString += R"( | Elapsed: <span id="elapsedClock">)";
        responseString += mqttModel.onlineDuration();
        responseString += R"(</span>
    </right>
  </footer>
</body>
</html>
)";

        res->send(responseString);
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

<picture>
  <source media="(max-width: 600px)" srcset="docs/readme/media/hero-mobile.svg">
  <img src="docs/readme/media/hero.svg" alt="MQTTSuite — connect devices, translate messages, bridge brokers and store telemetry. Built on SNode.C.">
</picture>

# MQTTSuite

**From a device message to an integrated system.**

MQTTSuite is a set of five C++ applications for **MQTT 3.1.1**: run a broker, translate topics and payloads, connect separate brokers, publish and subscribe from the command line, or persist messages in MariaDB. Use the applications independently or combine them into a pipeline that fits your devices and existing services.

Built on [SNode.C](https://github.com/SNodeC/snode.c), the suite shares its event-driven networking and configuration model. Native MQTT and MQTT over WebSockets are available over IPv4, IPv6 and Unix-domain sockets, with plain and TLS variants according to build configuration.

[Install](docs/readme/install.md) · [Try it](#publish-your-first-message) · [Mapping](docs/readme/mapping.md) · [Bridging](docs/readme/bridging.md) · [Storage](docs/readme/storage.md) · [Deployment](docs/readme/deployment.md) · [API reference](https://snodec.github.io/mqttsuite-doc/html/index.html)

## Choose the job, choose the application

- **MQTTBroker** (`mqttbroker`) — Connect MQTT publishers and subscribers, inspect clients in the Web UI, and optionally run mappings inside the broker.
- **MQTTIntegrator** (`mqttintegrator`) — Subscribe to an existing broker, transform topics or payloads with static rules and INJA templates, and publish the result.
- **MQTTBridge** (`mqttbridge`) — Connect outward to multiple brokers and relay selected topics between them, with configured prefixes and loop prevention.
- **MQTTCli** (`mqttcli`) — Publish, subscribe and diagnose connections from a terminal or script.
- **MQTTStore** (`mqttstore`) — Store raw MQTT messages in MariaDB and optionally project JSON fields into application-owned typed tables.

MQTTIntegrator, MQTTBridge and MQTTStore connect as MQTT clients; they do not require MQTTBroker as the other endpoint. MQTTBridge is not another broker listener. Its optional `loop_prevention` setting uses a non-standard bridge flag that must be supported by the remote broker; the example below explains the distinction.

<picture>
  <source media="(max-width: 600px)" srcset="docs/readme/media/message-flow-mobile.svg">
  <img src="docs/readme/media/message-flow.svg" alt="Example message paths: devices publish to a broker; an integrator transforms and republishes, a bridge forwards to another broker, and a store persists messages in MariaDB.">
</picture>

*Choose the branches you need. These are cooperating applications, not five mandatory stages in one pipeline.*

## Publish your first message

Send a message through a broker running only on your own machine.

**You need:** installed `mqttbroker` and `mqttcli`, three terminals in the same working directory, and free loopback port **18883**. No source checkout is required for these examples.

**Configuration — save as `broker.conf`:**

<details>
<summary>Loopback-only broker configuration (all other listeners disabled)</summary>

```ini
# Local README demonstration. No public listeners or web management endpoint.
[in-mqtt]
disabled = false
[in-mqtt.local]
host = 127.0.0.1
port = 18883
[in-mqtts]
disabled = true
[in6-mqtt]
disabled = true
[in6-mqtts]
disabled = true
[un-mqtt]
disabled = true
[un-mqtts]
disabled = true
[in-http]
disabled = true
[in-https]
disabled = true
[in6-http]
disabled = true
[in6-https]
disabled = true
[un-http]
disabled = true
[un-https]
disabled = true
```

</details>

**Run — terminal 1, start the broker:**

```sh
mqttbroker --config-file broker.conf
```

**Run — terminal 2, subscribe:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false \
  remote --host 127.0.0.1 --port 18883 \
  sub --topic 'demo/#'
```

**Run — terminal 3, publish:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false \
  remote --host 127.0.0.1 --port 18883 \
  pub --topic 'demo/hello' --message 'Hello from MQTTSuite!' \
  socket --reconnect=false
```

**Expected result:** the subscriber reports topic `demo/hello` and payload `Hello from MQTTSuite!`. Stop the subscriber and broker with Ctrl+C before starting the next example.

**Boundaries:** unencrypted loopback MQTT, no credentials and no persistent sessions. `socket --reconnect=false` makes the publisher a one-shot operation. The configuration section `in-mqtt` names an SNode.C connection instance; its `remote`, `pub`, `sub` and `socket` sections configure that instance's responsibilities.

**Go further:** [deployment and access controls](docs/readme/deployment.md).

## Translate a device’s language

Not every device publishes the topic or payload your application expects. This mapping turns button events into light commands inside MQTTBroker.

**You need:** `mqttbroker`, `mqttcli` and the `broker.conf` above. Stop any previous broker on 18883.

**Configuration — save as `mapping.json`:**

```json
{
  "mapping": {
    "topic_level": {
      "name": "devices",
      "topic_level": {
        "name": "button",
        "subscription": {
          "qos": 0,
          "static": {
            "mapped_topic": "actuators/light/set",
            "message_mapping": [
              { "message": "pressed", "mapped_message": "on" },
              { "message": "released", "mapped_message": "off" }
            ]
          }
        }
      }
    }
  }
}
```

**Run — terminal 1:**

```sh
mqttbroker --config-file broker.conf broker --mqtt-mapping-file mapping.json
```

**Run — terminal 2, subscribe before publishing:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  sub --topic 'actuators/light/set'
```

**Run — terminal 3, publish:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  pub --topic 'devices/button' --message 'pressed' \
  socket --reconnect=false
```

**Expected result:**

| Input on `devices/button` | Output on `actuators/light/set` |
| --- | --- |
| `pressed` | `on` |
| `released` | `off` |

**Boundaries:** other payloads do not match these rules. Stop the processes before the next example. To map through an existing broker use MQTTIntegrator instead, with its administrative listeners explicitly disabled as shown in the guide; do not apply the same rules in both places unless duplicate outputs are intended.

**Go further:** [complete integrator and JSON-template examples](docs/readme/mapping.md).

## Bridge separate brokers

Forward selected topics between two independent brokers without changing their publishers.

**You need:** `mqttbroker`, `mqttbridge`, `mqttcli`, the `broker.conf` above, and free loopback ports **18883** and **18884**. Stop earlier demo brokers.

**Configuration — save as `bridge-demo.json`:**

<details>
<summary>Complete two-broker topology</summary>

```json
{
  "bridges": [{
    "name": "demo",
    "prefix": "relay/",
    "brokers": [
      {
        "prefix": "a/",
        "network": {
          "instance_name": "demo-a",
          "protocol": "in",
          "in": { "host": "127.0.0.1", "port": 18883 },
          "encryption": "legacy",
          "transport": "stream"
        },
        "mqtt": {
          "client_id": "readme-bridge-a",
          "clean_session": true,
          "loop_prevention": true
        },
        "topics": [{ "topic": "telemetry/#", "qos": 0 }]
      },
      {
        "prefix": "b/",
        "network": {
          "instance_name": "demo-b",
          "protocol": "in",
          "in": { "host": "127.0.0.1", "port": 18884 },
          "encryption": "legacy",
          "transport": "stream"
        },
        "mqtt": {
          "client_id": "readme-bridge-b",
          "clean_session": true,
          "loop_prevention": true
        },
        "topics": [{ "topic": "commands/#", "qos": 0 }]
      }
    ]
  }]
}
```

</details>

**Run — terminals 1 and 2, one broker in each:**

```sh
mqttbroker --config-file broker.conf
```

```sh
mqttbroker --config-file broker.conf in-mqtt local --port 18884
```

**Run — terminal 3, bridge:**

```sh
mqttbridge --config-file /dev/null \
  bridge --definition bridge-demo.json \
  admin-legacy --disabled=true admin-tls --disabled=true
```

**Run — terminal 4, subscribe on broker B:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18884 \
  sub --topic 'relay/#'
```

**Run — terminal 5, publish on broker A after the bridge and subscriber connect:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  pub --topic 'telemetry/temperature' --message '21.5' \
  socket --reconnect=false
```

**Expected result:** broker B's subscriber receives `relay/a/b/telemetry/temperature` with payload `21.5`. The rule is **bridge prefix + source prefix + destination prefix + original topic**.

<picture>
  <source media="(max-width: 600px)" srcset="docs/readme/media/bridge-topics-mobile.svg">
  <img src="docs/readme/media/bridge-topics.svg" alt="A to B: relay/ + a/ + b/ + telemetry/temperature. B to A: relay/ + b/ + a/ + commands/light. Relayed topics do not match the input subscriptions.">
</picture>

**Boundaries:** `legacy` means unencrypted transport. `loop_prevention` requests suppression of the bridge's own publications through a non-standard MQTT CONNECT bridge bit also used by Mosquitto; the remote broker must support that bit. For other brokers, disable it and design non-overlapping topic paths. This example's `relay/` outputs match neither `telemetry/#` nor `commands/#`, so they are not fed back into the bridge. MQTTBridge may normalize and rewrite `bridge-demo.json`; use this writable demo copy, not a read-only source of record. Stop all demo processes with Ctrl+C.

**Go further:** [bridge topology, sessions and deployment](docs/readme/bridging.md).

## Keep the original message—and query the useful fields

MQTTStore keeps the raw MQTT envelope and can project JSON fields into application-owned typed tables.

**You need:** installed `mqttstore`, `mqttbroker` and `mqttcli`, a local MariaDB server, and an administrative database account for setup. Start the loopback broker with `mqttbroker --config-file broker.conf` in another terminal. Replace the password below and adjust the database socket to your installation.

**Configuration — run this SQL as a MariaDB administrator:**

<details>
<summary>Database, dedicated account and typed table</summary>

```sql
CREATE DATABASE mqttsuite_demo
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'mqttstore_demo'@'localhost'
  IDENTIFIED BY 'REPLACE-WITH-A-UNIQUE-PASSWORD';
GRANT CREATE, INSERT, SELECT, INDEX ON mqttsuite_demo.*
  TO 'mqttstore_demo'@'localhost';
CREATE TABLE mqttsuite_demo.sensor_measurements (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  device_id VARCHAR(255) NOT NULL,
  value DOUBLE NOT NULL,
  unit VARCHAR(32) NULL,
  received_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
);
```

</details>

**Save as `projections.json`:**

```json
{
  "projections": [{
    "name": "temperature",
    "topic": "normalized/+/temperature",
    "table": "sensor_measurements",
    "columns": {
      "device_id": { "topic_level": 1, "required": true },
      "value": { "json_pointer": "/value", "required": true },
      "unit": { "json_pointer": "/unit" }
    }
  }]
}
```

`required: true` writes SQL NULL when the source is missing; without it, a missing source omits that column. It is not a pre-insert validation rule. Here, missing `value` conflicts with `NOT NULL`, so the typed insert fails while raw storage remains a separate operation. Topic levels are zero-based: `room1` is level 1.

**Save as `store.conf` and protect it with `chmod 600 store.conf`:**

```ini
[in-mqtt]
disabled = false
[in-mqtt.remote]
host = 127.0.0.1
port = 18883
[in-mqtt.session]
client-id = readme-store
[in-mqtt.sub]
topic = "normalized/#"
[in-mqtt.db]
socket = /run/mysqld/mysqld.sock
database = mqttsuite_demo
username = mqttstore_demo
password = REPLACE-WITH-A-UNIQUE-PASSWORD
[in-mqtt.db.storage]
raw-table = mqtt_messages
auto-create-raw-table = true
projection-file = projections.json
```

**Run — terminal 1:**

```sh
mqttstore --config-file store.conf
```

**Run — terminal 2, publish after MQTTStore connects:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  pub --topic 'normalized/room1/temperature' --message '{"value":21.5,"unit":"C"}' \
  socket --reconnect=false
```

**Verify — in a database client with read access:**

```sql
SELECT topic, payload_text, payload_format
FROM mqttsuite_demo.mqtt_messages ORDER BY id DESC LIMIT 1;
SELECT device_id, value, unit
FROM mqttsuite_demo.sensor_measurements ORDER BY id DESC LIMIT 1;
```

**Expected result:**

| Storage | Result |
| --- | --- |
| Raw row | Topic `normalized/room1/temperature`, original `{"value":21.5,"unit":"C"}` payload. |
| Typed row | `device_id = room1`, `value = 21.5`, `unit = C`. |

**Boundaries:** MQTTStore can create the raw table, but database/user provisioning and typed-table migrations are administrative tasks. Quote the INI topic filter because `#` otherwise starts a comment. Raw inserts and projections are separate operations; MQTT QoS is not an atomic database transaction guarantee. Stop MQTTStore and the broker with Ctrl+C; the database is retained.

**Go further:** [raw-only storage, permissions and database transport](docs/readme/storage.md).

## Connect it your way

| Connection | Broker instance | Client instance |
| --- | --- | --- |
| MQTT / IPv4 | `in-mqtt` | `in-mqtt` |
| MQTT + TLS / IPv4 | `in-mqtts` | `in-mqtts` |
| MQTT / IPv6 | `in6-mqtt` | `in6-mqtt` |
| MQTT + TLS / IPv6 | `in6-mqtts` | `in6-mqtts` |
| MQTT / Unix socket | `un-mqtt` | `un-mqtt` |
| MQTT + TLS / Unix socket | `un-mqtts` | `un-mqtts` |
| MQTT over WebSocket / IPv4 | `in-http` | `in-wsmqtt` |
| MQTT over secure WebSocket / IPv4 | `in-https` | `in-wsmqtts` |

WebSocket client/server variants also exist for IPv6 and Unix sockets. MQTTBridge creates its connection instances from its topology instead of using this fixed name list. Build-time selections may omit transports. WebSocket peers must agree on the `mqtt` subprotocol; certificates and trust are required for TLS/WSS.

## Install

[Choose your route →](docs/readme/install.md#choose-your-route) · [Binary packages](docs/readme/packages.md) · [Build from source](docs/readme/install.md#build-from-source) · [Deploy](docs/readme/deployment.md)

Signed packages cover **Debian, Ubuntu, Raspberry Pi OS, Rocky Linux, Fedora and OpenWrt**, with per-release and per-architecture selections. Application packages pull in the framework components they need; you do not have to build SNode.C manually when using these packages.

### Before exposing a service

MQTTBroker and MQTTBridge include browser-based management surfaces; MQTTIntegrator also starts a mapping-admin HTTP API. The integrator currently uses built-in Basic-auth credentials `admin` / `admin` and can rewrite its mapping file. Disable its `in-http` and `in-https` instances unless that API is deliberately isolated behind access controls. MQTTBroker's `in-http` / `in-https` listeners expose **both** MQTT-over-WebSocket and the client-inspection UI; they are not separate public/private listeners. See [deployment](docs/readme/deployment.md) before exposing either application.

## Learn more, contribute and license

| Task | Guide |
| --- | --- |
| Translate topics or messages | [Mapping walkthrough](docs/readme/mapping.md). |
| Link brokers | [Bridge walkthrough](docs/readme/bridging.md). |
| Store telemetry | [Storage walkthrough](docs/readme/storage.md) and the [extended MQTTStore guide](https://github.com/SNodeC/mqttsuite/blob/master/docs/mqttstore-user-guide.md). |
| Choose packages and versions | [Package matrix](docs/readme/packages.md). |
| Run unattended | [Deployment checklist](docs/readme/deployment.md). |
| Extend the applications | [MQTTSuite API reference](https://snodec.github.io/mqttsuite-doc/html/index.html) and [SNode.C](https://github.com/SNodeC/snode.c). |

[Report an issue](https://github.com/SNodeC/mqttsuite/issues) with the application, transport, version and a sanitized configuration. Small topic/payload examples make mapping and forwarding reports much easier to reproduce.

Copyright © Volker Christian and contributors. MQTTSuite is dual-licensed under **[MIT](https://github.com/SNodeC/mqttsuite/blob/master/LICENSE-MIT) OR [GPL-3.0-or-later](https://github.com/SNodeC/mqttsuite/blob/master/LICENSE-GPL-3.0-or-later)**. SNode.C and bundled dependencies retain their own licensing terms.

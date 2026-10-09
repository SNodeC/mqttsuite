# Connect two brokers

<p>
  <a href="../../README.md"><img src="media/menu/back-mqttsuite.svg" alt="← MQTTSuite" width="110" height="24"></a>
</p>

The landing page shows the basic example; this guide extends it.

MQTTBridge connects as a client to every broker in a logical bridge. Messages received from one connection are forwarded to the other connected brokers, not directly back to their origin. Subscription filters decide which messages enter the bridge; prefixes decide their destination topics.

## The local example

**You need:** MQTTBroker, MQTTBridge, MQTTCli, five terminals in the same empty working directory, and free loopback ports **18883** and **18884**. Follow the [first-run and listener defaults](../../README.md#publish-your-first-message).

The topology declares two loopback endpoints:

| Endpoint | Address | Input subscription | Prefix |
| --- | --- | --- | --- |
| Broker A | `127.0.0.1:18883` | `telemetry/#` | `a/` |
| Broker B | `127.0.0.1:18884` | `commands/#` | `b/` |

The bridge prefix is `relay/`. Both connections request clean sessions and set `loop_prevention`. That option uses the non-standard MQTT CONNECT bridge bit also used by Mosquitto to request suppression of a bridge's own publications. Verify support in the remote broker before enabling it; otherwise disable the option and keep the topic paths non-overlapping.

**Configuration — save as `bridge.json`:**

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

This is the complete topology, also available as the optional [bridge.json download](examples/bridge.json). `legacy` means unencrypted transport.

## 1. Start two independent brokers

Use the same working directory in each terminal. Stop only earlier demonstration brokers you started; if an unrelated service occupies a port, choose another port and adjust the topology and commands together.

**Run — terminal 1:**

```text
mqttbroker \
    in-mqtt \
        local --port 18883
```

**Run — terminal 2:**

```text
mqttbroker \
    in-mqtt \
        local --port 18884 \
    in-mqtts \
        local --port 18885 \
    in6-mqtt \
        local --port 18886 \
    in6-mqtts \
        local --port 18887 \
    in-http \
        local --port 18081 \
    in-https \
        local --port 18082 \
    in6-http \
        local --port 18083 \
    in6-https \
        local --port 18084 \
    un-mqtt \
        local --sun-path /tmp/readme-broker-b-un-mqtt \
    un-mqtts \
        local --sun-path /tmp/readme-broker-b-un-mqtts \
    un-http \
        local --sun-path /tmp/readme-broker-b-un-http \
    un-https \
        local --sun-path /tmp/readme-broker-b-un-https
```

The second broker uses different ports and Unix socket paths for its other listeners as well, avoiding conflicts with the first broker. Omit instance sections absent from your build (`mqttbroker --help` lists them). Neither command configures a session-store file.

## 2. Start MQTTBridge

**Run — terminal 3:**

```text
mqttbridge \
    bridge --definition bridge.json \
    admin-legacy --disabled \
    admin-tls --disabled
```

MQTTBridge can normalize and write its active definition back to `bridge.json`; keep that local file writable. The demonstration disables the bridge’s administrative HTTP listeners. Use distinct client IDs, as the sample does. An existing client with the same ID on a broker can be disconnected by a new connection.

## 3. Observe broker B, then publish on A

**Run — terminal 4:**

```text
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18884 \
        sub --topic 'relay/#'
```

**Run — terminal 5, after the bridge and subscriber connect:**

```text
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        pub --topic 'telemetry/temperature' --message '21.5' \
        socket --reconnect=false
```

**Expected result:** on broker B, `relay/a/b/telemetry/temperature` with payload `21.5`.

The topic is formed as **bridge prefix + origin broker prefix + destination broker prefix + original topic**. In the reverse direction, `commands/light` published on B becomes `relay/b/a/commands/light` on A.

<picture>
  <source media="(max-width: 600px)" srcset="media/bridge-topics-mobile.svg">
  <img src="media/bridge-topics.svg" alt="The bridge composes destination topics from the relay, origin and destination prefixes; output topics do not match its input subscriptions.">
</picture>

## Deploy a deliberate topology

**Boundaries:** the sample’s `relay/…` outputs do not match either input subscription. Keep that separation, or design a similarly explicit namespace, when adding brokers. Built-in loop prevention is not a license to connect arbitrary overlapping bridges and wildcard subscriptions without analyzing message paths.

For off-host endpoints, configure TLS and appropriate broker access policy. A bridge is not broker clustering, consensus or an exactly-once end-to-end transaction mechanism. Plan reconnect behavior, retained messages, subscription QoS and persistent sessions around your workload.

Stop all demonstration processes with Ctrl+C.

**Go further:** [deployment](deployment.md) for unattended operation. Reference: the [bridge schema](https://github.com/SNodeC/mqttsuite/blob/master/mqttbridge/lib/bridge-schema.json) lists additional network and MQTT options.

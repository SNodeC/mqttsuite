# Map messages into the form your application needs

[← MQTTSuite](../../README.md)

The landing page shows the basic example; this guide extends it.

This walkthrough maps `pressed` / `released` on `devices/button` to `on` / `off` on `actuators/light/set`.

**You need:** MQTTBroker, MQTTCli, three terminals in the same empty working directory, and free loopback port **18883**. Save `broker.conf` from the [complete loopback configuration](../../README.md#publish-your-first-message) in that directory. No source checkout is needed.

## 1. Load the mapping

**Configuration — `mapping.json`:** save the JSON from [Translate a device’s language](../../README.md#translate-a-devices-language) as `mapping.json` beside `broker.conf`. It is also available as an optional [download](examples/mapping.json). Its topic-level tree describes the input topic; `subscription.static.message_mapping` contains the input/output pairs.

For the shortest demonstration, run the mapping inside MQTTBroker:

**Run — terminal 1:**

```sh
mqttbroker --config-file broker.conf \
  broker --mqtt-mapping-file mapping.json
```

Stop any earlier demo broker first: this command uses the same loopback port, 18883.

## 2. Observe the destination

**Run — terminal 2:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false \
  remote --host 127.0.0.1 --port 18883 \
  sub --topic 'actuators/light/set'
```

## 3. Send a device event

**Run — terminal 3, after the subscriber connects:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false \
  remote --host 127.0.0.1 --port 18883 \
  pub --topic 'devices/button' --message 'pressed' \
  socket --reconnect=false
```

**Expected result:** topic `actuators/light/set`, payload `on`. Publish `released` to get `off`.

**Boundaries:** unlisted input values do not match either static rule. This is an unencrypted loopback demonstration. Stop only your demonstration processes with Ctrl+C before the next example; do not stop an unrelated service occupying a port.

**Go further:** [use a separate integrator](#use-a-separate-integrator).

## Use a separate integrator

Map through an existing broker without adding rules to that broker.

**You need:** MQTTIntegrator, MQTTCli, and the `broker.conf` and `mapping.json` saved above. Stop your mapping-enabled demo broker before starting the plain one below. All terminals use the same working directory.

**Configuration — `integrator.conf`:** the following uses your local `mapping.json`. MQTTIntegrator's administrative API can rewrite this active file when enabled. Explicitly disabling unused connection instances also prevents their required remote-address options from affecting the example.

```ini
[integrator]
mqtt-mapping-file = mapping.json
[in-mqtt]
disabled = false
[in-mqtt.remote]
host = 127.0.0.1
port = 18883
[in-mqtt.session]
client-id = readme-integrator
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
[in-wsmqtt]
disabled = true
[in-wsmqtts]
disabled = true
[in6-wsmqtt]
disabled = true
[in6-wsmqtts]
disabled = true
[un-wsmqtt]
disabled = true
[un-wsmqtts]
disabled = true
[in-http]
disabled = true
[in-https]
disabled = true
```

**Run — terminal 1:**

```sh
mqttbroker --config-file broker.conf
```

**Run — terminal 2:**

```sh
mqttintegrator --config-file integrator.conf
```

**Run — terminals 3 and 4:** leave the integrator running, then use the subscriber and publisher commands in steps 2 and 3 above. Wait for both the integrator and subscriber to connect before publishing.

**Expected result:** `pressed` produces `on` on `actuators/light/set` through the external integrator.

**Boundaries:** the integrator's admin API currently uses built-in Basic-auth credentials `admin` / `admin`; keep it disabled unless deliberately isolated. Do not run the same mapping in both broker and integrator unless duplicate outputs are intended. Stop the integrator with Ctrl+C.

**Go further:** [configuration and management access](deployment.md#configuration).

## Move from lookup rules to templates

Turn structured sensor data into a compact summary.

**You need:** MQTTBroker, MQTTCli and the loopback `broker.conf`. Stop the previous broker and integrator first.

**Configuration — save as `template.json`:**

```json
{
  "mapping": {
    "topic_level": {
      "name": "sensors",
      "topic_level": {
        "name": "room1",
        "subscription": {
          "qos": 0,
          "json": {
            "mapped_topic": "normalized/room1/summary",
            "mapping_template": "T={{ message.temperature }};H={{ message.humidity }}"
          }
        }
      }
    }
  }
}
```

**Run — terminal 1:**

```sh
mqttbroker --config-file broker.conf \
  broker --mqtt-mapping-file template.json
```

**Run — terminal 2:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  sub --topic 'normalized/room1/summary'
```

**Run — terminal 3, after the subscriber connects:**

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false remote --host 127.0.0.1 --port 18883 \
  pub --topic 'sensors/room1' --message '{"temperature":21.5,"humidity":48}' \
  socket --reconnect=false
```

**Expected result:** topic `normalized/room1/summary`, payload `T=21.5;H=48`.

**Boundaries:** `json` selects parsed-JSON input; the output template may produce text. Use `value` for scalar/text input and `static` for exact matches. Stop demo processes with Ctrl+C.

The schema also supports mapping arrays, output QoS/retain settings, delay and suppression options, nested topic levels and plugin registration. Match input and output topics carefully to avoid feeding mapped output back into the same rule.

**Go further:** [deployment](deployment.md). Reference: [mapping schema](https://github.com/SNodeC/mqttsuite/blob/master/lib/mapping-schema.json) and [mapping implementation](https://github.com/SNodeC/mqttsuite/blob/master/lib/MqttMapper.cpp).

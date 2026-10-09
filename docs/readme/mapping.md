# Map messages into the form your application needs

<p>
  <a href="../../README.md"><img src="media/menu/back-mqttsuite.svg" alt="← MQTTSuite" width="110" height="24"></a>
</p>

The landing page shows the basic example; this guide extends it.

This walkthrough maps `pressed` / `released` on `devices/button` to `on` / `off` on `actuators/light/set`.

**You need:** MQTTBroker, MQTTCli, three terminals in the same empty working directory, and free loopback port **18883**. Follow the [first-run and listener defaults](../../README.md#publish-your-first-message). No source checkout is needed.

## 1. Load the mapping

**Configuration — `mapping.json`:** save the JSON from [Translate a device’s language](../../README.md#translate-a-devices-language) as `mapping.json` in your working directory. It is also available as an optional [download](examples/mapping.json). Its topic-level tree describes the input topic; `subscription.static.message_mapping` contains the input/output pairs.

For the shortest demonstration, run the mapping inside MQTTBroker:

**Run — terminal 1:**

```sh
mqttbroker \
    in-mqtt \
        local --port 18883 \
    broker --mqtt-mapping-file mapping.json
```

Stop any earlier demo broker first: this command uses the same loopback port, 18883.

## 2. Observe the destination

**Run — terminal 2:**

```sh
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        sub --topic 'actuators/light/set'
```

## 3. Send a device event

**Run — terminal 3, after the subscriber connects:**

```sh
mqttcli \
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

**You need:** MQTTIntegrator, MQTTCli, and the `mapping.json` saved above. Stop your mapping-enabled demo broker before starting the plain one below. All terminals use the same working directory.

The command below enables only the IPv4 MQTT connection. Other outgoing connections remain disabled by default; both administrative listeners are explicitly disabled. Check that `mqttintegrator --help` lists outgoing connections as disabled; older binaries with enabled defaults need those unused connections disabled explicitly.

**Run — terminal 1:**

```sh
mqttbroker \
    in-mqtt \
        local --port 18883
```

**Run — terminal 2:**

```sh
mqttintegrator \
    integrator --mqtt-mapping-file mapping.json \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        session --client-id readme-integrator \
    in-http --disabled \
    in-https --disabled
```

**Run — terminals 3 and 4:** leave the integrator running, then use the subscriber and publisher commands in steps 2 and 3 above. Wait for both the integrator and subscriber to connect before publishing.

**Expected result:** `pressed` produces `on` on `actuators/light/set` through the external integrator.

**Boundaries:** the integrator's admin API currently uses built-in Basic-auth credentials `admin` / `admin`; keep it disabled unless deliberately isolated. Do not run the same mapping in both broker and integrator unless duplicate outputs are intended. Stop the integrator with Ctrl+C.

**Go further:** [configuration and management access](deployment.md#configuration).

## Move from lookup rules to templates

Turn structured sensor data into a compact summary.

**You need:** MQTTBroker, MQTTCli and a free loopback port **18883**. Stop the previous broker and integrator first.

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
mqttbroker \
    in-mqtt \
        local --port 18883 \
    broker --mqtt-mapping-file template.json
```

**Run — terminal 2:**

```sh
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        sub --topic 'normalized/room1/summary'
```

**Run — terminal 3, after the subscriber connects:**

```sh
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        pub --topic 'sensors/room1' --message '{"temperature":21.5,"humidity":48}' \
        socket --reconnect=false
```

**Expected result:** topic `normalized/room1/summary`, payload `T=21.5;H=48`.

**Boundaries:** `json` selects parsed-JSON input; the output template may produce text. Use `value` for scalar/text input and `static` for exact matches. Stop demo processes with Ctrl+C.

The schema also supports mapping arrays, output QoS/retain settings, delay and suppression options, nested topic levels and plugin registration. Match input and output topics carefully to avoid feeding mapped output back into the same rule.

**Go further:** [deployment](deployment.md). Reference: [mapping schema](https://github.com/SNodeC/mqttsuite/blob/master/lib/mapping-schema.json) and [mapping implementation](https://github.com/SNodeC/mqttsuite/blob/master/lib/MqttMapper.cpp).

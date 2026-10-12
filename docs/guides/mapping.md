<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-guides-compact.svg"><img src="../media/nav/banner-guides.svg" alt="MQTTSuite — Guides" width="100%"></picture></p>
<p><a href="README.md#page-overview" title="Up to Guides"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-guides-dark.svg"><img src="../media/nav/up-guides-light.svg" alt="Up to Guides" title="Up to Guides" width="160" height="28"></picture></a><a href="https://github.com/SNodeC" title="SNodeC organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNodeC organization" title="SNodeC organization"></picture></a><a href="../../README.md#project-overview" title="MQTTSuite repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="MQTTSuite repository" title="MQTTSuite repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Map messages into the form your application needs

The landing page shows the basic example; this guide extends it.

Run [Translate a device’s language](../../README.md#translate-a-devices-language) first and keep its `mapping.json`.
## Use a separate integrator

Map through an existing broker without adding rules to that broker.

**You need:** MQTTIntegrator, MQTTCli, and the `mapping.json` from the README. Stop your mapping-enabled demo broker before starting the plain one below. All terminals use the same working directory.

The command below enables only the IPv4 MQTT connection. Other outgoing connections remain disabled by default; both administrative listeners are explicitly disabled. Check that `mqttintegrator --help` lists outgoing connections as disabled; older binaries with enabled defaults need those unused connections disabled explicitly.

**Run — terminal 1:**

```text
mqttbroker \
    in-mqtt \
        local --port 18883
```

**Run — terminal 2:**

```text
mqttintegrator \
    integrator --mqtt-mapping-file mapping.json \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        session --client-id readme-integrator \
    in-http --disabled \
    in-https --disabled
```

**Run — terminals 3 and 4:** leave the integrator running, then use the subscriber and publisher commands in [the README mapping example](../../README.md#translate-a-devices-language). Wait for both the integrator and subscriber to connect before publishing.

**Expected result:** `pressed` produces `on` on `actuators/light/set` through the external integrator.

**Boundaries:** the integrator's admin API currently uses built-in Basic-auth credentials `admin` / `admin`; keep it disabled unless deliberately isolated. Do not run the same mapping in both broker and integrator unless duplicate outputs are intended. Stop the integrator with Ctrl+C.

Next: [configuration and management access](deployment.md#configuration).

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

```text
mqttbroker \
    in-mqtt \
        local --port 18883 \
    broker --mqtt-mapping-file template.json
```

**Run — terminal 2:**

```text
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        sub --topic 'normalized/room1/summary'
```

**Run — terminal 3, after the subscriber connects:**

```text
mqttcli \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        pub --topic 'sensors/room1' --message '{"temperature":21.5,"humidity":48}' \
        socket --reconnect=false
```

**Expected result:** topic `normalized/room1/summary`, payload `T=21.5;H=48`.

**Boundaries:** `json` selects parsed-JSON input; the output template may produce text. Use `value` for scalar/text input and `static` for exact matches. Stop demo processes with Ctrl+C.

The schema also supports mapping arrays, output QoS/retain settings, delay and suppression options, nested topic levels and plugin registration. Match input and output topics carefully to avoid feeding mapped output back into the same rule.

Next: [deployment](deployment.md#page-overview), [mapping schema](../../lib/mapping-schema.json), [mapping implementation](../../lib/MqttMapper.cpp).

## Mapping field reference

The [mapping schema](../../lib/mapping-schema.json) defines the shape; `MqttMapper.cpp` owns rendering and emission behavior.

| Field | Meaning |
| --- | --- |
| `mapping.topic_level` | One topic-level node or an array; `name` matches a literal or MQTT wildcard, with nested `topic_level` or a terminal `subscription` |
| `subscription.qos` | Requested input-subscription QoS (default 0), separate from output QoS |
| `subscription.static` | One static mapping or array; `message_mapping` matches an input message and supplies `mapped_message` |
| `subscription.value` / `json` | One template mapping or array; `mapping_template` renders the output payload from value/JSON context |
| `mapped_topic` | Required destination topic/template |
| `qos`, `retain` | Output publish policy; defaults 0 and false, not implicit copies of input values |
| `delay` | `-1` emits immediately; non-negative values use the delayed-publish path in seconds |
| `suppressions` | Exact rendered-payload strings to skip for template mappings; an empty retained output is still emitted so retained state can be cleared |
| `mapping.plugins` | Plugin-library paths loaded for registered template callbacks; use trusted local libraries only |

Arrays let one input produce multiple outputs. A template render error is logged rather than a successful publish; inspect logs and verify every expected output. Delay/suppression and output QoS do not turn mapping into an atomic or exactly-once transaction. Keep the complete static and JSON examples above as your first tests before extending rules.

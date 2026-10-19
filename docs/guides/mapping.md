<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-guides-compact.svg"><img src="../media/nav/banner-guides.svg" alt="MQTTSuite — Guides" width="100%"></picture></p>
<p><a href="README.md#banner" title="Up to Guides"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-guides-dark.svg"><img src="../media/nav/up-guides-light.svg" alt="Up to Guides" title="Up to Guides" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/account-dark.svg"><img align="right" src="../media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#banner" title="MQTTSuite repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="MQTTSuite repository" title="MQTTSuite repository"></picture></a><br clear="all"></p>
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

The integrator admin API uses built-in credentials `admin` / `admin`; leave it disabled unless access is restricted. Run a mapping in either the broker or the integrator to avoid duplicate outputs.

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

`json` selects parsed-JSON input; the output template may produce text. Use `value` for scalar/text input and `static` for exact matches.

The schema also supports mapping arrays, output QoS/retain settings, delay and suppression options, nested topic levels and plugin registration. Match input and output topics carefully to avoid feeding mapped output back into the same rule.

Next: [deployment](deployment.md#banner), [mapping schema](../../lib/mapping-schema.json), [mapping implementation](../../lib/MqttMapper.cpp).

## Mapping field reference

MQTTIntegrator ships a web interface for editing, validating, deploying and rolling back mappings, served at `/ui/index.html` on the admin router and protected by HTTP Basic authentication; see its [sources and build instructions](../../mqttintegrator/ui/README.md#banner).

The [mapping schema](../../lib/mapping-schema.json) defines the available fields.

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

Arrays let one input produce multiple outputs. If template rendering fails, the error is logged and that output is not published.

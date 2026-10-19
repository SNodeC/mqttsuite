<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-guides-compact.svg"><img src="../media/nav/banner-guides.svg" alt="MQTTSuite — Guides" width="100%"></picture></p>
<p><a href="README.md#banner" title="Up to Guides"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-guides-dark.svg"><img src="../media/nav/up-guides-light.svg" alt="Up to Guides" title="Up to Guides" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/account-dark.svg"><img align="right" src="../media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#banner" title="MQTTSuite repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="MQTTSuite repository" title="MQTTSuite repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Connect two brokers

Extend the README topology with TLS, session settings and additional brokers.

MQTTBridge connects as a client to every broker in a logical bridge. Messages received from one connection are forwarded to the other connected brokers, not directly back to their origin. Subscription filters decide which messages enter the bridge; prefixes decide their destination topics.

## Start with the working topology

Start with the [two-broker walkthrough](../../README.md#bridge-separate-brokers) for configuration, commands and expected messages.

In the reverse direction, `commands/light` published on broker B becomes `relay/b/a/commands/light` on A. Preserve the rule **bridge prefix + origin prefix + destination prefix + original topic** when designing both directions.

## Topology and delivery fields

These fields are defined by [the bridge schema](../../mqttbridge/lib/bridge-schema.json) and composed in `BridgeStore.cpp`. Per-bridge MQTT clients are separate from the bridge administration listeners.

| Scope / field | Meaning |
| --- | --- |
| `bridge.name`, `bridge.prefix` | Name and output prefix for one logical bridge |
| `broker.network.instance_name` | Unique local connection label; full instance name is `bridge-name+instance_name` |
| `broker.network.protocol` | `in`, `in6`, `un`, `rc` or `l2`; supply the matching address object |
| `broker.network.encryption` / `transport` | `legacy` or `tls`; `stream` or `websocket`, subject to built components |
| `broker.topics[].topic`, `qos` | Input subscription filter and requested subscription QoS (0–2) |
| `broker.prefix` | Origin/destination topic prefix, combined with the bridge prefix |
| `broker.mqtt.client_id`, `clean_session` | Identity and MQTT session policy; use a stable unique ID when deliberately retaining a broker-side session |
| `broker.session_store` | Local session-store path; keep it distinct per connection and writable by the bridge service account |
| `broker.mqtt.keep_alive` | MQTT keepalive seconds (schema default 60) |
| `broker.mqtt.username`, `password` | Credentials sent to the remote broker; that broker owns authentication |
| `broker.mqtt.will_*` | Broker-side last-will topic, payload, QoS and retain policy |
| `broker.mqtt.loop_prevention` | Requests the non-standard CONNECT bridge bit (e.g. Mosquitto); requires broker support and does not make arbitrary topologies safe |

The bridge forwards the received payload, QoS and retain flag to every connected destination except the origin. Retained messages can therefore populate retained topics on destination brokers. Configure broker-side persistent sessions with `clean_session: false`; configure local session storage separately.

## Encrypted remote-broker connection

For an IPv4 TLS stream, change that broker's `network.encryption` to `tls`, keep `transport: "stream"`, and provide the real remote host/port under `network.in`. The topology schema contains transport selection, not PEM credentials. Configure its dynamically named connection through the runtime interface after selecting the topology:

```sh
mqttbridge bridge --definition bridge.json 'demo+a' --help=expanded
mqttbridge bridge --definition bridge.json 'demo+a' tls --ca-cert /path/to/trusted-ca.pem
```

For example, replace only that broker's `network` object (the hostname is a placeholder, not an existing service):

```json
{
  "instance_name": "a",
  "protocol": "in",
  "encryption": "tls",
  "transport": "stream",
  "in": { "host": "REPLACE_WITH_BROKER_DNS", "port": 8883 }
}
```

Here `demo+a` combines `bridge.name: "demo"` and `network.instance_name: "a"`; adjust it for your topology. Use a hostname matching the server certificate and a trusted CA. Add client certificate/key options if the broker requires mutual TLS, then save the connection settings in a protected configuration file.

## Administration

MQTTBridge ships a web interface for editing its configuration; its [sources and build instructions](../../mqttbridge/ui/README.md#banner) live in `mqttbridge/ui/`.

`admin-legacy` (8081) and `admin-tls` (8082) expose the mutable configuration API, including `/config`, without built-in authentication. Disable both when not used; otherwise isolate/authenticate them outside the application. Do not expose administration merely to establish encrypted MQTT connections. See [deployment](deployment.md#banner).

## Extend the topology

When adding brokers, keep output topics outside the input subscriptions to prevent feedback, as the sample does with `relay/…`.

For remote brokers, configure TLS and broker access credentials. Choose subscriptions, QoS and persistent-session settings for the messages you need to forward.

Stop all demonstration processes with Ctrl+C.

Next: [deployment](deployment.md#banner) or [the bridge JSON schema](../../mqttbridge/lib/bridge-schema.json).

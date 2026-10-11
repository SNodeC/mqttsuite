<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../README.md#project-overview" title="MQTTSuite repository"><img src="media/page-banner.svg" alt="MQTTSuite repository" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# Connect two brokers

Extend the working README topology with deliberate delivery and deployment choices.

MQTTBridge connects as a client to every broker in a logical bridge. Messages received from one connection are forwarded to the other connected brokers, not directly back to their origin. Subscription filters decide which messages enter the bridge; prefixes decide their destination topics.

## Start with the working topology

Run the complete [two-broker walkthrough](../../README.md#bridge-separate-brokers), which owns the configuration, five terminal commands and expected topic. This guide explains extending that topology; it does not keep a second copy of the procedure.

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

`Bridge::publish` forwards the received payload, QoS and retain flag to every currently connected destination except the origin. Received retained messages can therefore seed a destination retained topic; choose subscriptions and namespaces deliberately. Broker-side persistent sessions (`clean_session: false`) and local session storage are separate choices, not an end-to-end durable transaction guarantee. Test disconnect/reconnect and retained replay against your actual brokers before deployment.

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

Here `demo+a` follows `bridge.name: "demo"` and `network.instance_name: "a"`; replace it for your topology. Use a remote hostname matching the server certificate, keep peer verification enabled, and add client certificate/key options only if that broker requires mutual TLS. Check the selected instance's help for available options and persist settings in a protected file. Never use demonstration certificates or accept-unknown trust bypasses in production. The local two-broker walkthrough remains the runnable first success; an off-host variant requires your own broker, trusted certificate and access policy.

## Administration is a separate boundary

`admin-legacy` (8081) and `admin-tls` (8082) expose the mutable configuration API, including `/config`, without built-in authentication. Disable both when not used; otherwise isolate/authenticate them outside the application. Do not expose administration merely to establish encrypted MQTT connections. See [deployment](deployment.md#page-overview).

## Deploy a deliberate topology

The sample’s `relay/…` outputs do not match either input subscription. Keep that separation, or design a similarly explicit namespace, when adding brokers. Built-in loop prevention is not a license to connect arbitrary overlapping bridges and wildcard subscriptions without analyzing message paths.

For off-host endpoints, configure TLS and appropriate broker access policy. A bridge is not broker clustering, consensus or an exactly-once end-to-end transaction mechanism. Plan reconnect behavior, retained messages, subscription QoS and persistent sessions around your workload.

Stop all demonstration processes with Ctrl+C.

[![deployment](media/menu/further-deployment.svg)](deployment.md#page-overview) [![bridge schema](media/menu/further-bridge-schema.svg)](../../mqttbridge/lib/bridge-schema.json)

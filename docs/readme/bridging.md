<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../README.md#project-overview" title="MQTTSuite repository"><img src="media/page-banner.svg" alt="MQTTSuite documentation" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# Connect two brokers

<p>
  <a href="../../README.md#project-overview"><img src="media/menu/back-mqttsuite.svg" alt="MQTTSuite" width="110" height="24"></a>
</p>

Extend the working README topology with deliberate delivery and deployment choices.

MQTTBridge connects as a client to every broker in a logical bridge. Messages received from one connection are forwarded to the other connected brokers, not directly back to their origin. Subscription filters decide which messages enter the bridge; prefixes decide their destination topics.

## Start with the working topology

Run the complete [two-broker walkthrough](../../README.md#bridge-separate-brokers), which owns the configuration, five terminal commands and expected topic. This guide explains extending that topology; it does not keep a second copy of the procedure.

In the reverse direction, `commands/light` published on broker B becomes `relay/b/a/commands/light` on A. Preserve the rule **bridge prefix + origin prefix + destination prefix + original topic** when designing both directions.

## Deploy a deliberate topology

**Boundaries:** the sample’s `relay/…` outputs do not match either input subscription. Keep that separation, or design a similarly explicit namespace, when adding brokers. Built-in loop prevention is not a license to connect arbitrary overlapping bridges and wildcard subscriptions without analyzing message paths.

For off-host endpoints, configure TLS and appropriate broker access policy. A bridge is not broker clustering, consensus or an exactly-once end-to-end transaction mechanism. Plan reconnect behavior, retained messages, subscription QoS and persistent sessions around your workload.

Stop all demonstration processes with Ctrl+C.

[![deployment](media/menu/further-deployment.svg)](deployment.md#page-overview) [![bridge schema](media/menu/further-bridge-schema.svg)](../../mqttbridge/lib/bridge-schema.json)

Deployment is for unattended operation. The bridge schema lists additional network and MQTT options.

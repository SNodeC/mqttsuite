# Operate MQTTSuite deliberately

[← MQTTSuite](../../README.md)

The examples use loopback, disposable client IDs and small topic filters. A deployment also needs explicit network access, credentials, state ownership and recovery policy.

## Configuration

Each application exposes SNode.C’s configuration system. Application-wide sections, connection instances and protocol sections have different scopes:

```sh
mqttbroker broker --help
mqttbridge bridge --help
mqttcli in-mqtt --disabled=false --help=expanded
snodec-control --target "$(command -v mqttbroker)" --ui
```

The terminal UI requires Curses support. `--write-config file.conf` saves configuration and exits; `--config-file file.conf` loads it. Do not mistake saving for starting a service. Store credentials in permission-restricted files rather than command-line arguments; configuration dumps can reveal those credentials too.

## Broker and management listeners

Enable only the transport instances you use and bind them deliberately. The sample [broker.conf](examples/broker.conf) disables every listener except loopback MQTT. An installed default configuration may differ.

To inspect the Web UI locally, save the full configuration from [Publish your first message](../../README.md#publish-your-first-message) as `broker.conf` in an empty directory. With ports **18883** and **18080** free, run from that directory:

```sh
mqttbroker --config-file broker.conf \
  in-http --disabled=false local --host 127.0.0.1 --port 18080
```

Open `http://127.0.0.1:18080/`. The packaged/installed web assets must be available at the application’s HTML root. Keep administrative surfaces private or place them behind deliberately configured access controls; do not assume the demonstration setup provides an authenticated public management service.

MQTTBroker's `in-http` and `in-https` instances serve both MQTT-over-WebSocket and the client-inspection UI. Exposing one also exposes the other unless you add explicit access policy in front of it.

MQTTIntegrator starts its own mapping-admin API on `in-http` (default port 8085) and `in-https` (8086). Its current built-in Basic-auth credentials are **`admin` / `admin`**; the application passes those defaults directly to its router, without a dedicated CLI credential option. The API can deploy mappings and write the active mapping file. Disable both instances when not needed, as in the [integrator example](mapping.md#use-a-separate-integrator). If you enable the API, bind it to loopback and place any remote access behind separately enforced authentication and TLS; do not expose the default credentials to an untrusted network.

For MQTT over WebSockets, use a client advertising the **`mqtt`** subprotocol. TLS/WSS requires certificates, private keys and peer-trust settings. Do not ship the repository’s demo certificates as production identity material.

## State and delivery boundaries

| State | Responsibility |
| --- | --- |
| MQTT sessions | Configure session persistence where needed; clean-session behavior is a separate choice. |
| Broker session store | `broker --mqtt-session-store /path/to/session.store`; give the service account write access and back it up appropriately. |
| Mapping definitions | Validate against the mapping schema; keep source and output topic spaces intentional. |
| Bridge topology | Use unique client IDs, narrow subscriptions, explicit prefixes and loop prevention. |
| MariaDB | Provision users/schema, migrations, capacity, retention and backups separately. |
| Typed projections | Raw messages and projection inserts are separate operations; monitor failures rather than assuming end-to-end atomicity. |

## Service supervision

Under systemd or a container supervisor, run the process in the foreground with an explicit configuration path and a dedicated account. Do not combine daemon mode with a supervisor expecting a foreground process. Service filenames and paths depend on how the application was installed; inspect the package rather than assuming every platform ships the same unit.

On OpenWrt, after configuring `/etc/snode.c/mqttbroker.conf`, the packaged init script can be used:

```sh
/etc/init.d/mqttbroker enable
/etc/init.d/mqttbroker start
```

Confirm the intended listeners and logs before opening firewall access. See the matching [distribution guide](packages.md) for platform-specific setup and updates.

## Before an upgrade

- Keep framework libraries, application libraries, executables and WebSocket plugins ABI-compatible.
- Back up configurations, mapping/bridge definitions, session data and databases.
- Verify reconnect behavior with the actual brokers and network conditions.
- Set appropriate connection/write-queue limits and logging levels. Trace-level payload logging is for diagnosis, not a mandatory operating mode.
- Re-run a representative publish → map/bridge/store path after upgrading. MQTT QoS does not replace application-level verification of the final effect.

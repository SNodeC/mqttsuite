# Operate MQTTSuite deliberately

<p>
  <a href="../../README.md"><img src="media/menu/back-mqttsuite.svg" alt="← MQTTSuite" width="110" height="24"></a>
</p>

The examples use loopback, disposable client IDs and small topic filters. A deployment also needs explicit network access, credentials, state ownership and recovery policy.

## Configuration

Each application exposes SNode.C’s configuration system. Application-wide sections, connection instances and protocol sections have different scopes:

```text
mqttbroker broker --help
mqttbridge bridge --help
mqttcli in-mqtt --help=expanded
snodec-control --target "$(command -v mqttbroker)" --ui
```

The terminal UI requires Curses support. Append `-w` to a working command to save persistent settings to the default configuration file and exit. Run the application without arguments as the same user to load those settings. `--write-config file.conf` saves to a chosen file; `--config-file file.conf` loads that file. These files use INI-style dotted option names:

```ini
in-mqtt.disabled=false
in-mqtt.remote.host="127.0.0.1"
in-mqtt.remote.port=18883
```

These keys describe a client connection. Use the application's `-w` output when preparing a persistent configuration. Do not mistake saving for starting a service. Store credentials in permission-restricted files rather than command-line arguments; configuration dumps can reveal those credentials too.

## Broker and management listeners

MQTTBroker enables its compiled listeners by default. Bind the listeners you use deliberately and disable the others. The [loopback example](../../README.md#publish-your-first-message) shows a first-run command; other compiled listeners remain enabled with their defaults.

To inspect the Web UI locally, start without a saved configuration and with ports **18883** and **18080** free:

```text
mqttbroker \
    in-mqtt \
        local --port 18883 \
    in-http \
        local --port 18080
```

Open `http://127.0.0.1:18080/`. The packaged/installed web assets must be available at the application’s HTML root. Keep administrative surfaces private or place them behind deliberately configured access controls; do not assume the demonstration setup provides an authenticated public management service.

MQTTBroker's `in-http` and `in-https` instances serve both MQTT-over-WebSocket and the client-inspection UI. Exposing one also exposes the other unless you add explicit access policy in front of it.

MQTTIntegrator starts its own mapping-admin API on `in-http` (default port 8085) and `in-https` (8086). Its current built-in Basic-auth credentials are **`admin` / `admin`**; the application passes those defaults directly to its router, without a dedicated CLI credential option. The API can deploy mappings and write the active mapping file. Disable both instances when not needed, as in the [integrator example](mapping.md#use-a-separate-integrator). If you enable the API, bind it to loopback and place any remote access behind separately enforced authentication and TLS; do not expose the default credentials to an untrusted network.

For MQTT over WebSockets, use a client advertising the **`mqtt`** subprotocol. TLS/WSS requires certificates, private keys and peer-trust settings. Do not ship the repository’s demo certificates as production identity material.

## State and delivery boundaries

- **MQTT sessions.** Configure session persistence where needed; clean-session behavior is a separate choice.
- **Broker session store.** Set `--mqtt-session-store` in the `broker` section; give the service account write access and back up the store.
- **Mapping definitions.** Validate against the mapping schema; keep source and output topic spaces intentional.
- **Bridge topology.** Use unique client IDs, narrow subscriptions, explicit prefixes and loop prevention.
- **MariaDB.** Provision users/schema, migrations, capacity, retention and backups separately.
- **Typed projections.** Raw messages and projection inserts are separate operations; monitor failures rather than assuming end-to-end atomicity.

## Service supervision

Under systemd or a container supervisor, run the process in the foreground with an explicit configuration path and a dedicated account. Do not combine daemon mode with a supervisor expecting a foreground process. DEB/RPM packages currently do not ship systemd units; create a unit for the application and configuration you intend to run. OpenWrt packages include procd scripts for MQTTBroker, MQTTBridge and MQTTIntegrator.

On OpenWrt, after configuring `/etc/snode.c/mqttbroker.conf`, the packaged init script can be used:

```text
/etc/init.d/mqttbroker enable
/etc/init.d/mqttbroker start
```

Confirm the intended listeners and logs before opening firewall access. See [Packages](https://github.com/SNodeC/Packages#readme) for platform-specific setup and updates.

## Before an upgrade

- Keep framework libraries, application libraries, executables and WebSocket plugins ABI-compatible.
- Back up configurations, mapping/bridge definitions, session data and databases.
- Verify reconnect behavior with the actual brokers and network conditions.
- Set appropriate connection/write-queue limits and logging levels. Trace-level payload logging is for diagnosis, not a mandatory operating mode.
- Re-run a representative publish → map/bridge/store path after upgrading. MQTT QoS does not replace application-level verification of the final effect.

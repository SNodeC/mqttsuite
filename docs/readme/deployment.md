<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../README.md#project-overview" title="MQTTSuite repository"><img src="media/page-banner.svg" alt="MQTTSuite repository" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# Operate MQTTSuite deliberately

The example clients connect through loopback, with disposable IDs and small topic filters. Broker listeners retain their defaults unless explicitly bound or disabled; a loopback client address does not make every listener private. A deployment also needs explicit network access, credentials, state ownership and recovery policy.

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

MQTTBroker enables its compiled listeners by default. Bind the listeners you use deliberately and disable the others. The [first publish/subscribe example](../../README.md#publish-your-first-message) shows a first-run command; other compiled listeners remain enabled with their defaults.

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

MQTTBridge starts `admin-legacy` on **8081** and `admin-tls` on **8082** by default. Its router serves `/config` and allows bridge-definition changes, without built-in authentication middleware. With a valid `bridge.json`, disable both with `mqttbridge bridge --definition bridge.json admin-legacy --disabled admin-tls --disabled` when unused. To use management deliberately, configure their `local --host` and ports, keep them private, and put remote access behind an external authenticated TLS boundary. TLS on `admin-tls` encrypts the connection; it does not add a user login.

The current MQTTBroker CONNECT path records MQTT username/password fields but does not validate them against a credential store; its subscription/publication path does not implement a topic ACL. Supplying client credentials is therefore not proof of broker authorization. Do not expose it to untrusted clients on that assumption; choose network/application access controls appropriate to the deployment.

For MQTT over WebSockets, use a client advertising the **`mqtt`** subprotocol. TLS/WSS requires certificates, private keys and peer-trust settings. Do not ship the repository’s demo certificates as production identity material.

## Management APIs: inspect, stage, deploy

These interfaces change live configuration; use only after applying the private-listener/access policy above. The names below are current source-defined routes, not a promise of compatibility across releases. Keep an offline copy of the active mapping/topology and test changes in a disposable environment first.

### MQTTIntegrator mapping API

Routes are rooted at the selected `in-http`/`in-https` listener, not at `/api`. Every route uses the application's built-in Basic-auth middleware (`admin` / `admin`); there is no dedicated CLI credential replacement. JSON writes require `Content-Type: application/json`. Its browser UI is `/ui`.

| Method and path | Effect |
| --- | --- |
| `GET /schema`, `GET /config` | Fetch mapping schema or the active in-memory mapping |
| `POST /config/validate` | Validate a supplied complete JSON document; invalid schema yields 422 |
| `POST /config` | Replace the on-disk draft with a complete JSON object, without deployment |
| `PATCH /config` | Apply a JSON Patch to the **active** mapping and save it as a draft; successive PATCH calls do not compose over a previous draft |
| `GET /config/validateDraft` | Validate the saved draft; absent draft yields 404 |
| `POST /config/deploy` | Promote the draft, load/persist it and update/reconnect subscriptions as needed |
| `GET /config/history` | List recorded versions (`id`, `comment`, `date`) |
| `POST /config/rollback` | Supply `{"version_id":"ID_FROM_HISTORY"}` to restore/load/persist that version and update subscriptions |

For a listener deliberately bound to loopback, a read-only check is `curl --user admin http://127.0.0.1:8085/config` (curl prompts for the password). Use your actual private port if changed. Validate a complete candidate, stage it with POST, then validate the saved draft before deploying. Check HTTP status and the returned `deploy-ack`/reload information, inspect `GET /config`, and publish a representative message to verify the real effect. A successful HTTP response does not prove end-to-end delivery or an atomic update across brokers. Failed deployment/rollback needs inspection of active state and disk files, not an assumed automatic transaction rollback.

### MQTTBridge topology API

The administration listener serves `/config` as the browser editor. `GET /api/bridge/config` reads the topology; `PATCH /api/bridge/config` applies a JSON Patch, persists the definition and may reconnect bridges. `/api/bridge/sse` streams updates. This is a different API from the integrator's draft/deploy workflow; it has no built-in authentication and no corresponding draft/history/rollback routes. Back up the definition before editing, handle 409 while restarting, and verify connections/topic paths after the change. Disabling `admin-legacy` and `admin-tls` does not prevent encrypted broker connections.

## Configure TLS deliberately

Use an installed TLS-enabled component and a PEM certificate chain/private key appropriate to the listener's identity. The native SNode.C `tls` section accepts `--cert`, `--cert-key`, `--ca-cert` and `--ca-cert-dir`; inspect the selected instance before configuring it:

```text
mqttbroker in-mqtts --help=expanded
mqttcli in-mqtts tls --help
```

For example, after replacing the certificate paths with your own files:

```text
mqttbroker in-mqtts local --host 127.0.0.1 --port 18884 \
  tls --cert /path/to/server-chain.pem --cert-key /path/to/server-key.pem
mqttcli in-mqtts --disabled=false remote --host 127.0.0.1 --port 18884 \
  tls --ca-cert /path/to/trusted-ca.pem \
  sub --topic 'demo/#'
```

These commands configure the chosen endpoints, not all other broker listeners. Disable unused compiled instances and inspect the effective configuration before deployment. Restrict key-file permissions, configure peer verification/trust as required by your application, and test the actual DNS/IP identity and rejected-peer cases. Do not enable `--ca-cert-accept-unknown` as a production workaround, and never use the repository's demonstration certificates as identity material. TLS trust is separate from MQTT user authorization.

## State and delivery boundaries

- **MQTT sessions.** Configure session persistence where needed; clean-session behavior is a separate choice.
- **Broker session store.** Set `--mqtt-session-store` in the `broker` section and give the service account write access. The current SNode.C broker consumes/removes a successfully opened store at startup and writes its in-memory sessions/retained/subscription state during destruction. This is a graceful-shutdown snapshot, not continuous crash-safe storage: a kill, crash or failed write can lose the current state. Stop cleanly and verify/back up the resulting file; test recovery with the same framework revision.
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

## Troubleshoot the first connection

- **Port in use:** check the broker's listener log and effective `--show-config`; select an unused port and update every client/topology reference together. Do not stop an unrelated service to make a demo work.
- **No received message:** confirm the selected transport is enabled, wait for subscriber/bridge/integrator connection before publishing, and compare the topic filter with the output topic. The README gives exact expected topics for each walkthrough.
- **Unexpected mapped output:** do not run the same mapping in broker and integrator unless duplication is intended; verify input/output namespaces cannot feed back into a rule.
- **Cannot load a shared library or plugin:** use binaries, libraries and WebSocket modules from a compatible installation; check custom-prefix runtime lookup and build components.
- **Database/projection failure:** distinguish MQTT delivery, raw persistence and typed insert failures. Follow [the storage walkthrough](storage.md#page-overview) and [MQTTStore troubleshooting](../mqttstore-user-guide.md#7-troubleshooting) for schema, permissions and socket settings.

<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../README.md#project-overview" title="MQTTSuite repository"><img src="readme/media/page-banner.svg" alt="MQTTSuite repository" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# MQTTStore User Guide

MQTTStore is the MQTTSuite service that subscribes to MQTT topic filters and writes incoming MQTT publishes to MariaDB. It is designed for the production pipeline:

```text
MQTT devices -> MQTTBroker -> optional MQTTIntegrator normalization -> MQTTStore -> MariaDB
```

MQTTStore is intentionally generic. It does not require The Things Network, a fixed device model, or a fixed JSON schema. The safe default is to persist every received MQTT publish as a raw MQTT envelope and, when the payload is valid JSON, additionally keep a parsed JSON copy for MariaDB JSON queries.

## 1. What MQTTStore creates automatically

When `storage --auto-create-raw-table` is enabled, MQTTStore creates the raw message table automatically with `CREATE TABLE IF NOT EXISTS`. The default table name is `mqtt_messages`; override it with `storage --raw-table <name>`.

MQTTStore automatically creates this table only inside an already existing MariaDB database/schema. Creating the database itself and creating/granting the database user are administrative bootstrap tasks that must be done once by a MariaDB administrator.

The automatically managed raw table contains:

| Column | Type | Meaning |
| ------ | ---- | ------- |
| `id` | `BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY` | Stable row id. |
| `received_at` | `TIMESTAMP(6)` | Database receive timestamp. |
| `source_instance` | `VARCHAR(255)` | MQTTSuite connection instance such as `in-mqtt`. |
| `topic` | `VARCHAR(1024)` | MQTT topic. |
| `qos` | `TINYINT UNSIGNED` | MQTT QoS of the received publish. |
| `retain_flag` | `BOOLEAN` | MQTT retain flag. |
| `dup_flag` | `BOOLEAN` | MQTT duplicate flag. |
| `packet_identifier` | `INT UNSIGNED NULL` | MQTT packet id when present. |
| `payload` | `LONGBLOB` | Original payload bytes. |
| `payload_text` | `LONGTEXT NULL` | Text copy when the payload is safe to expose as text. |
| `payload_json` | `JSON NULL` | Parsed JSON copy when parsing succeeds. |
| `payload_format` | `ENUM('json', 'text', 'binary')` | Payload classification. |

The table also has indexes on `received_at` and the first 255 characters of `topic`.

## 2. Database setup and permission profiles

Use the complete [storage walkthrough](readme/storage.md#page-overview) for database/account bootstrap, domain-table creation, projection JSON, MQTTStore launch and expected SQL results. This page owns operational reference and troubleshooting rather than a second onboarding procedure.

### Permission profiles

Use the permission profile that matches how you operate MQTTStore:

| Profile | Grants | When to use |
| ------- | ------ | ----------- |
| Raw table auto-create | `CREATE, INSERT, SELECT, INDEX` | Recommended first deployment. MQTTStore creates `mqtt_messages`. |
| Pre-created raw table | `INSERT, SELECT` | Use after a DBA creates the table manually. Start with `--auto-create-raw-table=false`. |
| Raw table plus projections | `CREATE, INSERT, SELECT, INDEX` plus `INSERT` on projection tables | Raw table is auto-created; projection tables are DBA-managed. |
| Read-only diagnostics user | `SELECT` | Separate user for dashboards, analysts, or ad-hoc queries. |

Do not use the MariaDB `root` user for MQTTStore. Give MQTTStore a dedicated user and only the permissions required for your deployment model.

## 3. Projection and raw-storage setup

Follow [typed-table setup](readme/storage.md#2-create-the-typed-table) and [raw-only storage](readme/storage.md#raw-only-storage). MQTTStore creates the raw table when enabled, not your domain-specific tables or migrations.

## 4. Persist the configuration

Protect credentials before persisting: restrict the configuration directory/file to the service account, never publish dumps, and avoid secret-bearing command lines or shell history in production. The example password below is disposable; use `--config-file` for a protected configuration. Quote INI topic filters containing `#`. For service-style operation, write a known-good configuration once with `--write-config` / `-w` according to the MQTTSuite configuration workflow:

```text
mqttstore \
    in-mqtt --disabled=false \
        remote --host 127.0.0.1 --port 18883 \
        session --client-id mqttstore-local \
        sub --topic 'normalized/#' \
        db --host 127.0.0.1 --database mqttsuite_demo --username mqttstore_demo --password 'REPLACE-WITH-A-UNIQUE-PASSWORD' \
            storage --raw-table mqtt_messages --auto-create-raw-table \
    -w
```

After that, the service can be started with the saved defaults, depending on your installation and instance selection.

## 5. Traffic and verification

The [storage walkthrough](readme/storage.md#4-publish-a-measurement) owns the complete MQTTCli publish command and [SQL verification](readme/storage.md#5-verify-the-raw-message-and-projection), using loopback port 18883 and `mqttsuite_demo`. Its projection contains `device_id`, `value`, `unit` and `received_at`; do not query a `metric` column that that schema does not define.

For raw storage, JSON messages populate the available JSON/text representations; plain text has no parsed JSON representation. Raw metadata also records QoS and retained/duplicate flags. To inspect a larger recent sample, use your database client with the same demonstration database:

```sql
SELECT id, received_at, topic, qos, retain_flag, payload_format, payload_text
FROM mqttsuite_demo.mqtt_messages
ORDER BY id DESC LIMIT 10;
```

Use the broker/client's own `pub --help` for additional retained/QoS traffic tests, and the `sub` topic suffix `##<qos>` in the subscription reference for an input QoS override. MQTT QoS is not a promise of exactly-once SQL insertion. Raw-only storage needs no projection file; typed tables, nullability and validation remain operator-owned.

### Subscription QoS override

A topic filter ending in `##<qos>` selects the requested subscription QoS, for example `normalized/###1` for `normalized/#` at QoS 1. Quote this value in the shell and in INI configuration so `#` is not mistaken for a comment. Check the installed application's `sub --help` before combining multiple filters. This setting is distinct from a publisher's QoS and the database's commit boundary.

## 6. Operational recommendations

- Keep raw storage enabled. It provides audit, replay, and debugging data even when projections change.
- Use MQTTIntegrator to normalize vendor-specific payloads before MQTTStore when you have multiple device families.
- Use a dedicated MariaDB user for MQTTStore and do not share it with dashboards or administrators.
- Use `--auto-create-raw-table=false` in tightly controlled production environments where DBAs own all DDL.
- Keep projection files in version control with the schema migrations for their target tables.
- Monitor table growth. Raw MQTT tables can grow quickly on wildcard subscriptions such as `#`.
- Prefer narrower topic filters in production, for example `normalized/#` instead of `#`.
- Treat retained messages deliberately. They are useful for state, but they may not represent fresh telemetry.

## 7. Troubleshooting

### MQTTStore starts but no rows appear

- Check that the selected instance is enabled and connected to the expected broker.
- Confirm the subscription filter matches the published topic.
- Verify database credentials and grants.
- Check that `storage --raw-table` uses only letters, digits, and `_`.

### Raw table is not created

- Confirm `storage --auto-create-raw-table` is set.
- Confirm the MariaDB user has `CREATE` and `INDEX` on the target database.
- Confirm the database itself already exists.

### Projection rows are missing

- Confirm the payload is valid JSON. Projections run only for JSON payloads.
- Confirm the projection `topic` matches the MQTT topic.
- Confirm the JSON Pointer path exists in the payload.
- Confirm the projection table already exists and the MQTTStore user has `INSERT` on it.

### Permission denied

Use MariaDB to inspect grants:

```sql
SHOW GRANTS FOR 'mqttstore'@'localhost';
```

Then add only the missing permission needed for your deployment profile.

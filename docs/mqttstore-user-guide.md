# MQTTStore User Guide

MQTTStore is the MQTTSuite service that subscribes to MQTT topic filters and writes incoming MQTT publishes to MariaDB. It is designed for the production pipeline:

```text
MQTT devices -> MQTTBroker -> optional MQTTIntegrator normalization -> MQTTStore -> MariaDB
```

MQTTStore is intentionally generic. It does not require The Things Network, a fixed device model, or a fixed JSON schema. The safe default is to persist every received MQTT publish as a raw MQTT envelope and, when the payload is valid JSON, additionally keep a parsed JSON copy for MariaDB JSON queries.

## 1. What MQTTStore creates automatically

When `storage --auto-create-raw-table true` is enabled, MQTTStore creates the raw message table automatically with `CREATE TABLE IF NOT EXISTS`. The default table name is `mqtt_messages`; override it with `storage --raw-table <name>`.

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

## 2. MariaDB bootstrap: database, user, and permissions

Log in as a MariaDB administrator, for example `root`:

```bash
sudo mariadb
```

Create a dedicated database and user:

```sql
CREATE DATABASE mqttsuite_store
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

CREATE USER 'mqttstore'@'localhost'
  IDENTIFIED BY 'replace-with-a-long-random-password';
```

Grant the minimum permissions needed for raw-table auto-generation and message insertion:

```sql
GRANT CREATE, INSERT, SELECT, INDEX
  ON mqttsuite_store.*
  TO 'mqttstore'@'localhost';

FLUSH PRIVILEGES;
```

For a remote MQTTStore host, replace `localhost` with the client host or subnet, for example:

```sql
CREATE USER 'mqttstore'@'10.10.20.%'
  IDENTIFIED BY 'replace-with-a-long-random-password';

GRANT CREATE, INSERT, SELECT, INDEX
  ON mqttsuite_store.*
  TO 'mqttstore'@'10.10.20.%';

FLUSH PRIVILEGES;
```

### Permission profiles

Use the permission profile that matches how you operate MQTTStore:

| Profile | Grants | When to use |
| ------- | ------ | ----------- |
| Raw table auto-create | `CREATE, INSERT, SELECT, INDEX` | Recommended first deployment. MQTTStore creates `mqtt_messages`. |
| Pre-created raw table | `INSERT, SELECT` | Use after a DBA creates the table manually. Start with `--auto-create-raw-table false`. |
| Raw table plus projections | `CREATE, INSERT, SELECT, INDEX` plus `INSERT` on projection tables | Raw table is auto-created; projection tables are DBA-managed. |
| Read-only diagnostics user | `SELECT` | Separate user for dashboards, analysts, or ad-hoc queries. |

Do not use the MariaDB `root` user for MQTTStore. Give MQTTStore a dedicated user and only the permissions required for your deployment model.

## 3. Optional: create projection tables

Raw storage works without any additional schema. Projection tables are optional typed tables for fast analytics and dashboards. MQTTStore does not auto-create projection tables because those are domain schemas and should be versioned/migrated explicitly.

Example table for normalized temperature telemetry:

```sql
CREATE TABLE mqttsuite_store.sensor_measurements (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    device_id VARCHAR(255) NOT NULL,
    metric VARCHAR(255) NOT NULL,
    value DOUBLE NOT NULL,
    unit VARCHAR(64) NULL,
    received_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    INDEX idx_device_metric_time (device_id, metric, received_at)
);

GRANT INSERT, SELECT
  ON mqttsuite_store.sensor_measurements
  TO 'mqttstore'@'localhost';

FLUSH PRIVILEGES;
```

Projection file example:

```json
{
  "projections": [
    {
      "name": "room_temperature",
      "topic": "normalized/+/temperature",
      "table": "sensor_measurements",
      "columns": {
        "device_id": { "topic_level": 1, "required": true },
        "metric": { "literal": "temperature" },
        "value": { "json_pointer": "/value", "required": true },
        "unit": { "json_pointer": "/unit" }
      }
    }
  ]
}
```

Projection rules:

- `--projection-file` belongs to the `storage` configuration section, so in SNode.C command-line syntax it must be written after the `storage` subcommand.
- MQTTStore reads the projection file at startup. Restart the service after editing the file.
- The JSON file may contain either a top-level `projections` array or the array itself.
- `topic` uses MQTT topic-filter syntax with `+` and `#`.
- `topic_level` is zero-based. For `normalized/boiler/temperature`, level `0` is `normalized`, level `1` is `boiler`, and level `2` is `temperature`.
- `literal` writes a constant string value.
- `json_pointer` reads from the parsed JSON payload using JSON Pointer syntax, for example `/value` or `/battery/voltage`.
- Shorthand column mappings such as `"value": "/value"` are accepted and mean `json_pointer: "/value"`. Use the object form when you need `required`, `topic_level`, or `literal`.
- `required: true` inserts `NULL` when the source is missing; without `required`, missing values are skipped.
- Projection inserts are attempted only for valid JSON payloads whose MQTT topic matches the projection filter; the raw MQTT envelope is inserted separately.

## 4. Start MQTTStore with automatic raw-table generation

For a local broker on plain MQTT/TCP:

```bash
mqttstore in-mqtt \
  remote --host 127.0.0.1 \
         --port 1883 \
  session --client-id mqttstore-local \
  sub --topic '#' \
  db --host 127.0.0.1 \
     --database mqttsuite_store \
     --username mqttstore \
     --password 'replace-with-a-long-random-password' \
  storage --raw-table mqtt_messages \
          --auto-create-raw-table true
```

With a projection file:

```bash
mqttstore in-mqtt \
  remote --host 127.0.0.1 \
         --port 1883 \
  session --client-id mqttstore-local \
  sub --topic 'normalized/#' \
  db --host 127.0.0.1 \
     --database mqttsuite_store \
     --username mqttstore \
     --password 'replace-with-a-long-random-password' \
  storage --raw-table mqtt_messages \
          --auto-create-raw-table true \
          --projection-file /etc/mqttsuite/mqttstore-projections.json
```

### Working `--projection-file` walkthrough

This is the smallest end-to-end projection example. It keeps raw storage enabled and adds one typed projection for temperature telemetry published to `normalized/<device>/temperature`.

1. Create the typed projection table once if you did not already create it in section 3:

   ```sql
   CREATE TABLE IF NOT EXISTS mqttsuite_store.sensor_measurements (
       id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
       device_id VARCHAR(255) NOT NULL,
       metric VARCHAR(255) NOT NULL,
       value DOUBLE NOT NULL,
       unit VARCHAR(64) NULL,
       received_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
       INDEX idx_device_metric_time (device_id, metric, received_at)
   );
   ```

2. Create the projection file on the host that runs MQTTStore:

   ```bash
   sudo install -d -m 0755 /etc/mqttsuite
   sudo tee /etc/mqttsuite/mqttstore-projections.json >/dev/null <<'JSON'
   {
     "projections": [
       {
         "name": "room_temperature",
         "topic": "normalized/+/temperature",
         "table": "sensor_measurements",
         "columns": {
           "device_id": { "topic_level": 1, "required": true },
           "metric": { "literal": "temperature" },
           "value": { "json_pointer": "/value", "required": true },
           "unit": { "json_pointer": "/unit" }
         }
       }
     ]
   }
   JSON
   ```

3. Start MQTTStore with the file in the `storage` section:

   ```bash
   mqttstore in-mqtt \
     remote --host 127.0.0.1 --port 1883 \
     session --client-id mqttstore-projection-demo \
     sub --topic 'normalized/#' \
     db --host 127.0.0.1 \
        --database mqttsuite_store \
        --username mqttstore \
        --password 'replace-with-a-long-random-password' \
     storage --raw-table mqtt_messages \
             --auto-create-raw-table true \
             --projection-file /etc/mqttsuite/mqttstore-projections.json
   ```

4. Publish a matching message from another terminal:

   ```bash
   mosquitto_pub -h 127.0.0.1 -p 1883 \
     -t 'normalized/boiler/temperature' \
     -m '{"value":63.4,"unit":"C","source":"demo"}'
   ```

5. Confirm both writes in MariaDB:

   ```sql
   SELECT topic, payload_format, payload_text
   FROM mqtt_messages
   ORDER BY id DESC
   LIMIT 1;

   SELECT device_id, metric, value, unit
   FROM sensor_measurements
   ORDER BY id DESC
   LIMIT 1;
   ```

   The projected row should contain `boiler`, `temperature`, `63.4`, and `C`. If the raw row appears but the projection row does not, check that the payload is valid JSON, the topic matches `normalized/+/temperature`, the JSON Pointer `/value` exists, and the `sensor_measurements` table already exists.

For MQTT over WebSockets:

```bash
mqttstore in-wsmqtt \
  remote --host 127.0.0.1 \
         --port 8080 \
  http --target /ws \
  session --client-id mqttstore-ws \
  sub --topic 'normalized/#' \
  db --database mqttsuite_store \
     --username mqttstore \
     --password 'replace-with-a-long-random-password' \
  storage --auto-create-raw-table true
```

## 5. Persist the configuration

For service-style operation, write a known-good configuration once with `--write-config` / `-w` according to the MQTTSuite configuration workflow:

```bash
mqttstore in-mqtt \
  remote --host 127.0.0.1 \
         --port 1883 \
  session --client-id mqttstore-local \
  sub --topic 'normalized/#' \
  db --host 127.0.0.1 \
     --database mqttsuite_store \
     --username mqttstore \
     --password 'replace-with-a-long-random-password' \
  storage --raw-table mqtt_messages \
          --auto-create-raw-table true \
  -w
```

After that, the service can be started with the saved defaults, depending on your installation and instance selection.

## 6. Generate example MQTT traffic

The examples below use Mosquitto clients against a local broker. If your broker listens elsewhere, adjust host and port.

### JSON telemetry

```bash
mosquitto_pub -h 127.0.0.1 -p 1883 \
  -t 'normalized/boiler/temperature' \
  -m '{"value":63.4,"unit":"C","source":"demo"}'
```

Expected raw-table behavior:

- `topic = normalized/boiler/temperature`
- `payload_format = json`
- `payload_text` contains the original JSON string
- `payload_json` contains the parsed JSON document

If the projection example above is enabled, MQTTStore also inserts a row into `sensor_measurements`:

| device_id | metric | value | unit |
| --------- | ------ | ----- | ---- |
| `boiler` | `temperature` | `63.4` | `C` |

### Plain text status

```bash
mosquitto_pub -h 127.0.0.1 -p 1883 \
  -t 'devices/pump-1/status' \
  -m 'running'
```

Expected raw-table behavior:

- `payload_format = text`
- `payload_text = running`
- `payload_json = NULL`

### Retained state

```bash
mosquitto_pub -h 127.0.0.1 -p 1883 \
  -r \
  -t 'devices/pump-1/availability' \
  -m 'online'
```

Expected raw-table behavior:

- `retain_flag = 1`
- The message is still stored like any other publish.
- Downstream consumers can decide whether retained state represents a new measurement or broker state.

### QoS 1 publish

```bash
mosquitto_pub -h 127.0.0.1 -p 1883 \
  -q 1 \
  -t 'normalized/room-101/temperature' \
  -m '{"value":22.7,"unit":"C"}'
```

Expected raw-table behavior:

- `qos = 1`
- JSON parsing and projections behave the same as for QoS 0.

### Subscribing with a QoS override

MQTTStore topic filters accept the MQTTSuite `##<qos>` suffix. For example, subscribe to normalized messages at QoS 1:

```bash
mqttstore in-mqtt \
  remote --host 127.0.0.1 --port 1883 \
  session --client-id mqttstore-qos1 \
  sub --topic 'normalized/###1' \
  db --database mqttsuite_store --username mqttstore --password 'replace-with-a-long-random-password' \
  storage --auto-create-raw-table true
```

## 7. Verify stored data

Open MariaDB:

```bash
mariadb -u mqttstore -p mqttsuite_store
```

Inspect recent messages:

```sql
SELECT id,
       received_at,
       source_instance,
       topic,
       qos,
       retain_flag,
       dup_flag,
       packet_identifier,
       payload_format,
       payload_text
FROM mqtt_messages
ORDER BY id DESC
LIMIT 10;
```

Query JSON payloads:

```sql
SELECT id,
       topic,
       JSON_UNQUOTE(JSON_EXTRACT(payload_json, '$.value')) AS value,
       JSON_UNQUOTE(JSON_EXTRACT(payload_json, '$.unit')) AS unit
FROM mqtt_messages
WHERE payload_format = 'json'
ORDER BY id DESC
LIMIT 10;
```

Inspect projected measurements:

```sql
SELECT id, device_id, metric, value, unit, received_at
FROM sensor_measurements
ORDER BY id DESC
LIMIT 10;
```

## 8. Operational recommendations

- Keep raw storage enabled. It provides audit, replay, and debugging data even when projections change.
- Use MQTTIntegrator to normalize vendor-specific payloads before MQTTStore when you have multiple device families.
- Use a dedicated MariaDB user for MQTTStore and do not share it with dashboards or administrators.
- Use `--auto-create-raw-table false` in tightly controlled production environments where DBAs own all DDL.
- Keep projection files in version control with the schema migrations for their target tables.
- Monitor table growth. Raw MQTT tables can grow quickly on wildcard subscriptions such as `#`.
- Prefer narrower topic filters in production, for example `normalized/#` instead of `#`.
- Treat retained messages deliberately. They are useful for state, but they may not represent fresh telemetry.

## 9. Troubleshooting

### MQTTStore starts but no rows appear

- Check that the selected instance is enabled and connected to the expected broker.
- Confirm the subscription filter matches the published topic.
- Verify database credentials and grants.
- Check that `storage --raw-table` uses only letters, digits, and `_`.

### Raw table is not created

- Confirm `storage --auto-create-raw-table true` is set.
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

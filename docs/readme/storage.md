# Persist messages and project useful fields

[← MQTTSuite](../../README.md)

MQTTStore subscribes to topic filters and writes messages to MariaDB. Raw storage and typed projections are separate: keep the original payload even when a message is not JSON, and add a typed projection when the payload follows a useful schema.

This example uses the loopback broker on port **18883**, a local MariaDB server and the [projection file](examples/projections.json). Database administration requires an appropriately privileged account. Replace the sample password before use.

## 1. Create the database and account

Run as a MariaDB administrator:

```sql
CREATE DATABASE mqttsuite_demo
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE USER 'mqttstore_demo'@'localhost'
  IDENTIFIED BY 'REPLACE-WITH-A-UNIQUE-PASSWORD';

GRANT CREATE, INSERT, SELECT, INDEX ON mqttsuite_demo.*
  TO 'mqttstore_demo'@'localhost';
```

Use a dedicated account, not the database administrator. This demonstration allows automatic creation of the raw table. For an operator-managed schema, pre-create that table, disable automatic creation and narrow the runtime privileges.

## 2. Create the typed table

MQTTStore does not create or migrate your domain-specific tables. Create this one explicitly:

```sql
CREATE TABLE mqttsuite_demo.sensor_measurements (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  device_id VARCHAR(255) NOT NULL,
  value DOUBLE NOT NULL,
  unit VARCHAR(32) NULL,
  received_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
);
```

The [projection JSON](examples/projections.json) matches `normalized/+/temperature`, extracts the device from topic level 1, and reads `/value` and `/unit` from JSON. Topic levels are zero-based. `required: true` writes SQL NULL when the source is missing; without it, the column is omitted. It does not validate and reject the message before insertion. Here a missing `value` violates `NOT NULL`, causing the typed insert to fail; raw storage is independent. Choose nullability, defaults and validation to fit the data you accept.

## 3. Store credentials in a protected configuration

Create `store.conf` using an editor and restrict it to the service user (`chmod 600 store.conf`). This keeps the password out of the process command line:

```ini
[in-mqtt]
disabled = false
[in-mqtt.remote]
host = 127.0.0.1
port = 18883
[in-mqtt.session]
client-id = readme-store
[in-mqtt.sub]
topic = "normalized/#"
[in-mqtt.db]
socket = /run/mysqld/mysqld.sock
database = mqttsuite_demo
username = mqttstore_demo
password = REPLACE-WITH-A-UNIQUE-PASSWORD
[in-mqtt.db.storage]
raw-table = mqtt_messages
auto-create-raw-table = true
projection-file = docs/readme/examples/projections.json
```

Keep the topic filter quoted: an unquoted `#` begins an INI comment. Adjust the MariaDB socket to your installation. For TCP database access, explicitly configure the socket/host/port combination according to `mqttstore in-mqtt --disabled=false db --help`; a configured Unix socket takes precedence. Other connection instances remain at their disabled defaults in this example.

From the repository root, with the demo broker running:

```sh
mqttstore --config-file store.conf
```

## 4. Publish a measurement

```sh
mqttcli --config-file /dev/null \
  in-mqtt --disabled=false \
  remote --host 127.0.0.1 --port 18883 \
  pub --topic 'normalized/room1/temperature' \
      --message '{"value":21.5,"unit":"C"}' \
  socket --reconnect=false
```

## 5. Verify the raw message and projection

Use a database client with appropriate read access:

```sql
SELECT topic, payload_text, payload_format
FROM mqttsuite_demo.mqtt_messages
ORDER BY id DESC LIMIT 1;

SELECT device_id, value, unit
FROM mqttsuite_demo.sensor_measurements
ORDER BY id DESC LIMIT 1;
```

The raw row contains topic `normalized/room1/temperature` and the original JSON payload. The typed row contains device `room1`, value `21.5` and unit `C`. Stop MQTTStore with Ctrl+C when finished; the database remains until you deliberately remove it.

## Raw-only storage

Omit `projection-file` when you only want raw persistence. The raw table records receive time, source instance, topic, QoS, retain/duplicate flags, packet identifier where present, original payload bytes and available text/JSON representations.

Database inserts, MQTT acknowledgements and typed projections are separate boundaries. Do not infer atomic raw-plus-projection writes or exactly-once database delivery from MQTT QoS. Plan retention, backups, reconnect behavior and capacity explicitly.

[Full MQTTStore guide](https://github.com/SNodeC/mqttsuite/blob/master/docs/mqttstore-user-guide.md) · [Projection schema](https://github.com/SNodeC/mqttsuite/blob/master/mqttstore/lib/projection-schema.json) · [Deployment](deployment.md)

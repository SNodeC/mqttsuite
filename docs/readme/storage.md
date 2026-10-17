# Persist messages and project useful fields

<p>
  <a href="../../README.md"><img src="media/menu/back-mqttsuite.svg" alt="← MQTTSuite" width="110" height="24"></a>
</p>

The landing page shows the basic example; this guide extends it.

MQTTStore subscribes to topic filters and writes messages to MariaDB. Raw storage and typed projections are separate: keep the original payload even when a message is not JSON, and add a typed projection when the payload follows a useful schema.

**You need:** MQTTBroker, MQTTCli, MQTTStore, a local MariaDB server and database-client access with an appropriately privileged account. Start in an empty working directory. Follow the [first-run and listener defaults](../../README.md#publish-your-first-message), and save `projections.json` from [the complete storage example](../../README.md#keep-the-original-messageand-query-the-useful-fields). The projection is also available as an optional [download](examples/projections.json). Port **18883** must be free. Replace the sample password before use.

**Run — terminal 1, start the broker:**

```sh
mqttbroker \
	in-mqtt \
		local --host 127.0.0.1 --port 18883
```

## 1. Create the database and account

**Configuration — SQL, run through a MariaDB administrator connection:**

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

**Configuration — SQL:** MQTTStore does not create or migrate your domain-specific tables. Create this one explicitly through the same database-client connection:

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

## 3. Start MQTTStore

Configure the MQTT connection, subscription and database explicitly. Adjust the MariaDB socket to your installation. For TCP database access, inspect `mqttstore in-mqtt db --help`; a configured Unix socket takes precedence. Other connection instances remain disabled by default.

**Run — terminal 2, from the same working directory:**

```sh
mqttstore \
	in-mqtt --disabled=false \
		remote --host 127.0.0.1 --port 18883 \
		session --client-id readme-store \
		sub --topic 'normalized/#' \
		db --socket /run/mysqld/mysqld.sock --database mqttsuite_demo --username mqttstore_demo --password 'REPLACE-WITH-A-UNIQUE-PASSWORD' \
			storage --raw-table mqtt_messages --auto-create-raw-table --projection-file projections.json
```

## 4. Publish a measurement

**Run — terminal 3, after MQTTStore connects:**

```sh
mqttcli \
	in-mqtt --disabled=false \
		remote --host 127.0.0.1 --port 18883 \
		pub --topic 'normalized/room1/temperature' --message '{"value":21.5,"unit":"C"}' \
		socket --reconnect=false
```

## 5. Verify the raw message and projection

**Run — database client, with appropriate read access:**

```sql
SELECT topic, payload_text, payload_format
FROM mqttsuite_demo.mqtt_messages
ORDER BY id DESC LIMIT 1;

SELECT device_id, value, unit
FROM mqttsuite_demo.sensor_measurements
ORDER BY id DESC LIMIT 1;
```

**Expected result:** the raw row contains topic `normalized/room1/temperature` and the original JSON payload. The typed row contains device `room1`, value `21.5` and unit `C`. Stop MQTTStore and your demo broker with Ctrl+C when finished; the database remains until you deliberately remove it.

## Raw-only storage

Omit `--projection-file` when you only want raw persistence. The raw table records receive time, source instance, topic, QoS, retain/duplicate flags, packet identifier where present, original payload bytes and available text/JSON representations.

**Boundaries:** database inserts, MQTT acknowledgements and typed projections are separate boundaries. Do not infer atomic raw-plus-projection writes or exactly-once database delivery from MQTT QoS. Plan retention, backups, reconnect behavior and capacity explicitly.

**Go further:** [deployment](deployment.md). Reference: [full MQTTStore guide](https://github.com/SNodeC/mqttsuite/blob/master/docs/mqttstore-user-guide.md) and [projection schema](https://github.com/SNodeC/mqttsuite/blob/master/mqttstore/lib/projection-schema.json).

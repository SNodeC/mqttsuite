"""Exercise the real broker over MQTT 3.1.1 without third-party clients."""
import contextlib
from pathlib import Path
import re
import socket
import struct
import subprocess
import sys
import tempfile
import time


def text(value):
    encoded = value.encode()
    return struct.pack("!H", len(encoded)) + encoded


def send(connection, header, payload):
    remaining = len(payload)
    length = bytearray()
    while True:
        byte = remaining % 128
        remaining //= 128
        length.append(byte | (128 if remaining else 0))
        if not remaining:
            break
    connection.sendall(bytes([header]) + length + payload)


def exact(connection, count):
    result = b""
    while len(result) < count:
        block = connection.recv(count - len(result))
        if not block:
            raise AssertionError("broker closed the connection")
        result += block
    return result


def receive(connection):
    header = exact(connection, 1)[0]
    length = 0
    for shift in range(0, 28, 7):
        byte = exact(connection, 1)[0]
        length |= (byte & 127) << shift
        if not byte & 128:
            return header, exact(connection, length)
    raise AssertionError("invalid MQTT remaining length")


def connect(port, name):
    connection = socket.create_connection(("127.0.0.1", port), timeout=2)
    try:
        send(connection, 0x10, text("MQTT") + b"\x04\x02\x00\x1e" + text(name))
        assert receive(connection) == (0x20, b"\x00\x00"), "CONNECT refused"
        return connection
    except BaseException:
        connection.close()
        raise


def subscribe(connection):
    send(connection, 0x82, b"\x00\x01" + text("snodec/ci/smoke") + b"\x00")
    assert receive(connection) == (0x90, b"\x00\x01\x00"), "SUBSCRIBE refused"


def main():
    executable = str(Path(sys.argv[1]).resolve())
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with tempfile.TemporaryDirectory() as directory:
        # Isolate configuration, session persistence and all other listeners.
        configuration = Path(directory) / "broker.conf"
        configuration.write_text("")
        command = [executable, "--config-file", str(configuration), "--log-level", "0"]
        help_output = subprocess.run([executable, "--help"], capture_output=True, text=True, timeout=5)
        assert help_output.returncode == 2, "broker help contract changed"
        instances = re.findall(r"^  ([a-z0-9-]+)\n       Configuration for server instance",
                               help_output.stdout, flags=re.M)
        assert "in-mqtt" in instances, "IPv4 broker listener unavailable"
        for instance in instances:
            if instance != "in-mqtt":
                command += [instance, "--disabled=true"]
        command += ["in-mqtt", "local", "--host", "127.0.0.1", "--port", str(port)]
        with tempfile.TemporaryFile() as log:
            process = subprocess.Popen(command, cwd=directory, stdout=log, stderr=log)
            try:
                deadline = time.monotonic() + 10
                while True:
                    if process.poll() is not None:
                        raise AssertionError("broker exited before accepting clients")
                    try:
                        client = connect(port, "snodec-ci-publisher")
                        break
                    except ConnectionRefusedError:
                        if time.monotonic() >= deadline:
                            raise
                        time.sleep(0.05)
                with contextlib.closing(client):
                    subscribe(client)
                    payload = text("snodec/ci/smoke") + b"first success"
                    send(client, 0x31, payload)  # QoS 0, retained publish.
                    assert receive(client) == (0x30, payload), "live publish was not delivered"
                    send(client, 0xc0, b"")
                    assert receive(client) == (0xd0, b""), "PINGRESP missing"
                    with contextlib.closing(connect(port, "snodec-ci-retained")) as reader:
                        send(reader, 0x82, b"\x00\x01" + text("snodec/ci/smoke") + b"\x00")
                        # A broker may deliver retained messages before SUBACK.
                        replies = [receive(reader), receive(reader)]
                        assert (0x90, b"\x00\x01\x00") in replies, "SUBSCRIBE refused"
                        assert (0x31, payload) in replies, "retained publish was not delivered"
                        send(reader, 0xe0, b"")
                    send(client, 0xe0, b"")
                assert process.poll() is None, "broker died during publish/subscribe"
                print("MQTT CONNECT, SUBSCRIBE, live/retained PUBLISH, PING and DISCONNECT passed")
            except BaseException:
                log.seek(0)
                sys.stderr.write(log.read().decode(errors="replace"))
                raise
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()

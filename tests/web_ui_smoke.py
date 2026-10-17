"""Exercise embedded UI serving and directory configuration at public boundaries."""
import contextlib
import http.client
import json
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile
import time
import unittest

AUTH = {"Authorization": "Basic YWRtaW46YWRtaW4="}


@contextlib.contextmanager
def server(executable, application, directory, html_dir=None, configured_dir=None):
    configuration = directory / "application.conf"
    configuration.write_text(f'{application}.html-dir="{configured_dir}"\n' if configured_dir else "")
    help_result = subprocess.run([str(executable), "--config-file", str(configuration), "--help"],
                                 capture_output=True, text=True, timeout=5)
    assert help_result.returncode == 2, help_result.stdout + help_result.stderr
    instances = re.findall(r"^  ([a-z0-9-]+)(?: REQUIRED)?\n       Configuration for (?:server|client) instance",
                           help_result.stdout, re.M)
    listener = "admin-legacy" if application == "bridge" else "in-http"
    assert listener in instances, help_result.stdout
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    command = [str(executable), "--config-file", str(configuration), "--log-level", "0", application]
    if html_dir is not None:
        command += ["--html-dir", str(html_dir)]
    if application == "bridge":
        definition = directory / "bridge.json"
        definition.write_text(json.dumps({"bridges": [{"name": "smoke", "disabled": True,
            "brokers": [{"network": {"instance_name": "smoke", "protocol": "in",
                         "encryption": "legacy", "transport": "stream",
                         "in": {"host": "127.0.0.1", "port": 1883}}}]}]}))
        command += ["--definition", str(definition)]
    for instance in instances:
        if instance != listener:
            command += [instance, "--disabled=true"]
    command += [listener, "local", "--host", "127.0.0.1", "--port", str(port)]
    with tempfile.TemporaryFile() as log:
        process = subprocess.Popen(command, cwd=directory, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 10
            while True:
                assert process.poll() is None, "application exited before listening"
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=1):
                        break
                except ConnectionRefusedError:
                    if time.monotonic() >= deadline:
                        raise
                    time.sleep(0.05)
            yield port
        except BaseException:
            log.seek(0)
            print(log.read().decode(errors="replace"), file=sys.stderr)
            raise
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)


def request(port, path, authenticated=True, method="GET", body=None):
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        headers = dict(AUTH) if authenticated else {}
        if body is not None:
            headers["Content-Type"] = "application/json-patch+json"
        connection.request(method, path, body=body, headers=headers)
        response = connection.getresponse()
        return response.status, dict(response.getheaders()), response.read()
    finally:
        connection.close()


class WebUISmoke(unittest.TestCase):
    def test_help_and_default(self):
        result = subprocess.run([str(EXECUTABLE), "--config-file", "/dev/null", APPLICATION, "--help"],
                                capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 2)
        self.assertIn(f"--html-dir directory:DIR [{DEFAULT_DIR}]", result.stdout)
        self.assertIn("Path to HTML source directory", result.stdout)

    def test_reject_missing_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            missing = Path(temporary) / "missing"
            configuration = Path(temporary) / "invalid.conf"
            configuration.write_text(f'{APPLICATION}.html-dir="{missing}"\n')
            for arguments in [["--config-file", "/dev/null", APPLICATION, "--html-dir", str(missing)],
                              ["--config-file", str(configuration)]]:
                with self.subTest(arguments=arguments):
                    result = subprocess.run([str(EXECUTABLE), *arguments], capture_output=True,
                                            text=True, timeout=5)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("Directory does not exist", result.stdout + result.stderr)

    def test_generated_assets_and_authentication(self):
        mount = "/config" if APPLICATION == "bridge" else "/ui"
        with tempfile.TemporaryDirectory() as temporary:
            with server(EXECUTABLE, APPLICATION, Path(temporary), HTML_DIR) as port:
                status, headers, _ = request(port, mount)
                self.assertEqual(status, 302)
                self.assertEqual(headers["Location"], mount + "/index.html")
                for asset in HTML_DIR.rglob("*"):
                    if asset.is_file():
                        status, _, body = request(port, mount + "/" + asset.relative_to(HTML_DIR).as_posix())
                        self.assertEqual(status, 200, str(asset))
                        self.assertEqual(body, asset.read_bytes(), str(asset))
                if APPLICATION == "bridge":
                    status, _, body = request(port, "/api/bridge/config")
                    self.assertEqual(status, 200)
                    self.assertEqual(json.loads(body)["bridges"][0]["name"], "smoke")
                    patch = json.dumps([{"op": "replace", "path": "/bridges/0/name", "value": "renamed"}])
                    status, _, body = request(port, "/api/bridge/config", method="PATCH", body=patch)
                    self.assertEqual(status, 200, body)
                    self.assertEqual(json.loads(body)["success"], True)
                    status, _, body = request(port, "/api/bridge/config")
                    self.assertEqual(status, 200)
                    self.assertEqual(json.loads(body)["bridges"][0]["name"], "renamed")
                    persisted = json.loads((Path(temporary) / "bridge.json").read_text())
                    self.assertEqual(persisted["bridges"][0]["name"], "renamed")
                if APPLICATION == "integrator":
                    status, _, _ = request(port, mount + "/index.html", authenticated=False)
                    self.assertEqual(status, 401)
                    status, _, body = request(port, "/config")
                    self.assertEqual(status, 200)
                    self.assertIn("mapping", json.loads(body))
                    status, _, body = request(port, "/schema")
                    self.assertEqual(status, 200)
                    self.assertIn("properties", json.loads(body))

    def test_config_file_and_cli_precedence(self):
        mount = "/config" if APPLICATION == "bridge" else "/ui"
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            configured = directory / "configured"
            override = directory / "override"
            for html, text in [(configured, "configured directory"), (override, "CLI directory")]:
                html.mkdir()
                (html / "index.html").write_text(text)
            for cli, expected in [(None, b"configured directory"), (override, b"CLI directory")]:
                with self.subTest(cli=cli):
                    with server(EXECUTABLE, APPLICATION, directory, cli, configured) as port:
                        status, _, body = request(port, mount + "/index.html")
                        self.assertEqual(status, 200)
                        self.assertEqual(body, expected)


if __name__ == "__main__":
    EXECUTABLE = Path(sys.argv[1]).resolve()
    APPLICATION = sys.argv[2]
    HTML_DIR = Path(sys.argv[3]).resolve()
    DEFAULT_DIR = sys.argv[4]
    unittest.main(argv=[sys.argv[0]])

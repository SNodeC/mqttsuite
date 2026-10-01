# Install MQTTSuite

[← MQTTSuite](../../README.md)

## Choose your route

| Goal | Route |
| --- | --- |
| Binary packages | [Signed distribution packages](packages.md). |
| OpenWrt | [Packages and SDK route](packages.md#openwrt). |
| Build from source | [Develop against current source or customize a build](#build-from-source). |

## Install binary packages

The [distribution package guide](packages.md) lists releases, architectures and feed setup. Install only the applications you need; their package dependencies bring in the required SNode.C components automatically.

After configuring the signed feed:

```sh
# Debian, Ubuntu, Raspberry Pi OS
sudo apt-get install mqttsuite-broker mqttsuite-cli
```

```sh
# Fedora, Rocky Linux
sudo dnf install mqttsuite-broker mqttsuite-cli
```

For all five applications and both mapping plugins, install the `mqttsuite` metapackage on DEB/RPM systems. OpenWrt uses `mqttsuite-full`; see the [package guide](packages.md).

Installing software is not the same as enabling a public broker. Configure interfaces, authentication/access controls, TLS and state paths before starting an unattended service.

## Build from source

### Requirements

Install a compatible SNode.C development package first. Current MQTTSuite CMake files request **SNode.C 2.0.0** using its package compatibility rules. Build both from compatible revisions if you are working on their APIs; do not combine arbitrary old libraries and current headers.

The suite requires C++20 and CMake 3.14+. The full build also needs the SNode.C components for the chosen transports and MariaDB integration. On Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install git cmake ninja-build g++ pkg-config \
  nlohmann-json3-dev libssl-dev libmariadb-dev
```

### Build and install

```sh
git clone --recurse-submodules https://github.com/SNodeC/mqttsuite.git
cd mqttsuite
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

If SNode.C is in a custom prefix, add `-DCMAKE_PREFIX_PATH=/path/to/prefix`. Database server provisioning is separate from installing the connector library and MQTTStore executable.

### Select build features

Transport options are application-specific, such as `CONFIG_MQTTSUITE_BROKER_TCP_IPV4`, `CONFIG_MQTTSUITE_BROKER_TLS_IPV4` and `CONFIG_MQTTSUITE_BROKER_WS`. Inspect the CMake cache and each application’s CMake file before producing a reduced build. Examples using an omitted instance will not work unchanged.

## Verify the installation

```sh
mqttbroker --config-file /dev/null --help
mqttcli --config-file /dev/null --help
mqttbridge --config-file /dev/null bridge --help
mqttstore --config-file /dev/null in-mqtt --disabled=false --help=expanded
```

Run the checks for the applications you installed. Keep source installations and package-managed installations separate; stale libraries or WebSocket plugins can otherwise be selected at runtime.

## Next step

Run the [loopback publish/subscribe example](../../README.md#publish-your-first-message).

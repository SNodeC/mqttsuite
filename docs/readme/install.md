# Build and install MQTTSuite from source

<p>
  <a href="../../README.md"><img src="media/menu/back-mqttsuite.svg" alt="← MQTTSuite" width="110" height="24"></a>
</p>

<p>
  <a href="#native-linux-build" title="Native Linux build"><img src="media/install-native-linux.svg" alt="Native Linux build" width="222" height="24"></a>
  <a href="#openwrt-cross-compilation" title="OpenWrt cross-compilation"><img src="media/install-openwrt.svg" alt="OpenWrt cross-compilation" width="222" height="24"></a>
</p>

**Prefer prebuilt binaries?** See [Packages](https://github.com/SNodeC/Packages#readme) for supported distributions and installation instructions.

## Native Linux build

Build and install on the Linux machine that will run the software.

### Requirements

Install **SNode.C before configuring MQTTSuite**. Use [SNode.C’s prebuilt DEB/RPM packages](https://github.com/SNodeC/Packages#readme) (which include headers and CMake files), or follow the [SNode.C source-build guide](https://github.com/SNodeC/snode.c/blob/master/docs/readme/install.md#native-linux-build). OpenWrt runtime packages do not provide the development files needed for a native build. Current MQTTSuite CMake files request **SNode.C 2.0.0** using its package compatibility rules. Build both from compatible revisions if you are working on their APIs; do not combine arbitrary old libraries and current headers.

The suite requires C++20 and CMake 3.18+. The full build also needs the SNode.C components for the chosen transports and MariaDB integration. On Debian/Ubuntu:

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

### Verify the installation

```sh
mqttbroker --help
mqttcli --help
mqttbridge \
	bridge --help
mqttstore \
	in-mqtt --help=expanded
```

Run the checks for the applications you installed. Keep source installations and package-managed installations separate; stale libraries or WebSocket plugins can otherwise be selected at runtime.

### Next step

Run the [loopback publish/subscribe example](../../README.md#publish-your-first-message).

## OpenWrt cross-compilation

Build MQTTSuite packages with an official OpenWrt SDK matching the device's release, target and ABI. Use the [MQTTSuite recipe](../../misc/openwrt/Makefile) in this repository’s `misc/openwrt/` directory. The recipe uses the upstream build system without source patches. OpenWrt package selections are separate from native CMake options.

### Prepare the SDK

Follow [SNode.C's SDK preparation instructions](https://github.com/SNodeC/snode.c/blob/master/docs/readme/install.md#prepare-the-sdk) to choose, verify and extract the official SDK. Continue below in that SDK; OpenWrt will build SNode.C automatically as MQTTSuite's dependency. No published SNode.C SDK archive is required.

The recipes are included in the upstream checkouts. Clone whichever checkout you do not already have:

```sh
git clone https://github.com/SNodeC/snode.c.git
git clone https://github.com/SNodeC/mqttsuite.git
```

### Link the recipes into the SDK

Run the following inside the extracted SDK. Replace `/path/to/snode.c` and `/path/to/mqttsuite` with the absolute paths of your checkouts:

```sh
./scripts/feeds update base packages
./scripts/feeds install nlohmannjson libopenssl libmagic bluez-libs libmariadb
mkdir -p package/local
ln -s /path/to/snode.c/supplement/openwrt package/local/snode.c
ln -s /path/to/mqttsuite/misc/openwrt package/local/mqttsuite
```

The symlinks use the recipes from your current checkouts and reflect recipe changes automatically. Keep the checkouts at those locations while using the SDK.

Each recipe declares its default source release. To select other releases, export `SNODEC_SOURCE_TAG=vX.Y.Z` and `MQTTSUITE_SOURCE_TAG=vA.B.C` with compatible real release tags before invoking `make`; each recipe derives its package version from its tag. Record both recipe commits and selected source tags to reproduce the build.

### Select and compile MQTTSuite

In a fresh SDK, initialize the package selection:

```sh
cat > .config <<'EOF'
# CONFIG_ALL is not set
# CONFIG_ALL_NONSHARED is not set
# CONFIG_ALL_KMODS is not set
# CONFIG_SIGNED_PACKAGES is not set
# CONFIG_AUTOREMOVE is not set
CONFIG_PACKAGE_mqttsuite=m
EOF
make defconfig
make -j16 package/local/mqttsuite/compile V=s
```

OpenWrt builds and stages SNode.C before compiling MQTTSuite, as declared by the recipe’s build dependency. No separate SNode.C build command is needed.

This example creates unsigned packages for local use. Adjust the parallel job count to the build host. In an existing SDK, retain your `.config` and change selections through `make menuconfig` instead of replacing it.

Use **Network / MQTTSuite** in `make menuconfig`. `CONFIG_PACKAGE_<package>=m` builds an installable package; `y` also selects it for an image build; `n` omits it unless required by a selected consumer. `mqttsuite` selects all five applications and both mapping plugins. Required SNode.C layers are selected automatically. See the [package catalog](https://github.com/SNodeC/Packages/blob/main/docs/mqttsuite-package-options.md) for contents.

MQTTSuite compiles only selected applications. Each has eight transport options; WebSocket plugins compile and ship only with WS enabled. TLS requires its base socket family; WSS requires WS and an enabled TLS family. WS requires a socket family. IPv4 TCP is retained when both IPv6 and Unix sockets are disabled. Unix-socket TLS defaults to disabled. Mapping plugins are selected separately for packaging.

<details>
<summary>MQTTSuite transport defaults</summary>

For source builds, package selection uses `CONFIG_PACKAGE_<name>` with ordinary n/m/y semantics and automatic dependency selection. Build defaults do not replace package selectors.

All rows have the `CONFIG_` prefix in `.config`. Enabling a row compiles that application's endpoint support and selects the matching SNode.C packages. WS/WSS select the appropriate client/server WebSocket MQTT modules. Both share one application plugin; it is absent when WS is disabled.

| Config.in symbol | Default | Required options |
| --- | --- | --- |
| `MQTTSUITE_BROKER_TCP_IPV4` | `y` | None |
| `MQTTSUITE_BROKER_TLS_IPV4` | `y` | MQTTSUITE_BROKER_TCP_IPV4 |
| `MQTTSUITE_BROKER_TCP_IPV6` | `y` | None |
| `MQTTSUITE_BROKER_TLS_IPV6` | `y` | MQTTSUITE_BROKER_TCP_IPV6 |
| `MQTTSUITE_BROKER_UNIX` | `y` | None |
| `MQTTSUITE_BROKER_UNIX_TLS` | `n` | MQTTSUITE_BROKER_UNIX |
| `MQTTSUITE_BROKER_WS` | `y` | MQTTSUITE_BROKER_TCP_IPV4 or MQTTSUITE_BROKER_TCP_IPV6 or MQTTSUITE_BROKER_UNIX |
| `MQTTSUITE_BROKER_WSS` | `y` | MQTTSUITE_BROKER_WS; MQTTSUITE_BROKER_TLS_IPV4 or MQTTSUITE_BROKER_TLS_IPV6 or MQTTSUITE_BROKER_UNIX_TLS |
| `MQTTSUITE_INTEGRATOR_TCP_IPV4` | `y` | None |
| `MQTTSUITE_INTEGRATOR_TLS_IPV4` | `y` | MQTTSUITE_INTEGRATOR_TCP_IPV4 |
| `MQTTSUITE_INTEGRATOR_TCP_IPV6` | `y` | None |
| `MQTTSUITE_INTEGRATOR_TLS_IPV6` | `y` | MQTTSUITE_INTEGRATOR_TCP_IPV6 |
| `MQTTSUITE_INTEGRATOR_UNIX` | `y` | None |
| `MQTTSUITE_INTEGRATOR_UNIX_TLS` | `n` | MQTTSUITE_INTEGRATOR_UNIX |
| `MQTTSUITE_INTEGRATOR_WS` | `y` | MQTTSUITE_INTEGRATOR_TCP_IPV4 or MQTTSUITE_INTEGRATOR_TCP_IPV6 or MQTTSUITE_INTEGRATOR_UNIX |
| `MQTTSUITE_INTEGRATOR_WSS` | `y` | MQTTSUITE_INTEGRATOR_WS; MQTTSUITE_INTEGRATOR_TLS_IPV4 or MQTTSUITE_INTEGRATOR_TLS_IPV6 or MQTTSUITE_INTEGRATOR_UNIX_TLS |
| `MQTTSUITE_BRIDGE_TCP_IPV4` | `y` | None |
| `MQTTSUITE_BRIDGE_TLS_IPV4` | `y` | MQTTSUITE_BRIDGE_TCP_IPV4 |
| `MQTTSUITE_BRIDGE_TCP_IPV6` | `y` | None |
| `MQTTSUITE_BRIDGE_TLS_IPV6` | `y` | MQTTSUITE_BRIDGE_TCP_IPV6 |
| `MQTTSUITE_BRIDGE_UNIX` | `y` | None |
| `MQTTSUITE_BRIDGE_UNIX_TLS` | `n` | MQTTSUITE_BRIDGE_UNIX |
| `MQTTSUITE_BRIDGE_WS` | `y` | MQTTSUITE_BRIDGE_TCP_IPV4 or MQTTSUITE_BRIDGE_TCP_IPV6 or MQTTSUITE_BRIDGE_UNIX |
| `MQTTSUITE_BRIDGE_WSS` | `y` | MQTTSUITE_BRIDGE_WS; MQTTSUITE_BRIDGE_TLS_IPV4 or MQTTSUITE_BRIDGE_TLS_IPV6 or MQTTSUITE_BRIDGE_UNIX_TLS |
| `MQTTSUITE_CLI_TCP_IPV4` | `y` | None |
| `MQTTSUITE_CLI_TLS_IPV4` | `y` | MQTTSUITE_CLI_TCP_IPV4 |
| `MQTTSUITE_CLI_TCP_IPV6` | `y` | None |
| `MQTTSUITE_CLI_TLS_IPV6` | `y` | MQTTSUITE_CLI_TCP_IPV6 |
| `MQTTSUITE_CLI_UNIX` | `y` | None |
| `MQTTSUITE_CLI_UNIX_TLS` | `n` | MQTTSUITE_CLI_UNIX |
| `MQTTSUITE_CLI_WS` | `y` | MQTTSUITE_CLI_TCP_IPV4 or MQTTSUITE_CLI_TCP_IPV6 or MQTTSUITE_CLI_UNIX |
| `MQTTSUITE_CLI_WSS` | `y` | MQTTSUITE_CLI_WS; MQTTSUITE_CLI_TLS_IPV4 or MQTTSUITE_CLI_TLS_IPV6 or MQTTSUITE_CLI_UNIX_TLS |
| `MQTTSUITE_STORE_TCP_IPV4` | `y` | None |
| `MQTTSUITE_STORE_TLS_IPV4` | `y` | MQTTSUITE_STORE_TCP_IPV4 |
| `MQTTSUITE_STORE_TCP_IPV6` | `y` | None |
| `MQTTSUITE_STORE_TLS_IPV6` | `y` | MQTTSUITE_STORE_TCP_IPV6 |
| `MQTTSUITE_STORE_UNIX` | `y` | None |
| `MQTTSUITE_STORE_UNIX_TLS` | `n` | MQTTSUITE_STORE_UNIX |
| `MQTTSUITE_STORE_WS` | `y` | MQTTSUITE_STORE_TCP_IPV4 or MQTTSUITE_STORE_TCP_IPV6 or MQTTSUITE_STORE_UNIX |
| `MQTTSUITE_STORE_WSS` | `y` | MQTTSUITE_STORE_WS; MQTTSUITE_STORE_TLS_IPV4 or MQTTSUITE_STORE_TLS_IPV6 or MQTTSUITE_STORE_UNIX_TLS |

The IPv4 TCP switch is mandatory when both IPv6 TCP and Unix sockets are off. Integrator and bridge have upstream unconditional IPv4 HTTP and HTTPS admin servers, so disabling MQTT TLS does not remove their admin TLS dependency.

All 40 transport symbols and the five application selectors participate in recipe reconfiguration. Mapping plugin selectors govern separate package emission. SNode.C is a build/runtime dependency.

</details>

<details>
<summary>WebSocket loading and RPATH</summary>

#### WebSocket loading and RPATH

The SDK build dependency builds and stages SNode.C first. MQTTSuite uses a separate CMake build directory so its private `lib/Log.h` cannot shadow SNode.C's public `Log.h`. Every recipe configuration option participates in OpenWrt's reconfiguration stamp. The recipes use upstream build systems without source patches.

The HTTP loader opens `/usr/lib/snode.c/web/http/upgrade/libsnodec-websocket-{server,client}.so.<ABI>`. The WebSocket subprotocol loader then opens the application-specific `/usr/lib/snode.c/web/http/upgrade/websocket/mqtt<app>/libsnodec-websocket-mqtt-{server,client}.so.<ABI>`. MQTTSuite's plugin SONAME follows SNode.C's ABI major, while the real plugin filename follows MQTTSuite's release version. Both the real file and ABI symlink are packaged. Ordinary MQTTSuite libraries use MQTTSuite's ABI major.

The plugin directory identifies the `dlopen` object; its dependencies still need the correct ELF library search paths. Before OpenWrt runs `rstrip`, the MQTTSuite recipe removes only the literal staging-directory prefix from each RPATH/RUNPATH entry. It preserves target subdirectories, `$ORIGIN`, unrelated entries, permissions, and the original tag type; patchelf errors fail the build. It does not delete or shrink all RPATHs. OpenWrt may subsequently remove ordinary system-directory entries; the packaged libraries retain the remaining paths on each ELF file against that package's dependency closure.

</details>

### Install and verify on the device

On the matching router firmware, install the desired APKs with their dependencies (`apk add --allow-untrusted ./<package>.apk` for these unsigned local builds). Supply all referenced local SNode.C packages or a local feed; installing a single meta-package alone cannot discover unpublished packages.

The three existing services (`mqttbroker`, `mqttintegrator`, `mqttbridge`) run in the foreground under procd and send logs to logd. They load SNode.C's existing `/etc/snode.c/<application>.conf` configuration files. The recipe preserves `/etc/snode.c/` across upgrades. The bridge service starts only once `/etc/snode.c/mqttbridge.conf` exists; configure its `bridge.definition` with a valid bridge JSON file. Its packaged web assets are supplied through `bridge --html-dir /usr/var/www/mqttsuite/mqttbridge`. No site-specific remote brokers are built into the service.

MQTTStore and the CLI are independent executable packages. Configure database credentials/storage projections and MQTT endpoints before running MQTTStore. TLS endpoints need suitable certificates, keys and trust settings.

Then check application help/configuration, native MQTT, WS and WSS connections, service restart/logging, bridge forwarding, integrator mappings, and MQTTStore writes.

<p align="center"><img src="./docs/assets/README/Logo-New-Full.png" style="width: 50%;" /></p>

---

# MQTTSuite

### Overview

The [**MQTTSuite**](https://snodec.github.io/mqttsuite-doc/html/index.html) is a lightweight, production-ready MQTT integration system composed of four focused applications—**MQTTBroker**, **MQTTIntegrator**, **MQTTBridge**, and **MQTTCli**—all powered by [**SNode.C**](https://github.com/SNodeC/snode.c), a single-threaded, single-tasking C++ networking framework. Thanks to its very small footprint, MQTTSuite is especially suitable for resource-constrained systems (embedded Linux, routers, SBCs).

### Components

| Component | Role | Highlights |
|---|---|---|
| **MQTTBroker** | Full MQTT broker (MQTT 3.1.1) | Encrypted/unencrypted TCP and WebSockets; IPv4/IPv6 & UNIX domain sockets; built-in Web UI (live clients) |
| **MQTTIntegrator** | Mapping-driven IoT translator | Subscribes → transforms (INJA templates) → republishes; wildcards (`+`, `#`); fine-grained QoS/retain |
| **MQTTBridge** | Pure client-side bridge | Multiple logical bridges; connects to multiple brokers; selectively bridges topics among them |
| **MQTTCli** | Command-line client | Subscribe to topics and publish messages; supports TCP, WebSockets, and UNIX sockets |

---

### Table of Content

<!--ts-->
* [MQTTSuite](#mqttsuite)
      * [Overview](#overview)
      * [Components](#components)
      * [Table of Content](#table-of-content)
      * [License](#license)
      * [Copyright](#copyright)
      * [Installation](#installation)
         * [Supported Systems &amp; Hardware](#supported-systems--hardware)
         * [Minimum Required Compiler Versions](#minimum-required-compiler-versions)
         * [Requirements &amp; Dependencies](#requirements--dependencies)
            * [Tools](#tools)
            * [Frameworks](#frameworks)
            * [Libraries](#libraries)
         * [Installation on Debian-Style Systems (x86-64, ARM)](#installation-on-debian-style-systems-x86-64-arm)
            * [Install SNode.C](#install-snodec)
            * [Install system packages](#install-system-packages)
            * [Build &amp; Install MQTTSuite](#build--install-mqttsuite)
         * [Deployment on OpenWrt](#deployment-on-openwrt)
            * [Choose &amp; download an SDK](#choose--download-an-sdk)
            * [Patch the SDK to integrate the MQTTSuite feed](#patch-the-sdk-to-integrate-the-mqttsuite-feed)
            * [Install the MQTTSuite package and its dependencies](#install-the-mqttsuite-package-and-its-dependencies)
            * [Configure the SDK](#configure-the-sdk)
            * [Cross-compile MQTTSuite](#cross-compile-mqttsuite)
            * [Deploy MQTTSuite](#deploy-mqttsuite)
         * [Post-Install Tips](#post-install-tips)
   * [MQTTBroker](#mqttbroker)
      * [Overview &amp; Configuration](#overview--configuration)
         * [Connection instances of default builds of <em>MQTTBroker</em>](#connection-instances-of-default-builds-of-mqttbroker)
         * [Configuration](#configuration)
         * [Notes](#notes)
      * [Quick Start (Recommended Flow)](#quick-start-recommended-flow)
      * [Connection Methods](#connection-methods)
         * [MQTT over TCP/IP](#mqtt-over-tcpip)
         * [MQTT over UNIX Domain Sockets](#mqtt-over-unix-domain-sockets)
         * [MQTT over WebSockets](#mqtt-over-websockets)
         * [MQTT over UNIX Domain WebSockets](#mqtt-over-unix-domain-websockets)
      * [Web Interface](#web-interface)
         * [Web Interface over TCP/IP](#web-interface-over-tcpip)
         * [Web Interface over UNIX Domain Sockets](#web-interface-over-unix-domain-sockets)
      * [Additions &amp; Field-Tested Guidance](#additions--field-tested-guidance)
         * [Quick TLS checklist for MQTT(TCP) and MQTT over WebSockets](#quick-tls-checklist-for-mqtttcp-and-mqtt-over-websockets)
            * [Example (TCP/TLS):](#example-tcptls)
            * [Example (WebSockets/TLS):](#example-websocketstls)
         * [Making configuration persistent (recommended)](#making-configuration-persistent-recommended)
         * [WebSockets specifics that often trip clients](#websockets-specifics-that-often-trip-clients)
         * [Quick sanity checks with common tools](#quick-sanity-checks-with-common-tools)
         * [Quick sanity checks with MQTTSuites command line tool](#quick-sanity-checks-with-mqttsuites-command-line-tool)
         * [Notes for UNIX domain sockets](#notes-for-unix-domain-sockets)
         * [Embedded MQTTIntegrator (when --mqtt-mapping-file is provided)](#embedded-mqttintegrator-when---mqtt-mapping-file-is-provided)
         * [Extending transports](#extending-transports)
         * [Diagnostics, hardening &amp; tips](#diagnostics-hardening--tips)
      * [Protocol version note](#protocol-version-note)
      * [Why this layout works well in production](#why-this-layout-works-well-in-production)
   * [MQTT Mapping Description](#mqtt-mapping-description)
      * [Purpose](#purpose)
      * [Top-Level Structure](#top-level-structure)
         * [connection options (summary)](#connection-options-summary)
         * [The mapping Object (big picture)](#the-mapping-object-big-picture)
            * [Minimal shapes](#minimal-shapes)
            * [Wildcard examples](#wildcard-examples)
      * [Subscriptions &amp; Translation Rules](#subscriptions--translation-rules)
      * [Mapping Sections](#mapping-sections)
      * [Optional: plugins](#optional-plugins)
      * [Quick Start (Recommended Flow)](#quick-start-recommended-flow-1)
         * [Skeleton mapping file](#skeleton-mapping-file)
         * [Add a concrete subscription with a simple static mapping](#add-a-concrete-subscription-with-a-simple-static-mapping)
         * [Switch to a template mapping when you need logic](#switch-to-a-template-mapping-when-you-need-logic)
         * [Validate against the schema (example with ajv-cli)](#validate-against-the-schema-example-with-ajv-cli)
         * [Run the integrator with your mapping](#run-the-integrator-with-your-mapping)
      * [Field-Tested Guidance](#field-tested-guidance)
      * [Schema Highlights](#schema-highlights)
      * [In one sentence](#in-one-sentence)
   * [MQTTIntegrator](#mqttintegrator)
      * [Overview &amp; Configuration](#overview--configuration-1)
         * [Connection instances of default builds of <em>MQTTIntegrator</em>](#connection-instances-of-default-builds-of-mqttintegrator)
         * [Configuration](#configuration-1)
         * [Notes](#notes-1)
      * [Quick Start (Recommended Flow)](#quick-start-recommended-flow-2)
         * [Disable all but the in-mqtt instance](#disable-all-but-the-in-mqtt-instance)
         * [Connect to a plain MQTT broker over TCP/IPv4](#connect-to-a-plain-mqtt-broker-over-tcpipv4)
         * [Add a persistent session store (survives restarts)](#add-a-persistent-session-store-survives-restarts)
         * [Attach your mapping file (canonical test case below)](#attach-your-mapping-file-canonical-test-case-below)
         * [Persist your configuration using -w](#persist-your-configuration-using--w)
         * [(Optional) Use TLS (MQTTS) instead of plain TCP](#optional-use-tls-mqtts-instead-of-plain-tcp)
         * [(Optional) Use WebSockets (WSS)](#optional-use-websockets-wss)
         * [(Optional) Use Unix Domain WebSockets](#optional-use-unix-domain-websockets)
      * [Connection Methods](#connection-methods-1)
         * [MQTT over TCP/IP](#mqtt-over-tcpip-1)
         * [MQTT over UNIX Domain Sockets](#mqtt-over-unix-domain-sockets-1)
         * [MQTT over WebSockets](#mqtt-over-websockets-1)
         * [MQTT over UNIX Domain WebSockets](#mqtt-over-unix-domain-websockets-1)
         * [Notes](#notes-2)
      * [Canonical Mapping &amp; Test Case](#canonical-mapping--test-case)
      * [Behavior aligned with the Mapping Description](#behavior-aligned-with-the-mapping-description)
      * [Additions &amp; Field-Tested Guidance](#additions--field-tested-guidance-1)
         * [TLS checklist for client connections (MQTTS / WSS / UDS-WSS)](#tls-checklist-for-client-connections-mqtts--wss--uds-wss)
            * [Example (TCP/TLS) with mutual TLS:](#example-tcptls-with-mutual-tls)
            * [Example (WebSockets/TLS):](#example-websocketstls-1)
            * [Example (Unix Domain WebSockets, TLS layered on the WS endpoint):](#example-unix-domain-websockets-tls-layered-on-the-ws-endpoint)
         * [Persist-once workflow (recommended)](#persist-once-workflow-recommended)
         * [WebSockets specifics that often trip clients](#websockets-specifics-that-often-trip-clients-1)
         * [Diagnostics, hardening &amp; common pitfalls](#diagnostics-hardening--common-pitfalls)
      * [Protocol version note](#protocol-version-note-1)
      * [Why this layout works well in production](#why-this-layout-works-well-in-production-1)
   * [MQTTBridge](#mqttbridge)
      * [Overview &amp; Configuration](#overview--configuration-2)
      * [Quick Start (Recommended Flow)](#quick-start-recommended-flow-3)
         * [Create a minimal bridge-config.json](#create-a-minimal-bridge-configjson)
         * [Run the bridge:](#run-the-bridge)
         * [Verify (example):](#verify-example)
      * [Bridge Configuration JSON](#bridge-configuration-json)
         * [Overall Structure (at a glance)](#overall-structure-at-a-glance)
         * [Elements in Detail](#elements-in-detail)
            * [Root Object](#root-object)
            * [bridge (each item in bridges)](#bridge-each-item-in-bridges)
            * [broker (each item in brokers)](#broker-each-item-in-brokers)
            * [mqtt (client options)](#mqtt-client-options)
            * [topics (subscriptions for a broker)](#topics-subscriptions-for-a-broker)
            * [network (transport selection + addressing)](#network-transport-selection--addressing)
      * [Best Practices &amp; Validation Tips](#best-practices--validation-tips)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->
<!-- Added by: runner, at: Sun Nov  9 22:44:45 UTC 2025 -->

<!--te-->

### License

**SNode.C** is dual-licensed under **MIT** and **GPL-3.0-or-later**. You may choose either license when using the framework.

`SPDX-License-Identifier: MIT OR GPL-3.0-or-later`

> Note: MQTTSuite applications are built on top of SNode.C and follow the respective licensing terms of their repositories and bundled libraries.

### Copyright

Volker Christian ([me@vchrist.at](mailto:me@vchrist.at), [volker.christian@fh-hagenberg.at](mailto:volker.christian@fh-hagenberg.at))

---

### Installation

Installation is straightforward:

1. **Install the SNode.C framework.**
2. **Install required tools and libraries.**
3. **Clone, build, and install MQTTSuite.**

Use the detailed platform-specific instructions below.

---

#### Supported Systems & Hardware

Development is primarily on **Debian Sid**; since **Debian Bookworm** it also builds cleanly on Debian stable. With the required tools and libraries installed, it should compile on most Linux distributions.

Known good targets:

- **x86-64** (PC/servers)
- **ARM 32/64-bit** (e.g., Raspberry Pi 3/4/5)
- **OpenWrt** 23.05.0 and later (all architectures)
- **Android** via [Termux](https://termux.dev/en/) *(documentation in preparation)*

---

#### Minimum Required Compiler Versions

SNode.C leverages C++20 features; therefore recent compilers are required.

- **GCC ≥ 12.2**
- **Clang ≥ 13.0**

Either toolchain is supported.

---

#### Requirements & Dependencies

Some tools and libraries must be present. A few libraries are bundled within MQTTSuite.

##### Tools

**Mandatory**

- `git` — <https://git-scm.com/>
- `cmake` — <https://cmake.org/>
- `make` — <https://www.gnu.org/software/make/> **or**
- `ninja` — <https://ninja-build.org/>
- `g++` — <https://gcc.gnu.org/> **or**
- `clang` — <https://clang.llvm.org/>
- `pkg-config` — <https://www.freedesktop.org/wiki/Software/pkg-config/>

**Optional (useful for development/QA)**

- `iwyu` — <https://include-what-you-use.org/>
- `clang-format` — <https://clang.llvm.org/docs/ClangFormat.html>
- `cmake-format` — <https://cmake-format.readthedocs.io/>
- `doxygen` — <https://www.doxygen.nl/>

##### Frameworks

**Mandatory**

- **SNode.C** – Simple NODE in C++: <https://github.com/SNodeC/snode.c>

##### Libraries

**Mandatory**

- **libfmt** (≥ 11.0.0) development files — <https://github.com/fmtlib/fmt>

**Bundled (no separate installation required)**

- **JSON Schema Validator for Modern C++** — <https://github.com/pboettch/json-schema-validator>
- **INJA**: A Template Engine for Modern C++ — <https://github.com/pantor/inja>

---

#### Installation on Debian-Style Systems (x86-64, ARM)

##### Install SNode.C

Follow the SNode.C installation guide:  
<https://github.com/SNodeC/snode.c?tab=readme-ov-file#installation>

##### Install system packages

```sh
sudo apt update
sudo apt install libfmt-dev
## Recommended for building:
## sudo apt install git cmake build-essential ninja-build pkg-config
```

##### Build & Install MQTTSuite

```sh
mkdir mqttsuite
cd mqttsuite
git clone --recurse-submodules https://github.com/SNodeC/mqttsuite.git
mkdir build
cd build
cmake ../mqttsuite        ## optionally: -G Ninja -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"         ## or: ninja
sudo make install         ## or: sudo ninja install
sudo ldconfig
```

> Tip: Use all CPU threads (`-j$(nproc)`) to speed up the build—especially useful on SBCs.

---

#### Deployment on OpenWrt

*Assumptions:* You have **SSH** and **SFTP** access to the router, and WAN connectivity is configured.

> You **do not** install SNode.C manually; the OpenWrt build will pull it in automatically via the feed.

High-level steps:

1. **Choose and download an OpenWrt SDK**
2. **Patch the SDK to add the SNode.C (snodec) feed**
3. **Install the MQTTSuite package and dependencies**
4. **Configure the SDK**
5. **Cross-compile MQTTSuite**
6. **Deploy the resulting `.ipk` packages to the router**

##### Choose & download an SDK

Download an SDK (23.05.0-rc1 or later) from the OpenWrt download site and extract it to `<DIR>`.

Example (Netgear MR8300, IPQ4019):

```sh
cd <DIR>
wget -qO - https://downloads.openwrt.org/releases/23.05.0-rc2/targets/ipq40xx/generic/openwrt-sdk-23.05.0-rc2-ipq40xx-generic_gcc-12.3.0_musl_eabi.Linux-x86_64.tar.xz | tar Jx
```

This creates an SDK directory like:
`openwrt-sdk-<version>-<arch>-<atype>_<compiler>-<cver>_<libc>_<abi>.Linux-x86_64` → referred to as `<SDK_DIR>`.

For the example above:

- `<version>` = 23.05.0-rc2
- `<arch>` = ipq40xx
- `<atype>` = generic
- `<compiler>` = gcc
- `<cver>` = 12.3.0
- `<libc>` = musl
- `<abi>` = eabi

##### Patch the SDK to integrate the MQTTSuite feed

```sh
cd <SDK_DIR>
echo "src-git snodec https://github.com/SNodeC/OpenWRT" >> feeds.conf.default
```

##### Install the MQTTSuite package and its dependencies

```sh
cd <SDK_DIR>
./scripts/feeds update base packages snodec    ## only these feeds are needed
./scripts/feeds install mqttsuite
```

##### Configure the SDK

```sh
cd <SDK_DIR>
make defconfig
```

You can keep defaults. To customize SNode.C options:

```sh
cd <SDK_DIR>
make menuconfig
## Navigate: Network -> SNode.C
```

##### Cross-compile MQTTSuite

```sh
cd <SDK_DIR>
make package/mqttsuite/compile -j"$(nproc)"
```

What happens:

1. Feeds and package metadata are refreshed locally.
2. All direct/indirect dependencies are fetched and built as needed.
3. MQTTSuite is cloned and cross-compiled.

> After building, the `.ipk` packages for **MQTTSuite** and **SNode.C** are usually found under:  
> `<SDK_DIR>/bin/packages/<architecture>/snodec/`

##### Deploy MQTTSuite

Copy and install the generated packages onto the router:

```sh
cd <SDK_DIR>/bin/packages/<architecture>/snodec
sftp root@<router-ip>
cd /tmp
put snode.c*.ipk
put mqttsuite*.ipk
exit

ssh root@<router-ip>
cd /tmp
opkg install snode.c*.ipk mqttsuite*.ipk   ## add --force-reinstall to overwrite same version
exit
```

> On first installation you may see errors about missing dependencies from the base OpenWrt repos. Install those with `opkg` and rerun the `opkg install` for MQTTSuite/SNode.C.

During package installation, a new **UNIX group** (with member `root`) is created and used for managing config, log, and PID files.  
**Note:** log out and log in again to activate the new group membership.

---

#### Post-Install Tips

- **Persist-once workflow:** All MQTTSuite applications support `--write-config` / `-w`. Start once with explicit flags, verify, then run with no flags.
- **Use UNIX domain sockets** for same-host integrations; they reduce overhead and surface area.
- **TLS everywhere off-host:** Use TLS (and client auth where appropriate) when crossing untrusted networks.
- **Parallel builds:** Always use `-j$(nproc)` (or Ninja) to accelerate compiles.

---

## MQTTBroker

### Overview & Configuration

The *MQTTBroker* is the central broker of the **MQTTSuite** and supports multiple connection instances across several transport protocols. Additional transports (e.g., Bluetooth) can be added in the [`mqttbroker.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttbroker/mqttbroker.cpp) source code.

#### Connection instances of default builds of *MQTTBroker*

- `in-mqtt`: MQTT over IPv4
- `in-mqtts`: MQTTS over IPv4
- `in6-mqtt`: MQTT over IPv6
- `in6-mqtts`: MQTTS over IPv6
- `un-mqtt`: MQTT over Unix Domain Sockets
- `un-mqtts`: MQTTS over Unix Domain Sockets
- `in-http`: Web Interface and MQTT over WS via IPv4
- `in-https`: Web Interface and MQTTS over WSS over IPv4
- `in6-http`: Web Interface and MQTT over WS via IPv6
- `in6-https`: Web Interface and MQTTS over WSS via IPv6
- `un-http`: Web Interface and MQTT over WS via Unix Domain Sockets
- `un-https`: Web Interface and MQTTS over WSS via Unix Domain Sockets

One can control which instances are built by enabling or disabling them through the available `cmake` options.

#### Configuration

All aspects—such as port numbers, UNIX *sun* paths, and per-listener options—are configurable via the [SNode.C configuration system](https://github.com/SNodeC/snode.c?tab=readme-ov-file#configuration). You can set options in code, on the command line, or by using a configuration file.

If encrypted access is required, [suitable certificates need to be provided](https://github.com/SNodeC/snode.c?tab=readme-ov-file#ssltls-configuration-section-tls).

#### Notes

- **Persistent sessions:** Configure a *session store* if you want client sessions to survive broker restarts by adding the command-line option  
  `--mqtt-session-store <path-to-session-store-file>`.
- **Embedded integrator:** If the MQTTBroker should also act as an integrated **MQTTIntegrator**, provide a *[mapping description file](#mqtt-mapping-description)* via  
  `--mqtt-mapping-file <path-to-mqtt-mapping-file.json>`.
- **Web UI templates:** The path to the HTML templates for the MQTTBroker Web Interface can be set with  
  `--html-dir <dir-of-html-templates>`. The default directory `/var/www/mqttsuite/mqttbroker` is already configured in [`mqttbroker.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttbroker/mqttbroker.cpp).
- **Persisting options:** All three options above can be made *persistent* by storing their values in a configuration file; append `--write-config` or `-w` to the command line.

---

### Quick Start (Recommended Flow)

Per default all supported connection instances are enabled. Unused instances need to be disabled.

1. **Disable all but the `in-mqtt` instance**

   ```bash
   mqttbroker in-mqtts --disabled
              in6-mqtt --disabled
              in6-mqtts -disabled
              un-mqtt --disabled
              un-mqtts --disabled
              in-http --disabled
              in-https --disabled
              in6-http --disabled
              in6-https --disabled
              un-http --disabled
              un-https --disabled
   ```

2. **Start a plain MQTT listener on TCP/IPv4**  

   ```bash
   mqttbroker in-mqtt \
                  local --host 0.0.0.0 \
                        --port 1883
   ```

3. **Add a persistent session store** (survives restarts)  

   ```bash
   mqttbroker --mqtt-session-store /var/lib/mqttsuite/mqttbroker/session.store \
              in-mqtt \
                  local --host 0.0.0.0 \
                        --port 1883
   ```

4. **Enable the Web Interface (HTTP)**  

   ```bash
   mqttbroker --mqtt-session-store /var/lib/mqttsuite/mqttbroker/session.store \
              in-mqtt \
                  local --host 0.0.0.0 \
                        --port 1883 
              in-http \
                  --disabled=false \
                  local --host 0.0.0.0 \
                        --port 8080
   ```

5. **Persist your configuration** using `-w` (so you don’t repeat flags)  

   ```bash
   mqttbroker --mqtt-session-store /var/lib/mqttsuite/mqttbroker/session.store \
              in-mqtt \
                  local --host 0.0.0.0 \
                        --port 1883 
              in-http \
                  --disabled=false \
                  local --host 0.0.0.0 \
                        --port 8080 \
              -w
   ```

   From now on, a plain `mqttbroker` starts with the stored configuration.

6. **(Optional) Embed the integrator**  (after the configuration has been saved)

   ```bash
   mqttbroker --mqtt-mapping-file /etc/mqttsuite/mappings/mqtt-mapping.json
   ```

7. **(For development only) Use custom HTML templates** for the Web UI (after the configuration has been saved)

   ```bash
   mqttbroker --html-dir /var/www/mqttsuite/mqttbroker
   ```

---

### Connection Methods

#### MQTT over TCP/IP

MQTT clients can connect via *IPv4* and *IPv6*, using both encrypted and unencrypted channels.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Protocol | Encryption | Local Port |
| ------------------------------------------------------------ | -------- | ---------- | ---------- |
| **`in-mqtt`**                                                | IPv4     | No         | `1883`     |
| **`in-mqtts`**                                               | IPv4     | Yes        | `8883`     |
| **`in6-mqtt`**                                               | IPv6     | No         | `1883`     |
| **`in6-mqtts`**                                              | IPv6     | Yes        | `8883`     |

#### MQTT over UNIX Domain Sockets

For local (same-host) communication, the broker also supports UNIX domain sockets.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Encryption | Local Sun Path             |
| ------------------------------------------------------------ | ---------- | -------------------------- |
| **`un-mqtt`**                                                | No         | `/tmp/mqttbroker-un-mqtt`  |
| **`un-mqtts`**                                               | Yes        | `/tmp/mqttbroker-un-mqtts` |

#### MQTT over WebSockets

Clients can connect via WebSockets over both *IPv4* and *IPv6*.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Protocol | Encryption | Local Port | Request Target | Sec-WebSocket-Protocol |
| ------------------------------------------------------------ | -------- | ---------- | ---------- | -------------- | ---------------------- |
| **`in-http`**                                                | IPv4     | No         | `8080`     | `/` or `/ws`   | `mqtt`                 |
| **`in-https`**                                               | IPv4     | Yes        | `8088`     | `/` or `/ws`   | `mqtt`                 |
| **`in6-http`**                                               | IPv6     | No         | `8080`     | `/` or `/ws`   | `mqtt`                 |
| **`in6-https`**                                              | IPv6     | Yes        | `8088`     | `/` or `/ws`   | `mqtt`                 |

#### MQTT over UNIX Domain WebSockets

For local (same-host) communication, the broker also supports WebSockets over UNIX domain sockets.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Encryption | Local Sun Path             | Request Target | Sec-WebSocket-Protocol |
| ------------------------------------------------------------ | ---------- | -------------------------- | -------------- | ---------------------- |
| **`un-http`**                                                | No         | `/tmp/mqttbroker-un-http`  | `/` or `/ws`   | `mqtt`                 |
| **`un-https`**                                               | Yes        | `/tmp/mqttbroker-un-https` | `/` or `/ws`   | `mqtt`                 |

---

### Web Interface

#### Web Interface over TCP/IP

The *MQTTBroker Web Interface* provides real-time visibility into active client connections.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Protocol | Encryption | Local Port | Request Target    |
| ------------------------------------------------------------ | -------- | ---------- | ---------- | ----------------- |
| **`in-http`**                                                | IPv4     | No         | `8080`     | `/` or `/clients` |
| **`in-https`**                                               | IPv4     | Yes        | `8088`     | `/` or `/clients` |
| **`in6-http`**                                               | IPv6     | No         | `8080`     | `/` or `/clients` |
| **`in6-https`**                                              | IPv6     | Yes        | `8088`     | `/` or `/clients` |

#### Web Interface over UNIX Domain Sockets

For local (same-host) communication, the broker also supports WebSockets over UNIX domain sockets.

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Encryption | Local Sun Path             | Request Target    |
| ------------------------------------------------------------ | ---------- | -------------------------- | ----------------- |
| **`un-http`**                                                | No         | `/tmp/mqttbroker-un-http`  | `/` or `/clients` |
| **`un-https`**                                               | Yes        | `/tmp/mqttbroker-un-https` | `/` or `/client`  |

---

### Additions & Field-Tested Guidance

#### Quick TLS checklist for MQTT(TCP) and MQTT over WebSockets

- Prepare the server certificate chain and private key; include a CA file/dir if you validate client certs or upstreams.
- Apply via the **instance’s** `tls` section (e.g., `in-mqtts`, `in6-mqtts`, `in-https`, `in6-https`).
- Verify CN/SAN covers the hostname clients use.  
- Lock down key file permissions (e.g., `chmod 600` for the key, owned by the service user).
- After a successful run, append `-w` to persist TLS configuration.

##### Example (TCP/TLS):

```bash
mqttbroker in-mqtts \
               local --host 0.0.0.0 \
                     --port 8883 \
               tls --cert /etc/ssl/mqttsuite/server.crt \
                   --cert-key /etc/ssl/mqttsuite/server.key \
                   --ca-cert /etc/ssl/mqttsuite/ca.crt
```

##### Example (WebSockets/TLS):

```bash
mqttbroker in-https \
               local --host 0.0.0.0 \
                     --port 8088 \
               tls --cert /etc/ssl/mqttsuite/server.crt
                   --cert-key /etc/ssl/mqttsuite/server.key
```

---

#### Making configuration persistent (recommended)

- Start the broker with your desired CLI options; verify behavior.
- Run **once** with `-w` (or `--write-config`) to save the configuration.
- Thereafter, start with just `mqttbroker` (and override via flags only when needed).

Commonly persisted options:

- `--mqtt-session-store <path>` — persist sessions across restarts.
- `--mqtt-mapping-file <path.json>` — enable the embedded integrator.
- `--html-dir <path>` — point the Web UI to your HTML templates.

---

#### WebSockets specifics that often trip clients

- **Subprotocol:** ensure clients send `Sec-WebSocket-Protocol: mqtt`. Most libraries expose this as a “subprotocol” option.
- **Request target:** use `/` or `/ws` (both are accepted by the WS and WSS instances).
- **Reverse proxies:** preserve `Upgrade`, `Connection`, and `Sec-WebSocket-Protocol` headers through the proxy.

---

#### Quick sanity checks with common tools

``````bash
## Plain MQTT over TCP
mosquitto_sub -h 127.0.0.1 -p 1883 -t '#' -v
mosquitto_pub -h 127.0.0.1 -p 1883 -t 'test/topic' -m 'hello'

## MQTT over WebSockets (HTTP)
mosquitto_sub -h 127.0.0.1 -p 8080 -t '#' -v --protocol websockets
``````



#### Quick sanity checks with MQTTSuites command line tool

```bash
## Subscribe (plain MQTT)
mqttcli in-mqtt \
            remote --host 127.0.0.1 \
                   --port 1883 \
            sub --topic '#'

## Publish (plain MQTT)
mqttcli in-mqtt \
            remote --host 127.0.0.1 \
                   --port 1883 \
            pub --topic 'test/topic' \
                --message 'hello'

## MQTT over WebSockets (HTTP)
mqttcli in-wsmqtt \
            remote --host 127.0.0.1 \
                   --port 8080 \
            pub --topic 'test/topic' \
                --message 'hello'

## MQTT over Unix Domain Sockets
mqttcli un-mqtt \
            remote --sun-path /tmp/mqttbroker-un-mqtt \
            pub --topic 'test/topic' \
                --message 'hello'

## Subscribe and publish at once
mqttcli in-mqtt \
            remote --host 127.0.0.1 \
                   --port 1883 \
            sub --topic '#' \
            pub --topic 'test/topic' \
                --message 'hello'
```

---

#### Notes for UNIX domain sockets

- Use the UNIX sockets (`/tmp/mqttbroker-un-mqtt*`) for same-host services; they offer low overhead and simple isolation.
- Place sockets under `/run` or `/var/run` for service-managed lifecycles.
- Ensure appropriate **ownership/group** (e.g., a dedicated `mqttsuite` group) and **permissions**, so clients can connect without elevating privileges.

---

#### Embedded MQTTIntegrator (when `--mqtt-mapping-file` is provided)

- The broker can embed the **MQTTIntegrator** to transform/normalize payloads and remap topics without touching devices.
- Typical uses:
  - Convert vendor-specific JSON into a normalized schema.
  - Scale/rename fields (e.g., `temp` → `temperature`, multiply by 0.1).
  - Route subsets of topics to dashboards or database consumers.

> Keep your mapping JSON in version control and validate changes in staging before production.

---

#### Extending transports

- Thanks to SNode.C’s layered network design, adding transports (e.g., Bluetooth RFCOMM/L2CAP) follows the same **instance** pattern as TCP/IPv6/UNIX.
- Define endpoint parameters (address/port, channel, or path), optionally layer TLS (where applicable), and expose configuration via the broker’s CLI/config.

---

#### Diagnostics, hardening & tips

**Diagnostics**

- If a listener fails to start, check for “port in use” conflicts or permission issues on UNIX sockets.
- For WebSockets upgrade failures, verify the subprotocol and target path.

**Hardening**

- Bind to specific interfaces as needed (e.g., `--listen 127.0.0.1` for local-only).
- Use TLS for any untrusted network segments.
- Restrict access to private keys and UNIX sockets (ownership/mode).
- Apply firewall rules to limit exposure to necessary ports only.

**Common pitfalls**

- **TLS hostname mismatch:** client connects via a hostname not covered by CN/SAN.
- **Missing subprotocol on WS:** negotiation fails without `mqtt` subprotocol.
- **Permission denied on UNIX sockets:** fix group membership and file mode.

---

### Protocol version note

- The broker implements **MQTT 3.1.1**. Configure client libraries accordingly unless you intentionally diverge.

---

### Why this layout works well in production

- **Uniform instances:** Every listener (TCPv4/v6, UNIX, WS) is an *instance* with consistent options (listen address/port or path, TLS, limits). This keeps deployments predictable.
- **Predictable defaults:** Conventional ports (`1883/8883`, `8080/8088`) and request targets (`/` or `/ws`) make client configuration straightforward.
- **Persist-once workflow:** Use `-w` once, then keep day-to-day ops minimal and repeatable.

---

## MQTT Mapping Description

As optional for the MQTTBroker a MQTT mapping description is mandatory for the MQTTIntegrator.

### Purpose

An IoT app’s logic in **MQTTSuite** is expressed as a **JSON mapping file**.  
This file tells the **MQTTIntegrator** or **MQTTBroker** how to:

1) subscribe to incoming MQTT topics,  
2) translate (remap) the topics and payloads, and  
3) publish the translated messages back to the same broker connection.

A complete example lives in the [Canonical Mapping & Test Case](##canonical-mapping-&-test-case) section of the [MQTTIntegrator](#MQTTIntegrator) description.
The file must validate against the schema at [`lib/mapping-schema.json`](https://github.com/SNodeC/mqttsuite/blob/master/lib/mapping-schema.json).

---

### Top-Level Structure

A mapping description has three top-level objects:

- `discover_prefix` — prefix for discovery topics (**currently not used**).
- `connection` — MQTT client options used by the integrator.
- `mapping` — the actual topic tree and translation rules.

```json
{
  "discover_prefix": "snode.c",
  "connection": {
    "keep_alive": 60,
    "client_id": "Client",
    "clean_session": true,
    "will_topic": "will/topic",
    "will_message": "Last Will",
    "will_qos": 0,
    "will_retain": false,
    "username": "Username",
    "password": "Password"
  },
  "mapping": {
    /* see sections below */
  }
}
```

#### `connection` options (summary)

| Field           | Type    | Default | Notes                                     |
| --------------- | ------- | ------- | ----------------------------------------- |
| `keep_alive`    | integer | `60`    | Seconds between PINGREQs.                 |
| `client_id`     | string  | `""`    | Logical client name.                      |
| `clean_session` | boolean | `true`  | Start clean or resume a retained session. |
| `will_topic`    | string  | `""`    | Last Will topic.                          |
| `will_message`  | string  | `""`    | Last Will payload.                        |
| `will_qos`      | integer | `0`     | `0…2`.                                    |
| `will_retain`   | boolean | `false` | Retain Last Will.                         |
| `username`      | string  | `""`    | Username (optional).                      |
| `password`      | string  | `""`    | Password (optional).                      |

> The integrator uses these as **client** settings to connect to the broker it will subscribe to and republish on.

---

#### The `mapping` Object (big picture)

`mapping` defines a **topic hierarchy** and **subscriptions** with translation rules.

- A **topic level** is:
  - a single object describing one topic, **or**
  - an **array** of such objects describing **sibling** topics.
- Topic levels can be **recursively nested** to model trees.
- `topic_level.name` may be a **literal** (no wildcards) **or a single-character MQTT wildcard**:  
  - `+` → single-level wildcard  
  - `#` → multi-level wildcard (typically used as a **leaf**)

##### Minimal shapes

**Single topic level**

```json
"mapping": {
  "topic_level": {
    "name": "topic_level_name"
  }
}
```

**Nested topic levels**

```json
"mapping": {
  "topic_level": {
    "name": "parent",
    "topic_level": {
      "name": "child"
    }
  }
}
```

**Siblings with a nested child**

```json
"mapping": {
  "topic_level": [
    { "name": "sibling_1" },
    {
      "name": "sibling_2",
      "topic_level": { "name": "child_of_sibling_2" }
    }
  ]
}
```

##### Wildcard examples

**Single-level wildcard (`+`)**: subscribe to `home/+/temperature`

```json
"mapping": {
  "topic_level": {
    "name": "home",
    "topic_level": [
      {
        "name": "+",
        "topic_level": {
          "name": "temperature"
        }
      }
    ]
  }
}
```

**Multi-level wildcard (`#`)**: subscribe to everything below `sensors/#`

```json
"mapping": {
  "topic_level": {
    "name": "sensors",
    "topic_level": {
      "name": "#"
    }
  }
}
```

> The integrator performs correct wildcard matching when subscribing and dispatching to the defined `subscription`.

**A more complex hierarchy**

![A complex topic_level structure](/home/voc/projects/mqttsuite/mqttsuite/docs/images/mqtt-topics.png)

```json
"mapping": {
  "topic_level": {
    "name": "Home",
    "topic_level": [
      {
        "name": "Bedroom",
        "topic_level": [
          { "name": "..." },
          { "name": "..." }
        ]
      },
      { "name": "Garage" },
      {
        "name": "Kitchen",
        "topic_level": [
          {
            "name": "Fridge",
            "topic_level": [
              { "name": "Temperature" },
              { "name": "IceLevel" }
            ]
          },
          { "name": "Coffeemaker" }
        ]
      }
    ]
  }
}
```

---

### Subscriptions & Translation Rules

A `topic_level` can contain a `subscription` object.  
This instructs the integrator to **subscribe** to that concrete topic (using the topic path implied by the nested `name`s) and **map** incoming messages to new topics/payloads.

```json
"topic_level": {
  "name": "device",
  "topic_level": {
    "name": "sensor",
    "subscription": {
      "type": "binary_sensor",
      "qos": 2,
      /* one of: static / value / json (or an array of them) */
    }
  }
}
```

- `subscription.type` — freeform label (useful for UIs or discovery).
- `subscription.qos` — QoS used when the integrator subscribes at the broker (`0…2`).  
  *Publishing QoS is set per mapping via its own `qos` field (see below).*
- Exactly one **mapping section** is required (or an **array** of them):
  - `static` — string-match mapping (exact incoming payload → mapped payload).
  - `value` — template mapping where `message` is a scalar value.
  - `json` — template mapping where `message` is a JSON object.

> Each mapping section also accepts **arrays** (`[ … ]`) to apply multiple mappings for the same subscription.

---

### Mapping Sections

All mapping sections share **commons**:

- `mapped_topic` *(string, required)* — destination topic.  
  May be a **template** (supports `{{ ... }}`, `{{## ...}} … {{/ ...}}`) or a plain topic string (no `+` / `#`).
- `retain` *(boolean, default `false`)* — set MQTT retain on publishes.
- `qos` *(integer `0…2`, default `0`)* — **PUBLISH QoS** for the mapped message.  
  *(Independent of `subscription.qos`.)*

**`static` mapping**

Exact string-to-string payload conversion.

```json
"static": {
  "mapped_topic": "other_device/some_actuator/set",
  "message_mapping": [
    { "message": "pressed",  "mapped_message": "on"  },
    { "message": "released", "mapped_message": "off" }
  ],
  "retain": false,
  "qos": 0
}
```

**`value` mapping (template, scalar)**

Treat the incoming payload as a **scalar** bound to the template variable `message`.

```json
"value": {
  "mapped_topic": "other_device/some_actuator/set",
  "mapping_template": "{% if message == \"pressed\" %}on{% else if message == \"released\" %}off{% endif %}",
  "retain": false,
  "qos": 0
}
```

**`json` mapping (template, object)**

Treat the incoming payload as a **JSON object**, available as `message` inside the template.

```json
"json": {
  "mapped_topic": "other_device/some_actuator/set",
  "mapping_template": "{{ message.time.start }} to {{ message.time.end + 1 }}pm",
  "retain": false,
  "qos": 0
}
```

**Example input**

```json
{ "time": { "start": 5, "end": 10 } }
```

**Rendered output** → `5 to 11pm`

**Template extras**

- For template mappings (`value` / `json`), an optional field is available:

  ```json
  "suppressions": []
  ```

  Use this list for implementation-specific template controls. If unused, keep it empty (`[]`).

---

### Optional: `plugins`

Inside `mapping`, you may provide an optional `plugins` array:

```json
"mapping": {
  "plugins": ["pluginA", "pluginB"],
  "topic_level": { /* … */ }
}
```

This is a list of plugin identifiers that the integrator may use to extend mapping behavior.  
(Behavior is implementation-specific; leave empty if not needed.)

---

### Quick Start (Recommended Flow)

#### Skeleton mapping file

   ```json
   {
     "discover_prefix": "",
     "connection": { 
       "keep_alive": 60,
       "clean_session": true
     },
     "mapping": { 
       "topic_level": {
         "name": "devices"
       }
     }
   }
   ```

#### Add a concrete subscription with a simple static mapping

```json
{
  "mapping": {
    "topic_level": {
      "name": "devices",
      "topic_level": {
        "name": "button",
        "subscription": {
          "type": "binary_sensor",
          "qos": 0,
          "static": {
            "mapped_topic": "actuators/light/set",
            "message_mapping": [
              { "message": "pressed",  "mapped_message": "on"  },
              { "message": "released", "mapped_message": "off" }
            ]
          }
        }
      }
    }
  }
}
```

#### Switch to a template mapping when you need logic

```json
"subscription": {
  "type": "thermo",
  "qos": 1,
  "json": {
    "mapped_topic": "sensors/room1/summary",
    "mapping_template": "T={{ message.temp }};H={{ message.hum }}"
  }
}
```

#### Validate against the schema (example with `ajv-cli`)

```bash
ajv validate -s lib/mapping-schema.json -d your-mapping.json
```

#### Run the integrator with your mapping

```bash
mqttintegrator --mqtt-mapping-file your-mapping.json
```

---

### Field-Tested Guidance

- **Use wildcards deliberately.** `+` is perfect for “same depth, many devices”; `#` is best at a **leaf** to catch everything below a prefix.
- **Start simple.** Verify flow with a `static` mapping; then move to `value`/`json` templates.
- **Arrays unlock fan-out.** `static` / `value` / `json` accept arrays—emit multiple outputs from one input.
- **Decide `retain` intentionally.** For derived state (e.g., “current mode”), `retain: true` is often useful; for events, prefer `false`.
- **QoS roles are separate.** `subscription.qos` = **subscribe QoS**; mapping `qos` = **publish QoS**.
- **Validate early.** Keep the schema in CI; reject invalid mappings before deployment.
- **Version control.** Store mapping files with your app; review changes and test on staging.

---

### Schema Highlights

- **Root**: `mapping` is **required**.
- **`mapping.topic_level`**: a single object **or** an array of them.  
  Each `topic_level` **requires** `name` and **either** a nested `topic_level` **or** a `subscription`.
- **`topic_level.name`**: may be a literal **or** a single-char wildcard `+` or `#` (per schema pattern).  
  `#` is typically used at a leaf.
- **Mapping sections**: require either or an array of `static`, `value`, or `json` (each may be an array).
- **`mapped_topic`**: non-empty; either a plain topic (no `+/#`) **or** a template expression.

---

### In one sentence

**Model the topic tree (literals and `+`/`#`) → choose subscription points (with subscribe QoS) → translate via `static` / `value` / `json` (with publish QoS) → validate with the schema → run the integrator.**

---

## MQTTIntegrator

### Overview & Configuration

The *MQTTIntegrator* is the lightweight transformation engine of the **MQTTSuite**. It subscribes to topics on an MQTT broker, **translates** topics/payloads using the **mapping description** (presented earlier), and **publishes the translated messages back to the same broker connection**. It is **not** a multi-broker bridge; there are no `src-*` / `dst-*` pairs.

Additional transports can be added in the source: [`mqttintegrator.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttintegrator/mqttintegrator.cpp).

#### Connection instances of default builds of *MQTTIntegrator*

- `in-mqtt`: MQTT over IPv4 (client to remote `1883`)
- `in-mqtts`: MQTTS over IPv4 (client to remote `8883`)
- `in6-mqtt`: MQTT over IPv6 (client to remote `1883`)
- `in6-mqtts`: MQTTS over IPv6 (client to remote `8883`)
- `un-mqtt`: MQTT over Unix Domain Sockets (client to remote sun path)
- `un-mqtts`: MQTTS over Unix Domain Sockets (client to remote sun path)
- `in-wsmqtt`: MQTT over WebSockets via IPv4 (client to remote `8080`, target `/ws`)
- `in-wsmqtts`: MQTTS over WebSockets via IPv4 (client to remote `8088`, target `/ws`)
- `in6-wsmqtt`: MQTT over WebSockets via IPv6 (client to remote `8080`, target `/ws`)
- `in6-wsmqtts`: MQTTS over WebSockets via IPv6 (client to remote `8088`, target `/ws`)
- `un-wsmqtt`: MQTT over **Unix Domain WebSockets** (client to remote sun path, target `/ws`)
- `un-wsmqtts`: MQTTS over **Unix Domain WebSockets** (client to remote sun path, target `/ws`)

One can control which instances are built by enabling or disabling them through the available `cmake` options.

#### Configuration

All aspects—such as **remote** host/port, UNIX *sun* paths, WebSocket request targets, and per-connection options—are configurable via the [SNode.C configuration system](https://github.com/SNodeC/snode.c?tab=readme-ov-file#configuration). You can set options in code, on the command line, or via a configuration file.

If encrypted access to the broker is required, [suitable certificates need to be provided](https://github.com/SNodeC/snode.c?tab=readme-ov-file#ssltls-configuration-section-tls) for the **client** connection.

#### Notes

- **Persistent sessions:** Configure a *session store* if you want the client session to survive restarts:  
  `--mqtt-session-store <path-to-session-store-file>`.
- **Mapping file (required for translations):** Provide `--mqtt-mapping-file <path-to-mqtt-mapping-file.json>`.  
  The mapping syntax, wildcard support (`+`, `#`), **subscribe QoS** vs **publish QoS**, and templating are documented in the **MQTT Mapping Description** section placed before this one.
- **Active instances by default:** After installation, all connection instances are enabled. Disable unused ones explicitly with `--disabled` on those instances.
- **Persisting options:** Use `--write-config` or `-w` once to store current options in the configuration file.

---

### Quick Start (Recommended Flow)

Per default, all supported connection instances are **enabled**. Disable what you don’t use and keep a single, clear path to your broker.

#### Disable all but the `in-mqtt` instance

   ```bash
   mqttintegrator in-mqtts --disabled
                  in6-mqtt --disabled
                  in6-mqtts --disabled
                  un-mqtt --disabled
                  un-mqtts --disabled
                  in-wsmqtt --disabled
                  in-wsmqtts --disabled
                  in6-wsmqtt --disabled
                  in6-wsmqtts --disabled
                  un-wsmqtt --disabled
                  un-wsmqtts --disabled
   ```

#### Connect to a plain MQTT broker over TCP/IPv4

   ```bash
   mqttintegrator in-mqtt \
                      remote --host 127.0.0.1 \
                             --port 1883
   ```

#### Add a persistent session store (survives restarts)

   ```bash
   mqttintegrator --mqtt-session-store /var/lib/mqttsuite/mqttintegrator/session.store \
                  in-mqtt \
                      remote --host 127.0.0.1 \
                             --port 1883
   ```

#### Attach your mapping file (canonical test case below)

   ```bash
   mqttintegrator --mqtt-session-store /var/lib/mqttsuite/mqttintegrator/session.store \
                  --mqtt-mapping-file /etc/mqttsuite/mappings/mapping-example.json \
                  in-mqtt \
                      remote --host 127.0.0.1 \
                             --port 1883
   ```

#### Persist your configuration using `-w`

   ```bash
   mqttintegrator --mqtt-session-store /var/lib/mqttsuite/mqttintegrator/session.store \
                  --mqtt-mapping-file /etc/mqttsuite/mappings/mapping-example.json \
                  in-mqtt \
                      remote --host 127.0.0.1 \
                             --port 1883 \
                  -w
   ```

   From now on, a plain `mqttintegrator` starts with the stored configuration.

#### (Optional) Use TLS (MQTTS) instead of plain TCP

   ```bash
   mqttintegrator in-mqtts --disabled=false \
                      remote --host broker.example.org \
                             --port 8883 \
                      tls --cert /etc/ssl/mqttsuite/client.crt \
                          --cert-key /etc/ssl/mqttsuite/client.key \
                          --ca-cert /etc/ssl/mqttsuite/ca.crt
   ```

#### (Optional) Use WebSockets (WSS)

   ```bash
   mqttintegrator in-wsmqtts --disabled=false \
                      remote --host broker.example.org \
                             --port 8088 \
                      tls --ca-cert /etc/ssl/mqttsuite/ca.crt
   ```

#### (Optional) Use Unix Domain WebSockets

   ```bash
   mqttintegrator un-wsmqtt --disabled=false \
                      remote --sun-path /tmp/mqttbroker-un-http
   
   mqttintegrator un-wsmqtts --disabled=false \
                      remote --sun-path /tmp/mqttbroker-un-https3 \
                      tls --ca-cert /etc/ssl/mqttsuite/ca.crt
   ```

---

### Connection Methods

The *MQTTIntegrator* uses a **client** connection to a single broker. All tables show **remote** endpoints (not local listeners).  
Disable unused instances with `--disabled`.

#### MQTT over TCP/IP

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Protocol | Encryption | Remote Port |
| ------------------------------------------------------------ | -------- | ---------- | ----------- |
| **`in-mqtt`**                                                | IPv4     | No         | `1883`      |
| **`in-mqtts`**                                               | IPv4     | Yes        | `8883`      |
| **`in6-mqtt`**                                               | IPv6     | No         | `1883`      |
| **`in6-mqtts`**                                              | IPv6     | Yes        | `8883`      |

#### MQTT over UNIX Domain Sockets

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Encryption | Remote Sun Path            |
| ------------------------------------------------------------ | ---------- | -------------------------- |
| **`un-mqtt`**                                                | No         | `/tmp/mqttbroker-un-mqtt`  |
| **`un-mqtts`**                                               | Yes        | `/tmp/mqttbroker-un-mqtts` |

#### MQTT over WebSockets

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Protocol | Encryption | Remote Port | Request Target | Sec-WebSocket-Protocol |
| ------------------------------------------------------------ | -------- | ---------- | ----------- | -------------- | ---------------------- |
| **`in-wsmqtt`**                                              | IPv4     | No         | `8080`      | `/ws`          | `mqtt`                 |
| **`in-wsmqtts`**                                             | IPv4     | Yes        | `8088`      | `/ws`          | `mqtt`                 |
| **`in6-wsmqtt`**                                             | IPv6     | No         | `8080`      | `/ws`          | `mqtt`                 |
| **`in6-wsmqtts`**                                            | IPv6     | Yes        | `8088`      | `/ws`          | `mqtt`                 |

#### MQTT over UNIX Domain WebSockets

| [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) | Encryption | Remote Sun Path            | Request Target | Sec-WebSocket-Protocol |
| ------------------------------------------------------------ | ---------- | -------------------------- | -------------- | ---------------------- |
| **`un-wsmqtt`**                                              | No         | `/tmp/mqttbroker-un-http`  | `/ws`          | `mqtt`                 |
| **`un-wsmqtts`**                                             | Yes        | `/tmp/mqttbroker-un-https` | `/ws`          | `mqtt`                 |

#### Notes

- For all WS/WSS/UDS-WS instances, the integrator uses `Sec-WebSocket-Protocol: mqtt` and the request target `/ws`.
- All instances above are enabled by default after installation; deactivate any unneeded instance with `--disabled`.

---

### Canonical Mapping & Test Case

This minimal, **schema-valid** mapping subscribes to `kitchen/thermostat/1` and publishes to **`kitchen/heating/1/set`**:

- `"on"` if `temp < 21.0`
- `"off"` if `temp > 23.0`
- **no publish** in the 21.0–23.0 deadband

**Save as `~/mapping-example.json`:**

```json
{
  "discover_prefix": "snode.c",
  "connection": {
    "keep_alive": 60,
    "client_id": "Client",
    "clean_session": true,
    "will_topic": "will/topic",
    "will_message": "Last Will",
    "will_qos": 0,
    "will_retain": false,
    "username": "Username",
    "password": "Password"
  },
  "mapping": {
    "topic_level": {
      "name": "kitchen",
      "topic_level": {
        "name": "thermostat",
        "topic_level": {
          "name": "1",
          "subscription": {
            "type": "thermostat",
            "qos": 0,
            "json": {
              "mapped_topic": "kitchen/heating/1/set",
              "mapping_template": "{% if message.temp < 21.0 %}on{% else if message.temp > 23.0 %}off{% endif %}",
              "retain": false,
              "qos": 0,
              "suppressions": [""]
            }
          }
        }
      }
    }
  }
}
```

**Run the integrator for this mapping (in-mqtt):**

```bash
mqttintegrator --mqtt-session-store ~/session.store \
               --mqtt-mapping-file ~/mapping-example.json \
               in-mqtt \
                   remote --host 127.0.0.1 \
                          --port 1883
```

**Persist once:**

```bash
mqttintegrator --mqtt-session-store ~/session.store \
               --mqtt-mapping-file ~/mapping-example.json \
               in-mqtt
                   remote --host 127.0.0.1
                          --port 1883 \
               -w
```

**Then just run**

```bash
mqttintegrator
```

**Verification**

```bash
## Observe mapped results
mqttcli in-mqtt
            remote --host 127.0.0.1 \
                   --port 1883 \
            sub --topic 'kitchen/heating/1/set'
            

## 20.9 → "on"
mqttcli in-mqtt
            remote --host 127.0.0.1 \
                   --port 1883 \
            pub --topic 'kitchen/thermostat/1'
                --message '{"temp":20.9}'
                
## 23.5 → "off"mqttcli in-mqtt
            remote --host 127.0.0.1 \
                   --port 1883 \
            pub --topic 'kitchen/thermostat/1'
                --message '{"temp":23.5}'
                
## 22.7 → deadband → no publish
mqttcli in-mqtt
            remote --host 127.0.0.1 \
                   --port 1883 \
            pub --topic 'kitchen/thermostat/1'
                --message '{"temp":22.7}'
```

**QoS roles:** `subscription.qos` controls the **subscribe** QoS. The `json.qos` inside the mapping controls the **publish** QoS for the mapped message.

---

### Behavior aligned with the Mapping Description

- The **mapping file** defines where the integrator places **subscriptions** in the topic tree (including support for `+` and `#` in `topic_level.name`). The integrator performs correct wildcard matching when subscribing and dispatching.
- `subscription.qos` specifies the QoS used to subscribe at the broker; it does not control publish QoS.
- Each mapping rule (in `static`, `value`, or `json`) has its own publish `qos` and `retain` flags and a `mapped_topic` (string or template).
- Arrays are allowed (`static` / `value` / `json`), enabling **fan-out** (multiple publishes per inbound message).
- The integrator **republishes** transformed messages **back to the same broker connection** from which they were received.

---

### Additions & Field-Tested Guidance

#### TLS checklist for client connections (MQTTS / WSS / UDS-WSS)

- Provide **client certificate & key** if your broker enforces mutual TLS; otherwise a CA bundle is often sufficient.
- Ensure the broker hostname you connect to is covered by the server certificate’s **CN/SAN** (for TCP/WSS).
- Lock down permissions on key files (e.g., `chmod 600`, owned by the service user).
- Configure TLS under the chosen instance’s `tls` subsection.

##### Example (TCP/TLS) with mutual TLS:

```bash
mqttintegrator in-mqtts \
                  remote --host broker.example.org \
                         --port 8883 \
                  tls --ca-cert /etc/ssl/mqttsuite/ca.crt \
                      --cert /etc/ssl/mqttsuite/cert.crt \
                      --cert-key /etc/ssl/mqttsuite/cert.key \
                      --cert-key-password 'some-password'
```

##### Example (WebSockets/TLS):

```bash
mqttintegrator in-wsmqtts \
                  remote --host broker.example.org \
                         --port 8088 \
                  tls --ca-cert /etc/ssl/mqttsuite/ca.crt
```

##### Example (Unix Domain WebSockets, TLS layered on the WS endpoint):

```bash
mqttintegrator un-wsmqtts \
                  remote --sun-path /tmp/mqttbroker-un-https
```

---

#### Persist-once workflow (recommended)

- Start with explicit CLI flags; verify connectivity and message flow.
- Run **once** with `-w` (or `--write-config`) to save the configuration.
- Thereafter, use `mqttintegrator` without flags; override only deltas when needed.

Commonly persisted options:

- `--mqtt-session-store <path>` — keep client session state across restarts.
- `--mqtt-mapping-file <path.json>` — apply your transformation/normalization rules.

---

#### WebSockets specifics that often trip clients

- **Subprotocol:** the integrator sets `Sec-WebSocket-Protocol: mqtt`. Ensure the broker accepts it.
- **Request target:** use `/ws` (do not change unless your broker explicitly uses a different target).
- **Through proxies:** `Upgrade`, `Connection`, and `Sec-WebSocket-Protocol` must pass through unchanged.
- **UDS-WS:** ensure the **sun path** matches the broker’s WS UDS listener (e.g., `/tmp/mqttbroker-un-http` or `/tmp/mqttbroker-un-https`) and that file permissions allow access.

---

#### Diagnostics, hardening & common pitfalls

**Diagnostics**

- **Cannot connect:** verify `remote --host/--port` (or `--sun-path`) values and broker reachability.
- **TLS failures:** hostname mismatch, missing CA, or wrong client certificates.
- **WS upgrade errors:** wrong request target (`/ws`) or missing `mqtt` subprotocol.

**Hardening**

- Prefer UNIX domain sockets on the same host for reduced attack surface.
- Use TLS (and client auth where required) across untrusted networks.
- Restrict file permissions for keys and session store.

**Common pitfalls**

- **All instances active:** disable unused instances to avoid accidental connections.
- **Wrong broker path:** mismatched WS target or socket path leads to silent failures.
- **Mapping file path typos:** the integrator starts but no transforms occur—double-check the filename.

---

### Protocol version note

- The integrator speaks **MQTT 3.1.1** as a client. Configure your broker accordingly.

---

### Why this layout works well in production

- **Single-broker focus:** clear operational model—subscribe, transform, and republish on the same connection.
- **Uniform instance model:** TCPv4/v6, UNIX sockets, Unix Domain WebSockets, and IP WebSockets share a consistent configuration surface.
- **Persist-once simplicity:** Use `-w` once; day-to-day runs are minimal and repeatable.

---

## MQTTBridge

### Overview & Configuration

The **MQTTBridge** is a **pure client-side bridge** in the **MQTTSuite**. It creates one or more **logical bridges**, where each bridge connects to **two or more MQTT brokers** and **relays topics** among them. Forwarding is governed by a single JSON file, **`bridge-config.json`**. The bridge does **not** expose listeners; every connection is an **outbound client** connection to a remote broker.

Like the other MQTTSuite apps, the bridge supports the same transport families (TCP/IPv4, TCP/IPv6, UNIX domain sockets, and WebSockets over these), plus optional Bluetooth (RFCOMM/L2CAP) if built. The `network` section of the JSON selects transport and security **per broker**.

**Key properties**

- **Multi-broker fan-out:** Messages received from one broker are forwarded to all the other brokers in the same logical bridge, subject to **topic filters**, **prefix rules**, and **loop prevention**.
- **Prefixes:** Optional **bridge-level** and **broker-level** prefixes help avoid collisions and enable simple routing.
- **Loop prevention:** Optional per-broker flag to reduce message echo/loops.
- **QoS per subscription:** Each broker’s subscription list sets the **SUBSCRIBE QoS** used toward that broker.
- **Session stores:** Optional per-broker on-disk session stores to resume state across restarts.
- **Protocol:** MQTT **3.1.1** client behavior.

---

### Quick Start (Recommended Flow)

#### Create a minimal `bridge-config.json`

Two brokers, bidirectional, loop prevention on:

```json
{
  "bridges": [
    {
      "name": "local↔edge",
      "prefix": "bridge/",
      "brokers": [
        {
          "network": {
            "instance_name": "local-ws",
            "protocol": "in",
            "in": { "host": "127.0.0.1", "port": 8080 },
            "encryption": "legacy",
            "transport": "websocket"
          },
          "prefix": "local/",
          "mqtt": {
            "client_id": "bridge-local",
            "clean_session": true,
            "loop_prevention": true
          },
          "topics": [
            { "topic": "##", "qos": 0 }
          ]
        },
        {
          "network": {
            "instance_name": "edge-tcp",
            "protocol": "in",
            "in": { "host": "edge.example.net", "port": 1883 },
            "encryption": "legacy",
            "transport": "stream"
          },
          "prefix": "edge/",
          "mqtt": {
            "client_id": "bridge-edge",
            "clean_session": true,
            "loop_prevention": true
          },
          "topics": [
            { "topic": "##", "qos": 0 }
          ]
        }
      ]
    }
  ]
}
```

#### Run the bridge:

```bash
mqttbridge --bridge-config-file /etc/mqttsuite/bridge-config.json
```

#### Verify (example):

```bash
## Publish on 'local' broker → should appear on 'edge' with prefixing
mqttcli in-wsmqtt remote --host 127.0.0.1 --port 8080 \
  pub --topic 'sensors/temp' --message '21.5'

## Subscribe on edge:
mosquitto_sub -h edge.example.net -p 1883 -t 'bridge/local/##' -v
```

> **Persist-once workflow:** Like other MQTTSuite apps, you can persist CLI runtime options with `-w`. The **bridge topology** itself lives in `bridge-config.json`, so you typically run `mqttbridge --bridge-config-file …` thereafter.

---

### Bridge Configuration JSON

This section shows the **overall file structure** first, then elaborates on **each element**. Unknown keys are **rejected** (`additionalProperties: false` throughout the schema).

#### Overall Structure (at a glance)

```jsonc
{
  "bridges": [                       // required, ≥ 1 items
    {
      "disabled": false,             // optional
      "name": "bridge-name",         // required (string)
      "prefix": "bridge/",           // optional (string, default "")
      "brokers": [                   // required, ≥ 1 items
        {
          "disabled": false,         // optional
          "session_store": "/var/lib/mqttsuite/mqttbridge/session-1.store", // optional
          "network": {               // required
            "instance_name": "local-ws",
            "protocol": "in",        // one of: in | in6 | un | rc | l2
            "encryption": "legacy",  // one of: legacy | tls
            "transport": "websocket",// one of: stream | websocket

            // exactly one of: in | in6 | un | rc | l2, matching "protocol"
            "in":  { "host": "127.0.0.1", "port": 8080 }
            // "in6": { "host": "::1", "port": 8080 }
            // "un":  { "path": "/run/mqttbroker.unix" }
            // "rc":  { "host":"AA:BB:CC:DD:EE:FF","channel":1 }
            // "l2":  { "host":"AA:BB:CC:DD:EE:FF","psm":25 }
          },

          "mqtt": {                  // optional; defaults shown
            "client_id": "",
            "keep_alive": 60,
            "clean_session": true,
            "will_topic": "",
            "will_message": "",
            "will_qos": 0,           // 0..2
            "will_retain": false,
            "username": "",
            "password": "",
            "loop_prevention": false
          },

          "prefix": "local/",        // optional (string, default "")
          "topics": [                // optional (≥ 1), default: [{ "topic":"##", "qos":0 }]
            { "topic": "##", "qos": 0 }
          ]
        }

        // ... more brokers in this bridge
      ]
    }

    // ... more bridges
  ]
}
```

**Forwarding logic (mental model):**  
For each bridge, every broker subscribes to its configured `topics`. Incoming messages that match a broker’s subscriptions are **forwarded to all the other brokers** in the same bridge. When forwarding, the bridge applies (if set):

```
<bridge.prefix> + <source-broker.prefix> + <original-topic>
```

Looping is mitigated with `mqtt.loop_prevention: true` (per broker) plus sensible prefixing.

---

#### Elements in Detail

##### Root Object

| Field     | Type  | Required | Default | Notes                                                        |
| --------- | ----- | -------: | ------: | ------------------------------------------------------------ |
| `bridges` | array |        ✅ |       — | Must contain **≥ 1** `bridge` objects. Unknown keys rejected. |

##### `bridge` (each item in `bridges`)

| Field      | Type    | Required | Default | Notes                                                       |
| ---------- | ------- | -------: | ------: | ----------------------------------------------------------- |
| `disabled` | boolean |        ✖ | `false` | If `true`, this whole bridge is ignored.                    |
| `name`     | string  |        ✅ |       — | Logical name for logs/metrics.                              |
| `prefix`   | string  |        ✖ |    `""` | Optional **bridge-wide** prefix added before forwarding.    |
| `brokers`  | array   |        ✅ |       — | **≥ 1** broker definitions that participate in this bridge. |

##### `broker` (each item in `brokers`)

| Field           | Type    | Required |                       Default | Notes                                                        |
| --------------- | ------- | -------: | ----------------------------: | ------------------------------------------------------------ |
| `disabled`      | boolean |        ✖ |                       `false` | If `true`, this broker does not connect/forward.             |
| `session_store` | string  |        ✖ |                          `""` | Optional path to persist client session (useful with `clean_session: false`). |
| `network`       | object  |        ✅ |                             — | **Transport selection** and concrete address (see **`network`** below). |
| `mqtt`          | object  |        ✖ |                      defaults | MQTT client opts (credentials, last will, **loop prevention**). |
| `prefix`        | string  |        ✖ |                          `""` | Optional **source-broker** prefix added when forwarding **from this broker** to others. |
| `topics`        | array   |        ✖ | `[{ "topic":"##", "qos":0 }]` | Subscription filters **towards this broker** (SUBSCRIBE QoS per item). Must be ≥1 if present. |

> **Subscribe QoS** is taken from `topics[].qos` toward the *source* broker.  
> **Publish QoS** used when forwarding is chosen by the bridge’s policy; align deployment for consistency (QoS 0/1 are common).

##### `mqtt` (client options)

| Field             | Type    | Required | Default | Constraints / Notes                                        |
| ----------------- | ------- | -------: | ------: | ---------------------------------------------------------- |
| `client_id`       | string  |        ✖ |    `""` | Let the broker assign if empty, or set explicitly.         |
| `keep_alive`      | integer |        ✖ |    `60` | Seconds between pings.                                     |
| `clean_session`   | boolean |        ✖ |  `true` | Set `false` to resume session (pair with `session_store`). |
| `will_topic`      | string  |        ✖ |    `""` | Optional Last Will topic.                                  |
| `will_message`    | string  |        ✖ |    `""` | Optional Last Will payload.                                |
| `will_qos`        | integer |        ✖ |     `0` | `0..2`.                                                    |
| `will_retain`     | boolean |        ✖ | `false` | —                                                          |
| `username`        | string  |        ✖ |    `""` | Broker auth (if required).                                 |
| `password`        | string  |        ✖ |    `""` | Broker auth (if required).                                 |
| `loop_prevention` | boolean |        ✖ | `false` | Enable to mitigate echo loops. Combine with origin prefix. |

##### `topics` (subscriptions for a broker)

Each item is a `topic` object:

| Field   | Type    | Required | Default | Constraints / Notes                   |
| ------- | ------- | -------: | ------: | ------------------------------------- |
| `topic` | string  |        ✅ |  `"##"` | MQTT filter (`##`, `+` allowed).      |
| `qos`   | integer |        ✖ |     `0` | One of `0`, `1`, `2` (SUBSCRIBE QoS). |

> These control which inbound topics are **accepted from this broker**. Matching messages are forwarded to other brokers.

##### `network` (transport selection + addressing)

| Field           | Type   |             Required | Values / Example                               | Notes                              |
| --------------- | ------ | -------------------: | ---------------------------------------------- | ---------------------------------- |
| `instance_name` | string |                    ✅ | `"local-ws"`                                   | Logical name; shows in logs.       |
| `protocol`      | string |                    ✅ | `in` \| `in6` \| `un` \| `rc` \| `l2`          | Selects address object below.      |
| `encryption`    | string |                    ✅ | `legacy` \| `tls`                              | `tls` for TLS (MQTTS/WSS).         |
| `transport`     | string |                    ✅ | `stream` \| `websocket`                        | MQTT over TCP vs WebSockets.       |
| `in`            | object |  *if `protocol: in`* | `{ "host": "127.0.0.1", "port": 1883 }`        | IPv4 host/port.                    |
| `in6`           | object | *if `protocol: in6`* | `{ "host": "::1", "port": 1883 }`              | IPv6 host/port.                    |
| `un`            | object |  *if `protocol: un`* | `{ "path": "/run/mqtt.sock" }`                 | Unix domain path (max ~107 chars). |
| `rc`            | object |  *if `protocol: rc`* | `{ "host":"AA:BB:CC:DD:EE:FF", "channel": 1 }` | Bluetooth RFCOMM.                  |
| `l2`            | object |  *if `protocol: l2`* | `{ "host":"AA:BB:CC:DD:EE:FF", "psm": 25 }`    | Bluetooth L2CAP.                   |

Exactly **one** address object (`in`/`in6`/`un`/`rc`/`l2`) must be present, consistent with `protocol`.

**WebSockets specifics:** For WebSockets, the suite uses request target **`/ws`** and subprotocol **`mqtt`**. No extra fields are needed in the JSON.

**Address constraints (from schema):**

- `in`, `in6`: `port` ∈ `[1,65535]`, `host` is a string (IPv4/IPv6 or hostname).
- `un`: POSIX path, `^/(?:[^\0])+$`, `maxLength: 107`.
- `rc`: Bluetooth MAC (`^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$`), `channel` ∈ `[1,30]`.
- `l2`: Bluetooth MAC as above, `psm` ∈ `[1,65535]`.

---

### Best Practices & Validation Tips

- **Validation:** Stick to the schema—unknown keys fail validation (`additionalProperties: false`).
- **Secrets:** Keep credentials out of VCS; generate the final JSON at deploy time or mount via secrets.
- **Prefixes:** Use a **bridge prefix** (e.g., `bridge/`) and **origin prefixes** (e.g., `local/`, `ttn/`) to keep topics intelligible and enable filtering.
- **Loop prevention:** Enable `mqtt.loop_prevention: true` wherever bidirectional paths overlap. Combine with origin prefixing so the bridge recognizes its own forwarded traffic.
- **Subscriptions & QoS:** Start broad (`"##"`, QoS 0); once data flows, narrow filters. Keep end-to-end QoS consistent.
- **Sessions & LWT:** For unstable links, define a `session_store` and consider `clean_session: false`. Configure Last Will if operators rely on presence/health topics.
- **UNIX domain sockets:** Place sockets under `/run` or `/var/run`; set ownership/permissions for the bridge process.
- **WebSockets:** Target **`/ws`** and ensure the broker accepts subprotocol **`mqtt`**.
- **Diagnostics:**  
  - **No forwarding:** Check the *source broker’s* `topics[]` filter and verify the *other* brokers subscribe to the forwarded, prefixed paths.  
  - **Echo storms:** Add prefixes and enable loop prevention on both sides; avoid subscribing to your own bridged prefix everywhere.  
  - **WS handshake failures:** Wrong target or missing subprotocol.

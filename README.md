

# MQTTSuite: A lightweight MQTT Integration System

The [*MQTTSuite*](https://snodec.github.io/mqttsuite-doc/html/index.html) project consist of three applications *MQTTBroker*, *MQTTIntegrator* and *MQTTBridge* powered by *[SNode.C](https://github.com/VolkerChristian/snode.c)*, a si*mapping description file*ngle threaded, single tasking framework for networking applications written entirely in C++. Due to it's little resource usage it is especially usable on resource limited systems.

- **MQTTBroker**: Is a full featured MQTT broker utilizing version [3.1.1](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) of the MQTT protocol standard. It accepts encrypted and unencrypted incoming plain and WebSockets MQTT connections.
  In addition it provides a rudimentary Web-Interface showing all currently connected MQTT-clients.
- **MQTTIntegrator**: Is a IoT integration application controlled by a JSON-mapping description. It connects via SSL/TLS to an MQTT-broker.
- **MQTTBridge**: Is a purely client side bridge. It can establish multiple bridges each connecting to multiple mqtt brokers and bridges configurable topics among them.

# Table of Content
<!--ts-->
* [MQTTSuite: A lightweight MQTT Integration System](#mqttsuite-a-lightweight-mqtt-integration-system)
* [Table of Content](#table-of-content)
* [License](#license)
* [Copyright](#copyright)
* [Installation](#installation)
   * [Supported Systems and Hardware](#supported-systems-and-hardware)
   * [Minimum required Compiler Versions](#minimum-required-compiler-versions)
   * [Requirements and Dependencies](#requirements-and-dependencies)
      * [Tools](#tools)
         * [Mandatory](#mandatory)
         * [Optional](#optional)
      * [Frameworks](#frameworks)
         * [Mandatory](#mandatory-1)
      * [Libraries](#libraries)
         * [Mandatory](#mandatory-2)
         * [Bundled](#bundled)
   * [Installation on Debian Style Systems (x86-64, Arm)](#installation-on-debian-style-systems-x86-64-arm)
      * [Requirements and Dependencies](#requirements-and-dependencies-1)
      * [MQTTSuite](#mqttsuite)
   * [Deploment on OpenWRT](#deploment-on-openwrt)
      * [Choose and Download a SDK](#choose-and-download-a-sdk)
      * [Patch the SDK to Integrate the MQTTSuite Feed](#patch-the-sdk-to-integrate-the-mqttsuite-feed)
      * [Install the MQTTSuite Package and its Dependencies](#install-the-mqttsuite-package-and-its-dependencies)
      * [Configure the SDK](#configure-the-sdk)
      * [Cross Compile MQTTSuite](#cross-compile-mqttsuite)
      * [Deploy MQTTSuite](#deploy-mqttsuite)
* [MQTTSuite Usage Guide](#mqttsuite-usage-guide)
   * [MQTTBroker](#mqttbroker)
      * [Connection Options](#connection-options)
         * [MQTT over TCP/IP](#mqtt-over-tcpip)
         * [MQTT over Unix Domain Sockets](#mqtt-over-unix-domain-sockets)
         * [MQTT over WebSockets](#mqtt-over-websockets)
      * [Web Interface](#web-interface)
   * [MQTTIntegrator](#mqttintegrator)
      * [Connection Options](#connection-options-1)
         * [MQTT over TCP/IP](#mqtt-over-tcpip-1)
         * [MQTT over Unix Domain Sockets](#mqtt-over-unix-domain-sockets-1)
         * [MQTT over WebSockets](#mqtt-over-websockets-1)
   * [MQTT Mapping Description](#mqtt-mapping-description)
      * [The Structure of the JSON Mapping Description](#the-structure-of-the-json-mapping-description)
      * [Topic Levels and the JSON-Structure of the mapping Section](#topic-levels-and-the-json-structure-of-the-mapping-section)
         * [Single Topic Level](#single-topic-level)
         * [Nested Topic Levels](#nested-topic-levels)
         * [Sibling and Nested Topic Levels](#sibling-and-nested-topic-levels)
         * [A More Complex Topic Level Structure](#a-more-complex-topic-level-structure)
         * [The subscription Object of a Topic Level](#the-subscription-object-of-a-topic-level)
            * [<strong>Static</strong> Mapping](#static-mapping)
            * [<strong>Value</strong> Mapping](#value-mapping)
            * [<strong>JSON</strong> Mapping](#json-mapping)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->
<!-- Added by: runner, at: Sat Mar  1 19:56:57 UTC 2025 -->

<!--te-->

# License

SNode.C  is dual-licensed under MIT and GPL-3.0 (or any later version).
You can choose between one of them if you use this work.

`SPDX-License-Identifier: MIT OR GPL-3.0-or-later`

# Copyright

Volker Christian ([me@vchrist.at](mailto:me@vchrist.at) or [Volker.Christian@fh-hagenberg.at](mailto:volker.christian@fh-hagenberg.at))

# Installation

The installation of the MQTTSuite is straight forward:

- In a first step the network framework SNode.C needs to be installed

- In a second step all neccessary tools and libraries are installed.

- Afterwards the MQTTSuite can be cloned, compiled and installed.


## Supported Systems and Hardware

The main development of MQTTSuite takes place on a *Debian Sid* style Linux system. Since *Debian Bookworm* it compiles also on *Debian stable*. Though, it should compile cleanly on every Linux system provided that all required tools and libraries are installed.

MQTTSuite is known to compile and run successful on

- **x86-64** architecture
- **Arm** architecture (32 and 64 bit) tested on
  - Raspberry Pi 3, 4 and 5
- **OpenWrt** 23.05.0 and later
  - All architectures
- **Android** via [Termux](https://termux.dev/en/)
  -   Documentation in preparation

## Minimum required Compiler Versions

The most version-critical dependencies are the C++ compilers.

Either *GCC* or *clang* can be used but they need to be of an up to date version because SNode.C uses some new C++20 features internally.

- GCC 12.2 and later
- Clang 13.0 and later

## Requirements and Dependencies

MQTTSuite requires some external tools and depends on a view external libraries. Some of these libraries are directly included in the MQTTSuite.

### Tools

#### Mandatory

- git (https://git-scm.com/)
- cmake (https://cmake.org/)
- make (https://www.gnu.org/software/make/) or
- ninja (https://ninja-build.org/)
- g++ (https://gcc.gnu.org/) or
- clang (https://clang.llvm.org/)
- pkg-config (https://www.freedesktop.org/wiki/Software/pkg-config/)

#### Optional

- iwyu (https://include-what-you-use.org/)
- clang-format (https://clang.llvm.org/docs/ClangFormat.html)
- cmake-format (https://cmake-format.readthedocs.io/)
- doxygen (https://www.doxygen.nl/)

### Frameworks

#### Mandatory

- SNode.C: Simple NODE in C++ ([https://github.com/SNodeC/snode.c](https://github.com/SNodeC/snode.c))

### Libraries

#### Mandatory

- libfmt (11.0.0 or later) development files ([https://github.com/fmtlib/fmt](https://github.com/fmtlib/fmt))

#### Bundled

This libraries are already integrated directly in the MQTTSuite. Thus they need not be installed by hand

- JSON Schema Validator for Modern C++ ([https://github.com/pboettch/json-schema-validator](https://github.com/pboettch/json-schema-validator))
- inja: A Template Engine for Modern C++ ([https://github.com/pantor/inja](https://github.com/pantor/inja))

## Installation on Debian Style Systems (x86-64, Arm)

### Requirements and Dependencies

To install all dependencies on Debian style system

- install SNode.C ([https://github.com/SNodeC/snode.c?tab=readme-ov-file#installation](https://github.com/SNodeC/snode.c?tab=readme-ov-file#installation))

- and than just run

  ```sh
  sudo apt update
  sudo apt install libfmt-dev
  ```


### MQTTSuite

After installing all dependencies MQTTSuite can be cloned from github, compiled and installed.

```sh
mkdir mqttsuite
cd mqttsuite
git clone https://github.com/SNodeC/mqttsuite.git
mkdir build
cd build
cmake ../mqttsuite
make
sudo make install
sudo ldconfig
```

It is a good idea to utilize all processor cores and threads for compilation. Thus e.g. on a Raspberry Pi append `-j5` to the `make` or `ninja` command.

## Deploment on OpenWRT

As a starting point, it is assumed that local *ssh* and *sftp* access to the router exist and that the router is connected to the Internet via the WAN port.

***Note:*** You *do not need* to deploy SNode.C by hand as this framework is *pulled in* by the following procedure *automatically*.

Deploying SNode.C on an OpenWRT router involves a view tasks:

1. Choose and download an OpenWRT-SDK
2. Patch the SDK to integrate the SNode.C feed
3. Install the MQTTSuite package and its dependencies
4. Configure the SDK
5. Cross compile MQTTSuite
6. Deploy MQTTSuite

MQTTSuite needs to be *cross compiled* on a Linux host system to be deployable on OpenWRT. Don't be afraid about cross compiling it is strait forward.

### Choose and Download a SDK

First, download and extract a SDK-package of version  23.05.0-rc1 or later from the [OpenWRT download page](https://downloads.openwrt.org/) into an arbitrary directory \<DIR\>.

For example to download and use the SDK version 23.05.0-rc1 for the Netgear MR8300 Wireless Router (soc: IPQ4019) run 

```sh
cd <DIR>
wget -qO - https://downloads.openwrt.org/releases/23.05.0-rc2/targets/ipq40xx/generic/openwrt-sdk-23.05.0-rc2-ipq40xx-generic_gcc-12.3.0_musl_eabi.Linux-x86_64.tar.xz | tar Jx
```

to create the SDK directory `openwrt-sdk-<version>-<architecture>-<atype>_<compiler>-<cverstion>_<libc>_<abi>.Linux-x86_64` what from now on is referred as \<SDK_DIR\>.

In this example the values of the placeholder are:

- \<version> = 23.05.0-rc2

- \<architecture\> = ipq40xx\_

- \<atype\> = generic

- \<compiler\> = gcc

- \<cverstion\> = 12.3.0

- \<libc\> = musl

- \<abi\> = eabi

### Patch the SDK to Integrate the MQTTSuite Feed

Second step is to patch the default OpenWRT package feeds to add the SNode.C feed (MQTTSuite is provided by this feed) by executing

```sh
cd <SDK_DIR>
echo "src-git snodec https://github.com/SNodeC/OpenWRT" >> feeds.conf.default
```

### Install the MQTTSuite Package and its Dependencies

In the third step, all source packages required to compile SNode.C are installed.

```sh
cd <SDK_DIR>
./scripts/feeds update base packages snodec # Only the feeds base, packages and snodec are necessary
./scripts/feeds install mqttsuite
```

### Configure the SDK

The SDK is configured in the fourth step.

```sh
cd <SDK_DIR>
make defconfig
```

Default values for all configuration options can be used safely. 

Nevertheless, to customize the configuration of the base framework SNode.C run

```sh
cd <SDK_DIR>
make menuconfig
```

and navigate to `Network -> SNode.C`.

### Cross Compile MQTTSuite

In the last step, MQTTSuite is cross-compiled.

```sh
cd <SDK_DIR>
make package/mqttsuite/compile
```

The two steps, *Install Packages*, and *Cross Compile* (at most the last one) take some time as 

1. all feed and package definitions necessary to cross compile MQTTSuite are downloaded and installed locally from the OpenWRT servers and from github,
2. the sources of all dependent and indirect dependent packages are downloaded from upstream and build recursively and
3. MQTTSuite is cloned from github and cross compiled.

To parallelize the compilation use the switch `-j<thread-count>`  of `make` or `ninja` where \<thread-count\> is the number of CPU-threads dedicated to cross compile SNode.C and its dependencies.

***Note:*** For MQTTBroker and all it's build dependencies the created *ipk-packages* can be found in the directory `<SDK_DIR>bin/packages/\<architecture\>`.

### Deploy MQTTSuite

After cross compilation, MQTTSuite can be deployed.

The `mqttsuite_<version>_<architecture>.ipk` and `snode.c-*_<version>_<architecture>.ipk` packages must be downloaded to and installed on the router by executing

```sh
cd <SDK_DIR>/bin/packages/<architecture>/snodec
sftp root@<router-ip>
cd /tmp
put snode.c*.ipk
put mqttsuite*.ipk
exit
ssh root@<router-ip>
cd /tmp
opkg [--force-reinstall] install snode.c*.ipk mqttsuite*.ipk
exit
```

on the router. Use the option `--force-reinstall` in cast you want to reinstall the same version by overwriting the currently installed files.

***Note:*** On first install of MQTTSuite and SNode.C you will for sure get errors about missing dependent packages from the primary OpenWRT package repository. Just install them as usual using opkg and than go on to install MQTTSuite and SNode.C again.

During package installation a new *unix group* with member *root* is created, used for the group management of config-, log-, and pid-files.

***Note:*** A logout/login is necessary to activate the new group assignment.

# MQTTSuite Usage Guide

## MQTTBroker

### Connection Options

The *MQTTBroker* supports multiple connection methods across various protocols. Additional connection protocols e.g. Bluetooth can be easily added in the [`mqttbroker.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttbroker/mqttbroker.cpp) source code.

All aspects like port numbers, sun paths etc. can be configured using the [SNode.C configuration system](https://github.com/SNodeC/snode.c?tab=readme-ov-file#configuration) either in-code, on the command line or via a configuration file.

In case an encrypted access is required [suitable certificates needs to be provided](https://github.com/SNodeC/snode.c?tab=readme-ov-file#ssltls-configuration-section-tls) also.

***Note:*** A *session store* needs to be configured in case the current sessions shall be made persistent in case the MQTTBroker is stopped by using the command line option `--mqtt-session-store <path-to-session-store-file>`.

***Note:*** In case the MQTTBroker should also act as an integrated MQTTIntegrator a *[mapping description file](#MQTT-Mapping-Description)* needs to be provided via the command line option `--mqtt-mapping-file <path-to-mqtt-mapping-file.json>`.

***Note:*** The path to the HTML templates for the MQTTBroker Web Interface can be configured by providing the command line option `--html-dir <dir-of-html-templates>`. The default directory `/var/www/mqttsuite/mqttbroker` is already configured in  [`mqttbroker.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttbroker/mqttbroker.cpp).

***Note:*** All three options can be made *persistent* by storing their values in a *configuration file* by appending the switch `--write-config` or `-w` on the command line.

#### MQTT over TCP/IP

MQTT clients can connect via *IPv4* and *IPv6* using both encrypted and unencrypted channels.

| Protocol | Encryption | Local Port | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| -------- | ---------- | ---------- | ------------------------------------------------------------ |
| **IPv4** | No         | `1883`     | `in-mqtt`                                                    |
| **IPv4** | Yes        | `8883`     | `in-mqtts`                                                   |
| **IPv6** | No         | `1883`     | `in6-mqtt`                                                   |
| **IPv6** | Yes        | `8883`     | `in6-mqtts`                                                  |

#### MQTT over Unix Domain Sockets

For local communication, the broker also supports Unix domain sockets.

| Encryption | Local Sun Path             | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| ---------- | -------------------------- | ------------------------------------------------------------ |
| No         | `/tmp/mqttbroker-un-mqtt`  | `un-mqtt`                                                    |
| Yes        | `/tmp/mqttbroker-un-mqtts` | `un-mqtts`                                                   |

#### MQTT over WebSockets

Clients can connect via WebSockets using both *IPv4* and *IPv6*.

| Protocol | Encryption | Local Port | Request Target | Sec-WebSocket-Protocol | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| -------- | ---------- | ---------- | -------------- | ---------------------- | ------------------------------------------------------------ |
| **IPv4** | No         | `8080`     | `/` or `/ws`   | `mqtt`                 | `in-http`                                                    |
| **IPv4** | Yes        | `8088`     | `/` or `/ws`   | `mqtt`                 | `in-https`                                                   |
| **IPv6** | No         | `8080`     | `/` or `/ws`   | `mqtt`                 | `in6-http`                                                   |
| **IPv6** | Yes        | `8088`     | `/` or `/ws`   | `mqtt`                 | `in6-https`                                                  |

------

### Web Interface

The *MQTTBroker Web Interface* provides real-time visibility into active client connections.

| Protocol | Encryption | Local Port | Request Target    | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| -------- | ---------- | ---------- | ----------------- | ------------------------------------------------------------ |
| **IPv4** | No         | `8080`     | `/` or `/clients` | `in-http`                                                    |
| **IPv4** | Yes        | `8088`     | `/` or `/clients` | `in-https`                                                   |
| **IPv6** | No         | `8080`     | `/` or `/clients` | `in6-http`                                                   |
| **IPv6** | Yes        | `8088`     | `/` or `/clients` | `in6-https`                                                  |

## MQTTIntegrator

### Connection Options

The *MQTTBroker* supports multiple connection methods across various protocols. Additional connection protocols e.g. Bluetooth can be easily added in the [`mqttintegrator.cpp`](https://github.com/SNodeC/mqttsuite/blob/master/mqttintegrator/mqttintegrator.cpp)source code.

All aspects like port numbers, sun paths etc. can be configured using the [SNode.C configuration system](https://github.com/SNodeC/snode.c?tab=readme-ov-file#configuration) either in-code, on the command line or via a configuration file.

In case an encrypted access is required [suitable certificates needs to be provided](https://github.com/SNodeC/snode.c?tab=readme-ov-file#ssltls-configuration-section-tls) also.

***Note:*** A *session store* needs to be configured in case the current sessions shall be made persistent in case the MQTTIntegrator is stopped by using the command line option `--mqtt-session-store <path-to-session-store-file>`.

***Note:*** A *[mapping description file](#MQTT-Mapping-Description)* needs to be provided via the command line option `--mqtt-mapping-file <path-to-mqtt-mapping-file.json>`. 
This JSON file is used to provide the IoT logic by mapping mqtt messages received from a mqtt broker to outgoing mqtt messages. The translation of topics and messages are described by that file.

***Note:*** Both options can be made *persistent* by storing their values in a *configuration file* by appending the switch `--write-config` or `-w` on the command line.

#### MQTT over TCP/IP

| Protocol | Encryption | Remote Port | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| -------- | ---------- | ----------- | ------------------------------------------------------------ |
| **IPv4** | No         | `1883`      | `in-mqtt`                                                    |
| **IPv4** | Yes        | `8883`      | `in-mqtts`                                                   |
| **IPv6** | No         | `1883`      | `in6-mqtt`                                                   |
| **IPv6** | Yes        | `8883`      | `in6-mqtts`                                                  |

#### MQTT over Unix Domain Sockets

| Encryption | Remote Sun Path            | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| ---------- | -------------------------- | ------------------------------------------------------------ |
| No         | `/tmp/mqttbroker-un-mqtt`  | `un-mqtt`                                                    |
| Yes        | `/tmp/mqttbroker-un-mqtts` | `un-mqtts`                                                   |

#### MQTT over WebSockets

| Protocol | Encryption | Remote Port | Requested Target | Sec-WebSocket-Protocol | [Instance](https://github.com/SNodeC/snode.c?tab=readme-ov-file#instance-configuration) |
| -------- | ---------- | ----------- | ---------------- | ---------------------- | ------------------------------------------------------------ |
| **IPv4** | No         | `8080`      | `/ws`            | `mqtt`                 | `in-wsmqtt`                                                  |
| **IPv4** | Yes        | `8088`      | `/ws`            | `mqtt`                 | `in-wsmqtts`                                                 |
| **IPv6** | No         | `8080`      | `/ws`            | `mqtt`                 | `in6-wsmqtt`                                                 |
| **IPv6** | Yes        | `8088`      | `/ws`            | `mqtt`                 | `in6-wsmqtts`                                                |

***Note:*** All instances are active after installation.

***Note:*** To inactivate (disable) instances selectively as they are not needed for communication the command line switch `--disabled` must be used on that instances.

## MQTT Mapping Description

The logic of an IoT application is described by a JSON mapping description file. This file describes the translation of topics and messages of incoming mqtt messages from a mqtt broker. The translated topics and messages are send back to the broker.

The JSON structure of that file ([example](https://github.com/SNodeC/mqttsuite/blob/master/mapfile.json)) needs to fulfill a well defined [JSON schema definition](https://github.com/SNodeC/mqttsuite/blob/master/lib/mapping-schema.json).

### The Structure of the JSON Mapping Description

The big Structure of the JSON mapping description consists of three objects:

- `discover_prefix`: The prefix topic-level of the discover topics of an application (currently not used).
- `connection`: This object describes options for the MQTT connection.
- `mapping`: This object describes the mappings of incoming topics|messages to outgoing topics/messages.

```json
{
    "discover_prefix" : "snode.c",         // discover prefix of the application (not used currently)
    "connection" : {                       // connection options
        "keep_alive" : 60,                 // [60]    duration between two ping packets
        "client_id" : "Client",            // [""]    logical name of the client
        "clean_session" : true,            // [true]  start with clean session or pick up retained one
        "will_topic" : "will/topic",       // [""]    last will topic
        "will_message" : "Last Will",      // [""]    last will message
        "will_qos" : 0,                    // [0]     last will qos
        "will_retain" : false,             // [false] retain last will or not
        "username" : "Username",           // [""]    username
        "password" : "Password"            // [""]    password
    },
    "mapping" : {                          // mapping descriptions of templates|messages mapping
        ...
    }
}
```

### Topic Levels and the JSON-Structure of the `mapping` Section

- A `topic_level` is either
  - an object describing one topic or
  - an *array* of objects describing multiple *sibling* topics 
- `topic_level`s can be *recursively nested* to create a `topic_level` tree.

#### Single Topic Level

```json
"mapping" : {
    "topic_level" : {         // single topic level
        "name" : "topic_level_name",
    }
}
```
#### Nested Topic Levels

```json
"mapping" : {
    "topic_level" : {         // parent topic level
        "name" : "topic_level_name",
        "topic_level" : {     // sub-topic level
            "name" : "sub_topic_level_name",
        }
    }
}
```
#### Sibling and Nested Topic Levels

```json
"mapping" : {
    "topic_level" : [{        // 1st sibling topic level
        "name" : "sibling_1_topic_level_name"
    }, {                      // 2nd sibling topic level
        "name" : "sibling_2_topic_level_name",
        "topic_level" : {     // sub-topic level of 2nd sibling
            "name" : "sub_topic_level_name",
        }
    }]
}
```

#### A More Complex Topic Level Structure

<img src="docs/images/mqtt-topics.png" alt="A complexer topic_level Structure"/>

```json
"mapping" : {
    "topic_level" : {
        "name" : "Home",
        "topic_level" : [{
            "name" : "Bedroom",
            "topic_level" : [{
                "name" : "...",
            }, {
                "name" : "...",
            }],
        },
        {
            "name" : "Garage",
        },
        {
            "name" : "Kitchen",
            "topic_level" : [{
                "name" : "Fridge",
                "topic_level" : [{
                    "name" : "Temperature",
                },
                {
                    "name" : "IceLevel",
                }],
            },
            {
                "name" : "Coffeemaker",
            }],
        }]
    }
}
```

#### The `subscription` Object of a Topic Level

The `topic_level` object can contain a `subscription` object, which forces the MQTTIntegrator to *subscribes for this topic*.
This `subscription` object describes concrete *topic to topic* and *message to message* mappings.

```json
"topic_level" : {
    "name" : "device",
    "topic_level" : {
        "name" : "sensor",
        "subscription" : {
            "type" : "binary_sensor",
            "qos" : 2,
            <mapping-section>: {
            }
        }
    }
}
```

Three different kinds of `<mapping-section>` exist:

- **Static** mapping is thought for a *string-matching* mapping of incoming messages to outgoing one.
- **Value** mapping is used if the incoming message should be *interpreted as a value*. The actual mapping is done controlled by an [*inja*](https://github.com/pantor/inja) template, whereas the message is accessible via the template variable `message`.
- **JSON** mapping can be used in case the *incoming message* is a *JSON object*. The \alert{actual mapping} is done controlled by an [*inja*](https://github.com/pantor/inja) template. The JSON objects are accessible in the template *as elements* of the template JSON variable `message`.

##### **Static** Mapping

The `static` mapping section describes the mapped topic and the message mapping where the *incoming message* is *string-matched* to *string-values* specified in the `message_mappings` array section.

```json
"static" : {
    "mapped_topic" : "other_device/some_actuator/set",
    "message_mapping" : [{
        "message": "pressed",      // original message
        "mapped_message": "on"     // mapped message
    },
    {
        "message": "released",     // original message
        "mapped_message": "off"    // mapped message
    }],
    "retain" : false,
    "qos": 0
}
```

##### **Value** Mapping

The `value` mapping section describes the *mapped topic* and the *message mapping* where the *incoming message* is available in the variable `message` and is interpreted as *value* of an *inja template*.

```json
"value" : {
    "mapped_topic" : "other_device/some_actuator/set",
    "mapping_template" : "{% if message == \"pressed\" %}on{% else if message == \"released\" %}off{% endif %}",
    "retain" : false,
    "qos" : 0
}
```

##### **JSON** Mapping

The `json` mapping section can be used in case the *incoming message is a JSON* object. The actual mapping is controlled by an *inja template*.
The *JSON object* is accessible in the template as *elements* of the template *JSON variable* `message`.

```json
"json" : {
    "mapped_topic" : "other_device/some_actuator/set",
    "mapping_template" :  "{{ message.time.start }} to {{ message.time.end + 1 }}pm",
    "retain" : false,
    "qos" : 0
}
```
E.g. the JSON message

```json
{
    "time" : {
        "start" : 5,
        "end" : 10
    }
}
```

is rendered by the above `mapping_template` as **5 to 11pm**.

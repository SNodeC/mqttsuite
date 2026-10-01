# MQTTSuite binary packages

[← Install MQTTSuite](install.md)

The **SNode.C package feed** publishes both SNode.C and MQTTSuite. Choose your distribution below, register its signed source and install the applications you need.

## Choose distribution, release and architecture

Catalog snapshot: **1 October 2026**. Check the [live per-target status][status] for available versions and publication results.

| Distribution | Releases | Package architectures | Guide |
| --- | --- | --- | --- |
| Debian | Trixie, Forky, Sid | `amd64`, `arm64`, `armhf`, `riscv64` | [Debian][debian] |
| Ubuntu | Noble, Resolute | `amd64`, `arm64` | [Ubuntu][ubuntu] |
| Raspberry Pi OS | Bookworm, Trixie | `arm64`; Pi 3, 4 and 5 | [Raspberry Pi OS][raspberrypi] |
| Rocky Linux | 9, 10 | `x86_64`, `aarch64` | [Rocky Linux][rocky] |
| Fedora | 43, 44 | `x86_64`, `aarch64` | [Fedora][fedora] |
| OpenWrt | 24.10, 25.12 | 25 package architectures per release; ARM, AArch64, x86, MIPS, PowerPC, RISC-V and LoongArch variants | [OpenWrt][openwrt] |

Match the installed distribution and release as well as the architecture. Raspberry Pi OS Bookworm does not imply a Debian Bookworm feed. Debian Sid may need explicit suite selection, and Rocky Linux requires the documented CRB/EPEL prerequisites.

## Prepare the signed feed

Follow the matching guide to register the signed feed. Its installer normally installs both full project sets. Use **`--prepare`** when you only want to register the source and refresh indexes, then install selected packages. Review scripts before running them with administrative privileges; manual setup is documented too.

## Install with the system package manager

| Package | Contents |
| --- | --- |
| `mqttsuite-broker` | Broker, associated library, WebSocket plugin and web assets. |
| `mqttsuite-integrator` | Mapping/integration application and related components. |
| `mqttsuite-bridge` | Bridge, related components and web assets. |
| `mqttsuite-cli` | Command-line publisher/subscriber. |
| `mqttsuite-store` | MariaDB storage application; database configuration still required. |
| `mqttsuite-mapping-double` | Double mapping plugin. |
| `mqttsuite-mapping-storage` | Storage mapping plugin; distinct from the MQTTStore service. |

Install `mqttsuite` for the complete DEB/RPM set, or `mqttsuite-full` for the OpenWrt set. The package manager resolves framework dependencies. Retain official distribution sources for system dependencies and keep signature verification enabled.

```sh
# APT-based systems, after feed preparation
sudo apt-get install mqttsuite
```

```sh
# RPM-based systems, after feed preparation
sudo dnf install mqttsuite
```

## OpenWrt

After preparing the matching feed:

```sh
# OpenWrt 24.10, as root
opkg install mqttsuite-full
```

```sh
# OpenWrt 25.12, as root
apk add mqttsuite-full
```

For a smaller OpenWrt installation, choose individual application packages. Use `/etc/openwrt_release` and `DISTRIB_ARCH` to select packages; `uname -m` alone is insufficient. For other targets or custom firmware, consult the [SDK build guide][sdk].

## Updates and verification

Use your distribution's normal package-manager update procedure, review the proposed changes and retain signature verification. Compare installed versions with the [published status][status], and check the applications you installed using the [installation verification commands](install.md#verify-the-installation). Keep application libraries, executables and their SNode.C dependencies compatible; package updates do not replace a check of your own configuration and workload.

[Full component catalog][components] · [OpenWrt catalog][openwrt-components] · [Feed and signing keys][feed] · [Installation entry point][install] · [Troubleshooting][troubleshooting]

<!-- Rename-sensitive external destinations are deliberately centralized here. -->
[feed]: https://github.com/SNodeC/OpenWRT
[install]: https://github.com/SNodeC/OpenWRT/blob/main/docs/install-mqttsuite.md
[status]: https://github.com/SNodeC/OpenWRT/blob/packages/README.md
[debian]: https://github.com/SNodeC/OpenWRT/blob/main/docs/debian.md
[ubuntu]: https://github.com/SNodeC/OpenWRT/blob/main/docs/ubuntu.md
[raspberrypi]: https://github.com/SNodeC/OpenWRT/blob/main/docs/raspberrypi.md
[rocky]: https://github.com/SNodeC/OpenWRT/blob/main/docs/rocky.md
[fedora]: https://github.com/SNodeC/OpenWRT/blob/main/docs/fedora.md
[openwrt]: https://github.com/SNodeC/OpenWRT/blob/main/docs/openwrt.md
[components]: https://github.com/SNodeC/OpenWRT/blob/main/docs/linux.md
[openwrt-components]: https://github.com/SNodeC/OpenWRT/blob/main/docs/mqttsuite-package-options.md
[sdk]: https://github.com/SNodeC/OpenWRT/blob/main/docs/openwrt-build.md
[troubleshooting]: https://github.com/SNodeC/OpenWRT/blob/main/docs/troubleshooting.md

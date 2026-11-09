#!/bin/sh
set -eu
: "${SOURCE_DIR:?}" "${SDK_DIR:?}" "${SDK_STAGING_DIR:?}" "${SOURCE_TAG:?}"
mkdir -p /work/recipes/mqttsuite "$SDK_DIR/dl"
cp -a "$SOURCE_DIR/misc/openwrt/." /work/recipes/mqttsuite/
cp -a "$SDK_STAGING_DIR/usr/share/core-ci/openwrt/snode.c" /work/recipes/snode.c
mkdir -p "$SDK_STAGING_DIR/pkginfo"
cp -a "$SDK_STAGING_DIR/usr/share/core-ci/pkginfo/." "$SDK_STAGING_DIR/pkginfo/"
export MQTTSUITE_PACKAGE_RELEASE="$PACKAGE_REVISION"
export MQTTSUITE_SOURCE_TAG="$SOURCE_TAG"
export MQTTSUITE_PACKAGE_VERSION="$PACKAGE_UPSTREAM_VERSION"
archive="$SDK_DIR/dl/mqttsuite-$PACKAGE_UPSTREAM_VERSION.tar.gz"
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    --transform="s,^\.,mqttsuite-$PACKAGE_UPSTREAM_VERSION," -czf "$archive" -C "$SOURCE_DIR" .
MQTTSUITE_SOURCE_HASH=$(sha256sum "$archive" | cut -d ' ' -f1)
export MQTTSUITE_SOURCE_HASH
core_version=$(python3 - <<'VERSION'
import json, os
from pathlib import Path
metadata = json.loads((Path(os.environ["PREREQUISITES_DIR"]) / "snodec/metadata.json").read_text())
print(metadata["upstream_version"], metadata["revision"])
VERSION
)
SNODEC_PACKAGE_VERSION=${core_version% *}
SNODEC_PACKAGE_RELEASE=${core_version##* }
export SNODEC_PACKAGE_VERSION SNODEC_PACKAGE_RELEASE
cd "$SDK_DIR"
printf '\nsrc-link core_ci /work/recipes\n' >> feeds.conf.default
./scripts/feeds update base packages core_ci
./scripts/feeds install -a -p core_ci
cat > .config <<'CONFIG'
# CONFIG_ALL is not set
# CONFIG_ALL_NONSHARED is not set
# CONFIG_ALL_KMODS is not set
# CONFIG_AUTOREMOVE is not set
# CONFIG_SIGNED_PACKAGES is not set
CONFIG_PACKAGE_mqttsuite=m
CONFIG
make defconfig
# Reusing core skips its compile target, so prepare that target's SDK dependencies first.
dependencies=$(make --no-print-directory -s -f - dependencies <<'DEPENDENCIES'
curdir:=package
include .config
include tmp/.packagedeps
.PHONY: dependencies
dependencies:
	@printf '%s\n' '$(package/feeds/core_ci/snode.c/compile)'
DEPENDENCIES
)
test -n "$dependencies"
make -j"$(nproc)" $dependencies V=s BUILD_LOG=1
make -j"$(nproc)" MAKE='make -o package/feeds/core_ci/snode.c/compile' package/mqttsuite/compile V=s BUILD_LOG=1
python3 /tool/openwrt_container.py --outputs

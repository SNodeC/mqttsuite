#!/bin/sh
set -eu
: "${SOURCE_DIR:?}" "${SDK_DIR:?}" "${SDK_STAGING_DIR:?}" "${SOURCE_COMMIT:?}"
mkdir -p /work/recipes/mqttsuite "$SDK_DIR/dl"
cp -a "$SOURCE_DIR/misc/openwrt/." /work/recipes/mqttsuite/
cp -a "$SDK_STAGING_DIR/usr/share/core-ci/openwrt/snode.c" /work/recipes/snode.c
mkdir -p "$SDK_STAGING_DIR/pkginfo"
cp -a "$SDK_STAGING_DIR/usr/share/core-ci/pkginfo/." "$SDK_STAGING_DIR/pkginfo/"
export MQTTSUITE_PACKAGE_RELEASE="$PACKAGE_RELEASE"
export MQTTSUITE_SOURCE_TAG="$SOURCE_COMMIT"
export MQTTSUITE_PACKAGE_VERSION="$PACKAGE_UPSTREAM_VERSION"
archive="$SDK_DIR/dl/mqttsuite-$PACKAGE_UPSTREAM_VERSION.tar.gz"
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    --transform="s,^\.,mqttsuite-$PACKAGE_UPSTREAM_VERSION," -czf "$archive" -C "$SOURCE_DIR" .
MQTTSUITE_SOURCE_HASH=$(sha256sum "$archive" | cut -d ' ' -f1)
export MQTTSUITE_SOURCE_HASH
SNODEC_PACKAGE_VERSION=$(python3 -c 'import json,os,pathlib; r=json.load(open(next(pathlib.Path(os.environ["PREREQUISITES_DIR"]).glob("*/receipt.json")))); print(r["package_version"]["version"])')
SNODEC_PACKAGE_RELEASE=$(python3 -c 'import json,os,pathlib; r=json.load(open(next(pathlib.Path(os.environ["PREREQUISITES_DIR"]).glob("*/receipt.json")))); print(r["package_version"]["release"])')
export SNODEC_PACKAGE_VERSION SNODEC_PACKAGE_RELEASE
cd "$SDK_DIR"
printf '\nsrc-link core_ci /work/recipes\n' >> feeds.conf.default
./scripts/feeds update -a
./scripts/feeds install -a
printf '\nCONFIG_PACKAGE_mqttsuite=m\n' >> .config
make defconfig
make -j2 MAKE='make -o package/feeds/core_ci/snode.c/compile' package/mqttsuite/compile V=s BUILD_LOG=1
python3 /tool/openwrt_container.py --outputs

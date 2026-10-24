#!/usr/bin/env bash
cp -a source/misc/openwrt/. recipes/mqttsuite/
export MQTTSUITE_PACKAGE_RELEASE="$PACKAGE_RELEASE"
export MQTTSUITE_SOURCE_TAG=$(jq -r '.source_tags.mqttsuite' "$bundle/context.json")
export MQTTSUITE_PACKAGE_VERSION=$(jq -r '.versions.mqttsuite' "$bundle/context.json")
export MQTTSUITE_SOURCE_HASH=$(sha256sum "$bundle/mqttsuite-"*.tar.gz | cut -d' ' -f1)
compile() {
    make -j"$(nproc)" MAKE="make -o package/feeds/package-ci/snode.c/compile" package/mqttsuite/compile V=s BUILD_LOG=1
    python3 -m ci.repository sdk-export "$sdk" "$bundle" recipes/mqttsuite
    return 0
}

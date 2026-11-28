#!/bin/sh
set -eu
recipe="$SDK_DIR/package/mqttsuite"
mkdir "$recipe"
cp -R "$CI_DIR/openwrt/." "$recipe/"
dependency=$(printf '%s' "$DEPENDENCIES_JSON" | jq -er '
    if length == 1 and .[0].package == "snodec" then
        "snodec (=\(.[0].version)-r\(.[0].revision))"
    else error("Expected the resolved snodec dependency") end')
{
    printf 'MQTTSUITE_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'MQTTSUITE_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'MQTTSUITE_SOURCE_DIR:=%s\n' "$SOURCE_DIR"
    printf 'MQTTSUITE_SNODEC_DEPENDENCY:=%s\n' "$dependency"
    cat "$CI_DIR/openwrt/Makefile"
} > "$recipe/Makefile"

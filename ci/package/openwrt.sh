#!/bin/sh
set -eu
recipe="$SDK_DIR/feeds/base/mqttsuite"
rm -rf "$recipe"
mkdir -p "$recipe" "$OUTPUT_DIR"
ln -s "$recipe" "$SDK_DIR/package/mqttsuite"
cp -R "$CI_DIR/openwrt/." "$recipe/"
dependency=$(printf '%s' "$DEPENDENCIES_JSON" | jq -er '
    if length == 1 and .[0].package == "snodec" then
        "snodec (= \(.[0].version)-r\(.[0].revision))"
    else error("Expected the resolved snodec dependency") end')
{
    printf 'MQTTSUITE_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'MQTTSUITE_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'MQTTSUITE_SOURCE_DIR:=%s\n' "$SOURCE_DIR"
    printf 'EXTRA_DEPENDS:=%s\n' "$dependency"
    cat "$CI_DIR/openwrt/Makefile"
} > "$recipe/Makefile"
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_mqttsuite=y\n' >> .config
make defconfig
make package/mqttsuite/compile CMAKE_BUILD_TYPE=Release V=s -j"$(nproc)"
find bin/packages -type f \( -name 'mqttsuite*.ipk' -o -name 'mqttsuite*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;

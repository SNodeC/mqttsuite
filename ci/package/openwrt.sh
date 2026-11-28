#!/bin/sh
set -eu
mkdir -p "$OUTPUT_DIR"
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_mqttsuite=y\n' >> .config
make defconfig
make package/mqttsuite/compile CMAKE_BUILD_TYPE=Release V=s -j"$(nproc)"
find bin/packages -type f \( -name 'mqttsuite*.ipk' -o -name 'mqttsuite*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;

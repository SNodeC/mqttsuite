#!/bin/sh
set -eu
recipe="$SDK_DIR/feeds/base/mqttsuite"
mkdir -p "$recipe" "$SDK_DIR/dl" "$OUTPUT_DIR"
ln -s "$recipe" "$SDK_DIR/package/mqttsuite"
cp -R "$SOURCE_DIR/misc/openwrt/." "$recipe/"
commit=$(git -C "$SOURCE_DIR" rev-parse HEAD)
{
    printf 'MQTTSUITE_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'MQTTSUITE_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'MQTTSUITE_SOURCE_TAG:=%s\n' "$commit"
    cat "$SOURCE_DIR/misc/openwrt/Makefile"
} > "$recipe/Makefile"
archive="$SDK_DIR/dl/mqttsuite-$PACKAGE_VERSION.tar"
git -C "$SOURCE_DIR" archive --format=tar --prefix="mqttsuite-$PACKAGE_VERSION/" -o "$archive" HEAD
git -c "safe.directory=$SOURCE_DIR/lib/json-schema-validator" -C "$SOURCE_DIR/lib/json-schema-validator" archive --format=tar \
    --prefix="mqttsuite-$PACKAGE_VERSION/lib/json-schema-validator/" -o "$archive.submodule" HEAD
tar --concatenate --file="$archive" "$archive.submodule"
gzip -nf "$archive"
rm "$archive.submodule"
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_mqttsuite=y\n' >> .config
make defconfig
make package/mqttsuite/compile CMAKE_BUILD_TYPE=Release FORCE_UNSAFE_CONFIGURE=1 V=s -j"$(nproc)"
find bin/packages -type f \( -name 'mqttsuite*.ipk' -o -name 'mqttsuite*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;

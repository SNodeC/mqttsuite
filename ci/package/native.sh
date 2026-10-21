#!/usr/bin/env bash
# Product dependencies and build policy are versioned with this source.
if [ "$format" = DEB ]; then
    apt-get install -y libbluetooth-dev libmagic-dev libmariadb-dev libssl-dev openssl libncurses-dev nlohmann-json3-dev libsqlite3-dev
else
    dnf install -y bluez-libs-devel file-devel mariadb-connector-c-devel openssl-devel openssl ncurses-devel json-devel sqlite-devel
fi
cmake -S sources/mqttsuite -B build-product -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR="$libdir" -DCMAKE_INSTALL_SYSCONFDIR=/etc
cmake --build build-product --parallel 4
package build-product

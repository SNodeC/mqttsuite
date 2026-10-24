#!/usr/bin/env bash
# Product dependencies and build policy are versioned with this source.
if [ "$format" = DEB ]; then
    apt-get install -y build-essential cmake ninja-build pkg-config \
        libbluetooth-dev libmagic-dev libmariadb-dev libssl-dev openssl libncurses-dev nlohmann-json3-dev libsqlite3-dev
else
    dnf install -y gcc gcc-c++ cmake ninja-build make pkgconf-pkg-config rpm-build \
        bluez-libs-devel file-devel mariadb-connector-c-devel openssl-devel openssl ncurses-devel json-devel sqlite-devel
    if [ "$DISTRIBUTION:$SUITE" = rocky:9 ]; then
        dnf install -y gcc-toolset-14-gcc-c++
        source /opt/rh/gcc-toolset-14/enable
    fi
fi
cmake -S sources/mqttsuite -B build-product -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR="$libdir" -DCMAKE_INSTALL_SYSCONFDIR=/etc
cmake --build build-product --parallel 4
package build-product

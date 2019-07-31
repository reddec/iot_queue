#!/usr/bin/env bash
set -e -o pipefail
ARCH="armv6"
IMAGE=dockcross/linux-$ARCH:20190724-9658ba3
docker pull $IMAGE
rm -rf build/$ARCH
docker run --rm $IMAGE > ./dockcross-linux-$ARCH
chmod +x ./dockcross-linux-$ARCH
./dockcross-linux-$ARCH cmake -Bbuild/$ARCH -H. \
  -DWITH_PERF_TOOL=OFF \
  -DBUILD_SHARED=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_STATIC=ON \
  -DCMAKE_BUILD_TYPE=Release

./dockcross-linux-$ARCH make -C build/$ARCH
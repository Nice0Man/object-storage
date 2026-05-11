#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-backend/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"
INSTALL_DEPS="${INSTALL_DEPS:-1}"

if [[ "${INSTALL_DEPS}" == "1" ]]; then
  if command -v sudo >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config git curl unzip zip tar \
      libssl-dev libjsoncpp-dev libspdlog-dev nlohmann-json3-dev \
      libdrogon-dev librocksdb-dev
  else
    apt-get update
    apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config git curl unzip zip tar \
      libssl-dev libjsoncpp-dev libspdlog-dev nlohmann-json3-dev \
      libdrogon-dev librocksdb-dev
  fi
fi

cmake -S backend -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_TESTS=OFF \
  -DENABLE_LTO=ON \
  -DENABLE_STRIP=ON \
  -DENABLE_UNITY_BUILD=ON

cmake --build "${BUILD_DIR}" --target console --parallel "${BUILD_JOBS}"

echo "Backend binary ready: ${BUILD_DIR}/bin/console"

#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-backend/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"
INSTALL_DEPS="${INSTALL_DEPS:-1}"
BUILD_TESTS="${BUILD_TESTS:-1}"
RUN_TESTS="${RUN_TESTS:-0}"

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
  -DBUILD_TESTS="$([[ "${BUILD_TESTS}" == "1" ]] && echo ON || echo OFF)" \
  -DENABLE_LTO=ON \
  -DENABLE_STRIP=ON \
  -DENABLE_UNITY_BUILD=ON

if [[ "${BUILD_TESTS}" == "1" ]]; then
  cmake --build "${BUILD_DIR}" --parallel "${BUILD_JOBS}"
else
  cmake --build "${BUILD_DIR}" --target console --parallel "${BUILD_JOBS}"
fi

if [[ "${RUN_TESTS}" == "1" ]]; then
  if [[ "${BUILD_TESTS}" != "1" ]]; then
    echo "RUN_TESTS=1 requires BUILD_TESTS=1"
    exit 1
  fi
  ctest --test-dir "${BUILD_DIR}" --output-on-failure --parallel "${BUILD_JOBS}"
fi

echo "Backend binary ready: ${BUILD_DIR}/bin/console"

#!/bin/bash
apt-get update && apt-get install -y \
    libdrogon-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    libssl-dev \
    libboost-all-dev \
    libgtest-dev \
    libgmock-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev

echo "=== jwt-cpp (header-only) ==="
cd /tmp
if [ ! -d "jwt-cpp" ]; then
    git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git
fi
cp -r jwt-cpp/include/jwt-cpp /usr/local/include/ || true

ldconfig
echo "Done!"

#!/bin/bash
set -e

echo "=== Installing C++ dependencies ==="

# Update package list
sudo apt-get update

# Install build tools
echo "Installing build tools..."
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libssl-dev \
    zlib1g-dev

# Install Drogon dependencies
echo "Installing Drogon dependencies..."
sudo apt-get install -y \
    libjsoncpp-dev \
    uuid-dev \
    libsqlite3-dev \
    libpq-dev \
    libmariadb-dev \
    libbrotli-dev

# Install other dependencies
echo "Installing other dependencies..."
sudo apt-get install -y \
    nlohmann-json3-dev \
    libspdlog-dev \
    libssl-dev \
    libboost-all-dev

# Install GoogleTest
echo "Installing GoogleTest..."
sudo apt-get install -y \
    libgtest-dev \
    libgmock-dev

echo ""
echo "=== Cloning and building Drogon ==="
if [ ! -d "/tmp/drogon" ]; then
    cd /tmp
    git clone https://github.com/drogonframework/drogon.git
    cd drogon
    git submodule update --init
    mkdir -p build
    cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    make -j$(nproc)
    sudo make install
    sudo ldconfig
else
    echo "Drogon already cloned, skipping..."
fi

echo ""
echo "=== Cloning and building jwt-cpp ==="
if [ ! -d "/tmp/jwt-cpp" ]; then
    cd /tmp
    git clone https://github.com/Thalhammer/jwt-cpp.git
    cd jwt-cpp
    mkdir -p build
    cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DJWT_BUILD_EXAMPLES=OFF
    sudo make install
else
    echo "jwt-cpp already cloned, skipping..."
fi

echo ""
echo "=== All dependencies installed! ==="

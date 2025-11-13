# Инструкция по установке зависимостей

## Быстрая установка (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
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
```

## Установка jwt-cpp (header-only library)

```bash
cd /tmp
git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git
sudo cp -r jwt-cpp/include/jwt-cpp /usr/local/include/
```

## Сборка проекта

### Debug build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LTO=OFF
cmake --build build -j$(nproc)
```

### Release build (оптимизированная)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Запуск тестов
```bash
cd build
ctest -V
```

### Запуск приложения
```bash
./build/bin/console
```

## Альтернатива: vcpkg

Если системные пакеты недоступны, используйте vcpkg:

```bash
# Установка vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Установка зависимостей
./vcpkg install \
    drogon \
    nlohmann-json \
    jwt-cpp \
    spdlog \
    openssl \
    boost-system \
    gtest

# Сборка с vcpkg
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build -j$(nproc)
```

## Проверка зависимостей

```bash
# Проверка установленных пакетов
dpkg -l | grep -E "(drogon|nlohmann|spdlog|jsoncpp)"

# Проверка pkg-config
pkg-config --modversion jsoncpp
pkg-config --cflags jsoncpp
```

## Решение проблем

### Drogon не найден
```bash
sudo apt-get install libdrogon-dev
```

### jwt-cpp не найден
```bash
cd /tmp
git clone https://github.com/Thalhammer/jwt-cpp.git
sudo cp -r jwt-cpp/include/jwt-cpp /usr/local/include/
```

### spdlog не найден
```bash
sudo apt-get install libspdlog-dev
```

## Минимальные требования

- CMake >= 3.20
- C++20 compatible compiler (GCC 11+, Clang 14+)
- Drogon >= 1.8
- nlohmann-json >= 3.10
- jwt-cpp >= 0.6
- spdlog >= 1.9
- OpenSSL >= 1.1
- Boost >= 1.70
- GoogleTest >= 1.11 (для тестов)


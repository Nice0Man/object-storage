# Object Storage Console

[![CI](https://github.com/Nice0Man/object-storage/actions/workflows/ci.yml/badge.svg)](https://github.com/Nice0Man/object-storage/actions/workflows/ci.yml)
[![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL%203.0-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![Drogon](https://img.shields.io/badge/Drogon-1.9+-green.svg)](https://github.com/drogonframework/drogon)

A modern, high-performance S3-compatible object storage web console built with C++20 and Drogon framework.

## ✨ Features

- 🚀 **High Performance** - Built with modern C++20 and Drogon async framework
- 🔐 **Secure** - JWT authentication, SSL/TLS support, LDAP integration
- 🎨 **Modern UI** - React-based SPA with beautiful Material Design
- 📦 **S3 Compatible** - Works with MinIO, AWS S3, and any S3-compatible storage
- 🔄 **Real-time** - WebSocket support for live updates
- 📊 **Monitoring** - Built-in metrics and health checks
- 🧪 **Well Tested** - Comprehensive unit and integration tests
- 📝 **Well Documented** - Extensive documentation and examples

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────┐
│                  React SPA (Web UI)                  │
├─────────────────────────────────────────────────────┤
│              REST API (Drogon C++20)                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │   Auth   │  │ Buckets  │  │ Objects  │          │
│  └──────────┘  └──────────┘  └──────────┘          │
├─────────────────────────────────────────────────────┤
│            S3 Client (MinIO SDK / AWS SDK)           │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│         S3-Compatible Object Storage (MinIO)         │
└─────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- **C++ Compiler**: GCC 11+, Clang 14+, or MSVC 2022+
- **CMake**: 3.20 or higher
- **vcpkg**: For dependency management
- **Node.js**: 18+ (for frontend)

### Build

```bash
# Clone repository
git clone https://github.com/Nice0Man/object-storage.git
cd object-storage

# Install dependencies via vcpkg
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install

# Build backend
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)

# Build frontend
cd web-app
npm install
npm run build
cd ..

# Run
./build/bin/console config.json
```

### Docker

```bash
docker build -t object-storage-console .
docker run -p 9090:9090 -v $(pwd)/config.json:/app/config.json object-storage-console
```

## ⚙️ Configuration

Create `config.json` based on `config.example.json`:

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 9090,
    "enable_ssl": false
  },
  "s3": {
    "endpoint": "localhost:9000",
    "access_key": "minioadmin",
    "secret_key": "minioadmin"
  },
  "auth": {
    "jwt_secret": "your-secret-key"
  }
}
```

### Environment Variables

```bash
# Server
CONSOLE_HOST=0.0.0.0
CONSOLE_PORT=9090

# S3 (optional, usually empty for local storage)
S3_ENDPOINT=localhost:9000
S3_ACCESS_KEY=
S3_SECRET_KEY=

# Security
JWT_SECRET=your-secret-key-change-in-production

# Default Admin (change in production!)
DEFAULT_ADMIN_USERNAME=admin
DEFAULT_ADMIN_PASSWORD=changeme
DEFAULT_ADMIN_ACCOUNT_NAME=Administrator
DEFAULT_ADMIN_ENABLED=true
```

> **⚠️ Security Note:** Пароли теперь хешируются с использованием PBKDF2-SHA256. 
> См. [Security Improvements](docs/SECURITY_IMPROVEMENTS.md) для деталей.

## 🧪 Testing

```bash
# Unit tests
ctest --test-dir build --output-on-failure

# Integration tests
./build/bin/tests/integration_tests

# With coverage
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage
```

## 📚 Documentation

### Main Documentation

- [Architecture Overview](docs/01-architecture.md)
- [Backend Deep Dive](docs/02-backend-deep-dive.md)
- [Frontend Architecture](docs/03-frontend-architecture.md)
- [API Specification](docs/09-api-specification.md)
- [Build & Deployment](docs/07-build-deployment.md)
- [Testing Guide](docs/08-testing.md)

### Planning & Development

- [Implementation Plan](stages/01-implementation-plan.md) - Roadmap and current progress (~40%)
- [Pre-commit Fixes Guide](stages/02-fixes-guide.md) - Detailed guide for fixing linter issues
- [Quick Fix Guide](stages/03-quick-fix.md) - Fast pre-commit problem resolution

## 🛠️ Development

### Code Quality

```bash
# Format code
clang-format -i src/**/*.cpp include/**/*.hpp

# Lint
clang-tidy src/*.cpp -- -std=c++20

# Pre-commit hooks
pre-commit install
pre-commit run --all-files
```

### Project Structure

```
object-storage/
├── src/                    # C++ source files
│   ├── api/               # API controllers
│   ├── models/            # Data models
│   ├── services/          # Business logic
│   ├── middleware/        # Middleware components
│   └── utils/             # Utility functions
├── include/console/        # Header files
│   ├── api/
│   ├── models/
│   ├── services/
│   ├── middleware/
│   └── common/
├── tests/                  # Tests
│   ├── unit/
│   └── integration/
├── web-app/               # React frontend
├── docs/                  # Documentation
├── cmake/                 # CMake modules
└── CMakeLists.txt         # Build configuration
```

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the AGPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Drogon](https://github.com/drogonframework/drogon) - C++ web framework
- [MinIO](https://min.io/) - S3-compatible object storage
- [React](https://reactjs.org/) - Frontend framework
- [Material-UI](https://mui.com/) - React component library

## 📧 Contact

- GitHub: [@Nice0Man](https://github.com/Nice0Man)
- Email: e.krotov03@gmail.com

---

Made with ❤️ using Modern C++20

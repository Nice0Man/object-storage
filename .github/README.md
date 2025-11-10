# CI/CD и Code Quality Configuration

## 📋 Обзор

Этот проект использует комплексную систему проверок качества кода:

- ✅ **GitHub Actions CI/CD** - автоматическая сборка и тестирование
- ✅ **Pre-commit hooks** - проверки перед коммитом
- ✅ **Code formatting** - автоматическое форматирование
- ✅ **Static analysis** - поиск потенциальных проблем
- ✅ **Security scanning** - проверка на утечки секретов

## 🚀 Quick Start

### 1. Установка Pre-commit

```bash
# Установить pre-commit
pip install pre-commit

# Установить хуки в репозиторий
cd /path/to/object-storage
pre-commit install
pre-commit install --hook-type commit-msg

# Запустить проверки вручную
pre-commit run --all-files
```

### 2. Установка инструментов для C++

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    clang-format-14 \
    clang-tidy-14 \
    cppcheck \
    cmake \
    cmake-format
```

#### macOS

```bash
brew install clang-format
brew install cppcheck
brew install cmake
brew install cmake-format
```

#### Windows

```powershell
# Через Chocolatey
choco install llvm cppcheck cmake

# Или через vcpkg
vcpkg install llvm cppcheck
```

## 🔧 Конфигурационные файлы

### GitHub Actions Workflows

#### `.github/workflows/ci.yml`
Основной CI pipeline:
- ✅ Lint проверки (clang-format, clang-tidy, cppcheck)
- ✅ Markdown linting
- ✅ Multi-platform build (Linux, macOS, Windows)
- ✅ Multi-compiler (GCC, Clang, MSVC)
- ✅ Code coverage
- ✅ Documentation checks

#### `.github/workflows/codeql.yml`
Security scanning с CodeQL:
- 🔒 Static security analysis
- 🔒 Vulnerability detection
- 🔒 Weekly scheduled scans

### Pre-commit Configuration

#### `.pre-commit-config.yaml`
Автоматические проверки перед коммитом:
- Formatting (clang-format)
- CMake formatting
- Markdown linting
- Spell checking
- Secret detection
- Shell script validation
- YAML linting

### Code Style

#### `.clang-format`
C++ code formatting (Based on Mozilla style):
- Indent: 4 spaces
- Line length: 120
- Modern C++20 features
- Automatic include sorting

#### `.clang-tidy`
Static analysis checks:
- Bugprone patterns
- Modern C++ practices
- Performance optimizations
- Readability improvements
- Security best practices

#### `.editorconfig`
Universal editor settings:
- UTF-8 encoding
- LF line endings
- Consistent indentation
- Trailing whitespace removal

## 📝 Использование

### Автоматические проверки (Pre-commit)

Pre-commit хуки запускаются автоматически при `git commit`:

```bash
git add .
git commit -m "feat: add new feature"
# Pre-commit автоматически запустит все проверки
```

### Ручной запуск проверок

```bash
# Все проверки на всех файлах
pre-commit run --all-files

# Конкретная проверка
pre-commit run clang-format --all-files
pre-commit run markdownlint --all-files

# Проверка конкретных файлов
pre-commit run --files src/main.cpp include/api.hpp
```

### Форматирование кода

```bash
# Автоматическое форматирование C++ файлов
find src include -name "*.cpp" -o -name "*.hpp" | \
    xargs clang-format -i

# Форматирование CMake файлов
find . -name "CMakeLists.txt" -o -name "*.cmake" | \
    xargs cmake-format -i

# Форматирование Markdown
markdownlint --fix docs/**/*.md
```

### Static Analysis

```bash
# Clang-Tidy
find src -name "*.cpp" | \
    xargs clang-tidy --config-file=.clang-tidy -p build

# Cppcheck
cppcheck --enable=all \
    --suppress-xml=.cppcheck-suppressions.txt \
    --project=build/compile_commands.json \
    src/
```

## 🔍 CI/CD Pipeline

### On Push/Pull Request

```
1. Lint Checks
   ├─ Clang-Format verification
   ├─ Clang-Tidy analysis
   ├─ Cppcheck scanning
   └─ Markdown linting

2. Multi-Platform Build
   ├─ Linux (GCC 11, Clang 14)
   ├─ macOS (Apple Clang)
   └─ Windows (MSVC 2022)

3. Testing
   ├─ Unit tests (Google Test)
   ├─ Integration tests
   └─ Coverage report

4. Security
   ├─ CodeQL analysis
   ├─ Secret scanning
   └─ Dependency checks

5. Documentation
   ├─ Link validation
   └─ File completeness
```

### Coverage Reports

Coverage отчеты автоматически загружаются в Codecov:

```bash
# Локально сгенерировать coverage
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## 🛠️ Troubleshooting

### Pre-commit не работает

```bash
# Переустановить хуки
pre-commit uninstall
pre-commit install
pre-commit install --hook-type commit-msg

# Обновить хуки
pre-commit autoupdate
```

### Clang-format/Clang-tidy не найдены

```bash
# Ubuntu: указать правильную версию
sudo update-alternatives --install /usr/bin/clang-format clang-format \
    /usr/bin/clang-format-14 100

# macOS: добавить в PATH
export PATH="/usr/local/opt/llvm/bin:$PATH"
```

### CI падает на конкретной проверке

```bash
# Локально запустить ту же проверку
# Например, для clang-format:
find src include -name "*.cpp" -o -name "*.hpp" | \
    xargs clang-format --dry-run --Werror
```

### Пропустить pre-commit для экстренного коммита

```bash
# НЕ РЕКОМЕНДУЕТСЯ, только для экстренных случаев
git commit --no-verify -m "emergency fix"
```

## 📊 Badges для README

Добавьте в основной README.md:

```markdown
![CI](https://github.com/USERNAME/object-storage/workflows/CI/badge.svg)
![CodeQL](https://github.com/USERNAME/object-storage/workflows/CodeQL/badge.svg)
[![codecov](https://codecov.io/gh/USERNAME/object-storage/branch/main/graph/badge.svg)](https://codecov.io/gh/USERNAME/object-storage)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](https://github.com/pre-commit/pre-commit)
```

## 🔗 Полезные ссылки

- [Pre-commit Documentation](https://pre-commit.com/)
- [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [Clang-Tidy Checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Conventional Commits](https://www.conventionalcommits.org/)

## 🤝 Contribution Guidelines

При создании Pull Request убедитесь, что:

- ✅ Все pre-commit проверки проходят
- ✅ CI pipeline зеленый
- ✅ Code coverage не уменьшился
- ✅ Документация обновлена
- ✅ Commit messages следуют Conventional Commits

---

**Вопросы?** Создайте issue в репозитории.


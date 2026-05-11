# GitHub Actions Workflows

Автоматизация CI/CD для Object Storage Console.

## 📋 Workflows

### 1. Backend CI (`backend-ci.yml`)

Сборка и тестирование C++ backend приложения.

**Триггеры:**

- Push в `main`, `dev`
- Pull requests в `main`, `dev`
- Изменения в `src/`, `include/`, `tests/`, `CMakeLists.txt`, `vcpkg.json`

**Шаги:**

1. ✅ Установка системных зависимостей (libsqlite3, openssl, uuid, zlib)
2. ✅ Настройка vcpkg с кэшированием
3. ✅ Конфигурация CMake с Ninja
4. ✅ Сборка проекта (Release)
5. ✅ Запуск unit тестов
6. ⚠️ Запуск integration тестов (опционально)
7. 📊 Публикация результатов тестов
8. 📦 Upload артефактов (console_server + конфиги)

**Timeout:** 60 минут

---

### 2. Frontend CI (`frontend-ci.yml`)

Сборка и проверка React frontend приложения.

**Триггеры:**

- Push в `main`, `dev`
- Pull requests в `main`, `dev`
- Изменения в `frontend/`

**Шаги:**

1. ✅ Настройка Node.js 18 с npm кэшем
2. ✅ Установка зависимостей (`npm ci`)
3. 🔒 Security audit (npm audit)
4. 🎨 Линтинг (eslint)
5. 🔍 Type checking (TypeScript)
6. 🏗️ Production build
7. 📦 Upload build артефактов
8. 📊 Отчет о размере bundle

**Timeout:** 30 минут

---

### 3. Code Quality (`code-quality.yml`)

Проверка качества кода и конфигурационных файлов.

**Триггеры:**

- Push в `main`, `dev`
- Pull requests в `main`, `dev`

**Шаги:**

1. ✅ Pre-commit hooks (без cpplint и detect-secrets)
2. ✅ Валидация YAML файлов workflows
3. ✅ Проверка синтаксиса workflows (actionlint)

**Timeout:** 15 минут

---

## 🚀 Локальный запуск

### Backend тесты

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
cd build/tests && ./unit_tests
```

### Frontend проверки

```bash
cd frontend
npm ci
npm run lint
npm run type-check
npm run build
```

### Pre-commit hooks

```bash
pip install pre-commit
pre-commit install
pre-commit run --all-files
```

---

## 📊 Статус

| Workflow | Статус | Покрытие |
|----------|--------|----------|
| Backend CI | 🟢 Active | Unit: 87% (155/179) |
| Frontend CI | 🟢 Active | N/A |
| Code Quality | 🟢 Active | N/A |

---

## 🔧 Конфигурация

### Backend

- **Compiler:** GCC/Clang
- **Build System:** CMake + Ninja
- **Package Manager:** vcpkg
- **Test Framework:** Google Test

### Frontend

- **Runtime:** Node.js 18+
- **Package Manager:** npm 9+
- **Build Tool:** react-scripts (Create React App)
- **Linter:** ESLint
- **Type Checker:** TypeScript 5.3+

---

## 🐛 Troubleshooting

### vcpkg cache miss

Если сборка долгая, проверьте кэш vcpkg:

```yaml
key: ${{ runner.os }}-vcpkg-${{ hashFiles('vcpkg.json') }}
```

### Unit tests failing

Проверьте путь к executable:

```bash
cd build/tests
./unit_tests
```

### Frontend build warnings

ESLint warnings не блокируют build (`continue-on-error: true`).
Исправьте предупреждения локально перед push.

---

## 📝 Maintenance

- **Pre-commit config:** `.pre-commit-config.yaml`
- **vcpkg dependencies:** `vcpkg.json`
- **CMake config:** `CMakeLists.txt`
- **Node.js dependencies:** `frontend/package.json`

# Быстрое исправление pre-commit ошибок 🔧

## ✅ Что уже исправлено

- ✅ **Markdown** ошибки в IMPLEMENTATION_PLAN.md
- ✅ **detect-secrets** - добавлены `pragma: allowlist secret`
- ✅ **trailing-whitespace** и **end-of-file-fixer** - исправлено автоматически
- ✅ **clang-format** - применено автоматически

## 🚀 Быстрое исправление оставшихся проблем

### Вариант 1: Автоматическое исправление (Рекомендуется)

```bash
# Запустить скрипт автоматического исправления
./fix_cpplint.sh

# Проверить изменения
git diff

# Если все ОК, добавить в staging
git add .
```

### Вариант 2: Установить недостающие зависимости

```bash
# Установить PyYAML для cmake-format/cmake-lint
pip install pyyaml

# Переустановить pre-commit хуки
pre-commit clean
pre-commit install
```

### Вариант 3: Коммит с обходом хуков (Не рекомендуется)

```bash
git commit --no-verify -m "your commit message"
```

## 📋 Основные проблемы и решения

### 1. Copyright Headers

**Проблема:** Отсутствуют copyright headers в .cpp файлах

**Решение:** Скрипт `fix_cpplint.sh` добавит их автоматически

### 2. TODO Comments

**Проблема:** TODO без username

**Исправление вручную:**

```cpp
// TODO: Fix this  ❌
// TODO(Nice0Man): Fix this  ✅
```

### 3. Include Order

**Проблема:** Неправильный порядок #include

**Правильный порядок:**

```cpp
#include "YourFile.hpp"  // 1. Corresponding header

#include <cstdio>        // 2. C system headers

#include <algorithm>      // 3. C++ headers
#include <string>

#include <drogon/...>    // 4. External libraries
```

### 4. cmake-format/cmake-lint

**Проблема:** `ModuleNotFoundError: No module named 'yaml'`

**Решение:**

```bash
pip install pyyaml
```

**Или отключить в `.pre-commit-config.yaml`:**

```yaml
# Закомментировать:
# - id: cmake-format
# - id: cmake-lint
```

## 🎯 Проверка после исправлений

```bash
# Запустить все хуки
pre-commit run --all-files

# Только cpplint
pre-commit run cpplint --all-files

# После исправлений - коммит
git add .
git commit -m "fix: resolve pre-commit issues"
```

## 📚 Подробная документация

См. [FIXES.md](./FIXES.md) для детальных инструкций по каждой проблеме.

## 🆘 Быстрая помощь

**Если pre-commit блокирует коммит:**

```bash
# 1. Попробовать автофикс
./fix_cpplint.sh

# 2. Если не помогло - временно обойти
git commit --no-verify -m "WIP: fixes needed"

# 3. Потом исправить и зафиксировать
git add .
git commit --amend
```

---

**Важно:** Не злоупотребляйте `--no-verify`. Хуки помогают поддерживать качество кода! 🎯

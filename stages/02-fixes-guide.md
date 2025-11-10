# Исправление pre-commit ошибок

Этот документ содержит инструкции по исправлению всех проблем, найденных pre-commit хуками.

## ✅ Исправлено автоматически

Следующие проблемы были исправлены автоматически хуками:

1. **trailing-whitespace** - удалены пробелы в конце строк
2. **end-of-file-fixer** - добавлены переводы строк в конце файлов
3. **clang-format** - применено форматирование кода согласно `.clang-format`
4. **markdownlint** - исправлены проблемы markdown (нумерация списков, HTML элементы)
5. **detect-secrets** - добавлены `pragma: allowlist secret` комментарии для ложных срабатываний

## ❌ Требуют ручного исправления

### 1. cmake-format и cmake-lint (ModuleNotFoundError: yaml)

**Проблема:** Отсутствует модуль PyYAML

**Решение:**

```bash
# Установить PyYAML в pre-commit окружение
pip install pyyaml

# Или переустановить pre-commit с зависимостями
pre-commit clean
pre-commit install
```

**Альтернатива:** Отключить эти хуки в `.pre-commit-config.yaml`:

```yaml
# Закомментировать или удалить
# - id: cmake-format
# - id: cmake-lint
```

### 2. cpplint - Множественные ошибки стиля

#### 2.1 Отсутствие copyright headers

**Проблема:** `No copyright message found`

**Решение:** Добавить copyright header в начало каждого .cpp файла:

```cpp
// Copyright 2025 <Your Name or Organization>
// Licensed under AGPL-3.0

#include "..."
```

**Массовое исправление:**

```bash
# Создать header файл
cat > copyright_header.txt << 'EOF'
// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
EOF

# Применить ко всем cpp файлам
for file in src/**/*.cpp tests/**/*.cpp; do
    if ! head -n 1 "$file" | grep -q "Copyright"; then
        cat copyright_header.txt "$file" > "$file.tmp" && mv "$file.tmp" "$file"
    fi
done
```

#### 2.2 Include order (build/include_order)

**Проблема:** Неправильный порядок include директив

**Правильный порядок:**

1. Соответствующий header (.hpp для .cpp)
2. C system headers (`<cstdio>`, `<cstring>`, и т.д.)
3. C++ system headers (`<algorithm>`, `<string>`, и т.д.)
4. Other headers (сторонние библиотеки)

**Пример исправления:**

```cpp
// Неправильно
#include <string>
#include "MyClass.hpp"
#include <cstdio>

// Правильно
#include "MyClass.hpp"

#include <cstdio>

#include <string>
```

#### 2.3 TODO комментарии без username

**Проблема:** `Missing username in TODO`

**Решение:**

```cpp
// Неправильно
// TODO: Implement this

// Правильно
// TODO(username): Implement this
// или
// TODO(github.com/Nice0Man): Implement this
```

**Массовое исправление:**

```bash
# Заменить все TODO на TODO(username)
find src tests -name "*.cpp" -exec sed -i 's/\/\/ TODO:/\/\/ TODO(Nice0Man):/g' {} +
find src tests -name "*.hpp" -exec sed -i 's/\/\/ TODO:/\/\/ TODO(Nice0Man):/g' {} +
```

#### 2.4 Whitespace issues

**Проблема:** Недостаточно пробелов между кодом и комментариями

**Решение:**

```cpp
// Неправильно
int x = 5; // comment

// Правильно
int x = 5;  // comment (минимум 2 пробела)
```

#### 2.5 Namespace using-directives

**Проблема:** `Do not use namespace using-directives`

**Решение:**

```cpp
// Неправильно
using namespace std;
using namespace console;

// Правильно
using std::string;
using std::vector;
using console::models::User;

// Или без using вообще
std::string name;
console::models::User user;
```

#### 2.6 C++11 headers warning

**Проблема:** `<chrono> is an unapproved C++11 header`

**Решение:** Это предупреждение можно игнорировать в C++20 проекте. Добавить в `.clang-tidy`:

```yaml
Checks: '-build/c++11'
```

### 3. Быстрое исправление всех cpplint ошибок

Создайте скрипт `fix_cpplint.sh`:

```bash
#!/bin/bash

# Copyright headers
HEADER="// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
"

for file in $(find src tests -name "*.cpp" -o -name "*.hpp"); do
    if ! head -n 1 "$file" | grep -q "Copyright"; then
        echo "$HEADER$(cat $file)" > "$file"
    fi
done

# Fix TODO comments
find src tests -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's/\/\/ TODO:/\/\/ TODO(Nice0Man):/g'

# Fix comment spacing (minimum 2 spaces)
find src tests -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's/;[[:space:]]\/\//;  \/\//g'

echo "Fixed cpplint issues. Please review changes."
```

Запустить:

```bash
chmod +x fix_cpplint.sh
./fix_cpplint.sh
```

## 🔧 Рекомендации

### Отключение строгих проверок

Если хотите ослабить некоторые проверки, отредактируйте `.pre-commit-config.yaml`:

```yaml
repos:
  - repo: ...
    hooks:
      - id: cpplint
        args: ['--filter=-legal/copyright,-build/c++11,-whitespace/comments']
```

### Автоматическое форматирование перед коммитом

Добавьте в `.git/hooks/pre-commit`:

```bash
#!/bin/bash

# Auto-format code
clang-format -i $(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$')

# Add formatted files back to staging
git add $(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$')
```

## 📝 Проверка после исправлений

После применения исправлений, проверьте:

```bash
# Запустить все pre-commit хуки
pre-commit run --all-files

# Или только cpplint
pre-commit run cpplint --all-files

# Коммит с обходом хуков (не рекомендуется)
git commit --no-verify -m "your message"
```

## ✅ Чеклист перед коммитом

- [ ] Добавлены copyright headers во все новые файлы
- [ ] Исправлен порядок include директив
- [ ] TODO комментарии содержат username
- [ ] Код отформатирован clang-format
- [ ] Нет trailing whitespace
- [ ] Файлы заканчиваются переводом строки
- [ ] Markdown файлы проверены
- [ ] Нет real secrets в коде (fake secrets помечены pragma)

## 📚 Дополнительные ресурсы

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [cpplint documentation](https://github.com/cpplint/cpplint)
- [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [pre-commit documentation](https://pre-commit.com/)

---

**Последнее обновление:** 2025-11-10

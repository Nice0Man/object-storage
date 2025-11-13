# Security Improvements

## Обзор

Реализована безопасная система аутентификации с хешированием паролей и конфигурируемыми параметрами администратора по умолчанию.

## Реализованные улучшения

### 1. Безопасное хеширование паролей (PBKDF2-SHA256)

Создана утилита `PasswordHash` для криптографически безопасного хеширования паролей:

**Файлы:**
- `include/console/utils/PasswordHash.hpp`
- `src/utils/PasswordHash.cpp`

**Возможности:**
- **PBKDF2** (Password-Based Key Derivation Function 2)
- **SHA-256** в качестве хеш-функции
- **310,000 итераций** (рекомендация OWASP 2023)
- **16-байтовая соль** (128 бит)
- **Защита от timing attacks** (constant-time comparison)
- **Обратная совместимость** с plain-text паролями для миграции

**Формат хеша:**
```
$pbkdf2-sha256$<iterations>$<base64_salt>$<base64_hash>
```

**Пример использования:**
```cpp
// Хеширование пароля
String hashed = PasswordHash::hash("my_password");
// $pbkdf2-sha256$310000$randomSalt123...$hashValue456...

// Проверка пароля
bool valid = PasswordHash::verify("my_password", hashed);  // true
```

### 2. Конфигурируемый администратор по умолчанию

Удалены все захардкоженные учетные данные "minioadmin" из кода.

**Новая конфигурация** (`config.json`):
```json
{
  "default_admin": {
    "username": "admin",
    "password": "changeme",
    "account_name": "Administrator",
    "enabled": true
  }
}
```

**Переменные окружения:**
```bash
DEFAULT_ADMIN_USERNAME=admin
DEFAULT_ADMIN_PASSWORD=changeme
DEFAULT_ADMIN_ACCOUNT_NAME=Administrator
DEFAULT_ADMIN_ENABLED=true
```

**Измененные файлы:**
- `include/console/common/Types.hpp` - добавлена структура `DefaultAdminConfig`
- `include/console/common/Config.hpp` - добавлены методы для работы с default_admin
- `src/utils/Config.cpp` - загрузка конфигурации и переменных окружения
- `src/storage/DatabaseManager.cpp` - использование конфигурации вместо hardcoded значений

### 3. Аутентификация через базу данных

**AuthService обновлен:**
- Добавлена зависимость от `DatabaseManager`
- Метод `authenticate_with_minio()` теперь:
  1. Проверяет существование пользователя в БД
  2. Проверяет статус пользователя (active/disabled)
  3. Использует `PasswordHash::verify()` для безопасной проверки пароля
  4. Возвращает детальные ошибки (401 Unauthorized, 403 Forbidden, 500 Internal Error)

**AuthController обновлен:**
- Метод `validate_credentials()` использует:
  - DatabaseManager для поиска пользователей
  - PasswordHash для проверки паролей
  - Логирование всех попыток аутентификации

### 4. Пароли хешируются при создании пользователя

**DatabaseManager:**
- При создании дефолтного админа пароль автоматически хешируется
- Используется `PasswordHash::hash()` перед сохранением в БД
- Старый комментарий "In production, this should be hashed!" заменен на реальную реализацию

**Файл:** `src/storage/DatabaseManager.cpp`
```cpp
// Создание админа с хешированным паролем
admin.secret_key = utils::PasswordHash::hash(admin_config.password);
```

## Безопасность

### ✅ Реализовано

1. **Криптографически стойкое хеширование** - PBKDF2-SHA256 с 310K итерациями
2. **Уникальные соли** - каждый пароль хешируется с новой случайной солью
3. **Защита от timing attacks** - constant-time сравнение хешей
4. **Отсутствие hardcoded credentials** - все параметры конфигурируемы
5. **Безопасное логирование** - пароли никогда не логируются
6. **Детальные коды ошибок** - разделение 401/403 для диагностики
7. **Проверка статуса пользователя** - отключенные аккаунты не могут войти

### 🔒 Рекомендации для production

1. **Сменить дефолтные учетные данные:**
   ```bash
   DEFAULT_ADMIN_USERNAME=custom_admin
   DEFAULT_ADMIN_PASSWORD=strong_random_password_here
   ```

2. **Отключить создание дефолтного админа после первого запуска:**
   ```json
   {
     "default_admin": {
       "enabled": false
     }
   }
   ```

3. **Использовать переменные окружения** вместо config.json для секретов

4. **Включить SSL/TLS:**
   ```json
   {
     "server": {
       "enable_ssl": true,
       "ssl_cert": "/path/to/cert.pem",
       "ssl_key": "/path/to/key.pem"
     }
   }
   ```

5. **Настроить аудит:** Все попытки входа логируются

## Миграция существующих баз данных

Если в базе данных уже есть пользователи с plain-text паролями, система автоматически их обнаружит и выполнит plain-text сравнение (с предупреждением в логах).

**Для миграции на хешированные пароли:**
1. Создайте скрипт миграции, который:
   - Читает всех пользователей
   - Хеширует их пароли
   - Обновляет записи в БД

```cpp
// Пример миграции
for (auto& user : db_manager->list_users()) {
    if (!PasswordHash::is_hashed(user.secret_key)) {
        user.secret_key = PasswordHash::hash(user.secret_key);
        db_manager->update_user(user);
    }
}
```

## Производительность

- **Хеширование:** ~100-200ms на пароль (intentionally slow для защиты от brute-force)
- **Проверка:** ~100-200ms на пароль
- **Влияние на login:** Незначительное (происходит один раз при входе)
- **Влияние на API:** Нет (JWT токены используются после входа)

## Тестирование

```bash
# Тест хеширования
./tests/test_password_hash

# Тест аутентификации
./tests/test_auth_service

# Интеграционный тест
curl -X POST http://localhost:9090/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"changeme"}'
```

## Соответствие стандартам

- ✅ **OWASP Password Storage Cheat Sheet**
- ✅ **NIST SP 800-132** (PBKDF2 рекомендации)
- ✅ **CWE-759** (Use of a One-Way Hash without a Salt)
- ✅ **CWE-327** (Use of a Broken or Risky Cryptographic Algorithm)

## Changelog

### v1.1.0 (2025-01-10)

- ✨ Добавлено безопасное хеширование паролей (PBKDF2-SHA256)
- ✨ Добавлена конфигурация default_admin
- 🔒 Удалены все hardcoded учетные данные
- 🔒 Аутентификация через базу данных
- 🔒 Проверка статуса пользователя
- 📝 Улучшено логирование попыток входа
- 🐛 Исправлена уязвимость с plain-text паролями

---

**Автор:** Implementation based on OWASP and NIST recommendations  
**Дата:** 2025-01-10  
**Версия:** 1.1.0


# 📚 Документация Object Storage Browser

> Полное руководство по изучению современной C++ веб-разработки на примере Object Storage Console

## 📖 Содержание

### Введение

- **[00. Обзор проекта](00-overview.md)** - Общее понимание проекта, цели, технологический стек

### Архитектура

- **[01. Общая архитектура](01-architecture.md)** - High-level архитектура системы
- **[02. Backend Deep Dive](02-backend-deep-dive.md)** - Детальный разбор Go backend
- **[03. Frontend архитектура](03-frontend-architecture.md)** - React/TypeScript frontend

### Безопасность и доступ

- **[04. Аутентификация и авторизация](04-authentication-authorization.md)** - JWT, OAuth2, LDAP, policies

### Функциональность

- **[05. Функциональные модули](05-functional-modules.md)** - Buckets, Objects, Users, Configuration
- **[06. WebSocket архитектура](06-websocket-architecture.md)** - Real-time коммуникация

### Разработка и развертывание

- **[07. Сборка и развертывание](07-build-deployment.md)** - Build system, Docker, environment
- **[08. Тестирование](08-testing.md)** - Unit, Integration, E2E тесты

### API и интеграции

- **[09. API спецификация](09-api-specification.md)** - Swagger/OpenAPI детали
- **[11. Зависимости и интеграции](11-dependencies-integrations.md)** - Внешние сервисы и библиотеки

### Best Practices

- **[10. Паттерны и Best Practices](10-patterns-best-practices.md)** - Архитектурные паттерны, оптимизации

### Продвинутые темы

- **[12. Расширенные темы](12-advanced-topics.md)** - Subpath, Multi-tenancy, HA, Monitoring

### Практика

- **[13. Практические упражнения](13-practical-exercises.md)** - Hands-on guide, setup, debugging
- **[14. План обучения](14-learning-roadmap.md)** - Рекомендуемый порядок изучения

---

## 🚀 Быстрый старт

### Для новичков

Начните с:

1. [00-overview.md](00-overview.md) - понимание целей проекта
2. [01-architecture.md](01-architecture.md) - общая картина
3. [13-practical-exercises.md](13-practical-exercises.md) - запуск локально

### Для Backend разработчиков

Фокус на:

- [02-backend-deep-dive.md](02-backend-deep-dive.md)
- [04-authentication-authorization.md](04-authentication-authorization.md)
- [08-testing.md](08-testing.md)

### Для Frontend разработчиков

Фокус на:

- [03-frontend-architecture.md](03-frontend-architecture.md)
- [09-api-specification.md](09-api-specification.md)
- [06-websocket-architecture.md](06-websocket-architecture.md)

### Для DevOps

Фокус на:

- [07-build-deployment.md](07-build-deployment.md)
- [12-advanced-topics.md](12-advanced-topics.md)

---

## 📊 Статистика проекта

- **Язык Backend**: C++20 (GCC 11+, Clang 14+)
- **Web Framework**: Drogon / Oat++ / Crow
- **Язык Frontend**: TypeScript + React 18.3.1
- **Всего файлов моделей**: 153+
- **Строк в Swagger**: 2858
- **Тестовых сценариев**: 20+
- **Build System**: CMake 3.20+
- **Package Manager**: vcpkg / Conan
- **Testing**: Google Test, Catch2
- **Лицензия**: AGPL-3.0-or-later

---

## 🤝 Вклад

Эта документация создана для изучения современной C++ веб-разработки.
Проект демонстрирует best practices при создании высокопроизводительных веб-приложений на C++20.

**Основные технологии:**

- C++20 (coroutines, concepts, ranges)
- Drogon Web Framework
- CMake build system
- vcpkg package management
- Google Test для тестирования

---

**Автор документации**: Техническая документация на основе архитектурного анализа
**Дата создания**: 2025-11-10
**Цель**: Обучение современной C++ веб-разработке
**Уровень**: От intermediate до advanced C++

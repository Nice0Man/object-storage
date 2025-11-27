# 00. Обзор проекта Object Storage Console

## 🎯 Что это такое?

**Object Storage Console** - это графический веб-интерфейс для управления объектным хранилищем Object Storage Server. Проект является community-maintained решением для управления S3-совместимым хранилищем.

## 📜 История и мотивация

### Предыстория

Object Storage - это high-performance объектное хранилище, совместимое с Amazon S3 API. Проект создан для предоставления удобного веб-интерфейса для управления объектным хранилищем.

### Цель проекта
>
> "Открытое, функциональное и производительное решение для управления объектным хранилищем."

Цель проекта:

- Предоставить полностью открытое, полностью функциональное решение
- Следовать принципам минимализма, производительности и свободы
- Поддерживать community-driven подход к развитию

## 🏗️ Что предоставляет проект?

### Основной функционал

1. **Web UI для Server** - управление через браузер
2. **Object Browser** - просмотр, загрузка, скачивание файлов
3. **Bucket Management** - создание, настройка, удаление buckets
4. **User & Group Management** - управление доступом
5. **Policy Management** - IAM политики безопасности
6. **Monitoring & Logging** - мониторинг состояния системы
7. **Site Replication** - репликация между несколькими сайтами
8. **Encryption & Compliance** - WORM, encryption, retention

### Целевая аудитория

- **DevOps инженеры** - развертывание и управление хранилищем
- **Backend разработчики** - интеграция с S3-совместимым API
- **Data Engineers** - работа с большими объемами данных
- **System Administrators** - администрирование Object Storage кластеров
- **Security Engineers** - настройка политик безопасности

## 🛠️ Технологический стек

### Backend

```
Язык: C++20
Основные библиотеки:
  - Drogon - Высокопроизводительный веб-фреймворк
  - nlohmann/json - JSON processing
  - jwt-cpp - JWT authentication
  - spdlog - Асинхронное логирование
  - OpenSSL - Криптография и TLS
  - WebSocket++ - Real-time communication
  - Boost.Asio - Асинхронный I/O
```

### Frontend

```
Язык: TypeScript
Framework: React 18.3.1
State Management: Redux Toolkit
UI Library: MDS (Object Storage Design System)
Routing: React Router 6.29.0
HTTP Client: Superagent
```

### API

```
Спецификация: OpenAPI 2.0 (Swagger)
Формат: JSON
Протоколы: HTTP/HTTPS, WebSocket
Аутентификация: JWT Bearer tokens
```

### Build & Deploy

```
Build System: CMake 3.20+
Package Manager (Backend): vcpkg / Conan
Package Manager (Frontend): Yarn 4.4.0
Контейнеризация: Docker
Testing: Google Test, Catch2, Playwright, Jest
Компилятор: GCC 11+, Clang 14+, MSVC 2022
```

## 📦 Структура проекта (High-Level)

```
openmaxio-object-browser/
│
├── cmd/console/          # 🎯 Entry point приложения
│   ├── main.go          # CLI setup
│   ├── server.go        # HTTP server
│   └── app_commands.go  # Command registry
│
├── api/                  # 🔌 Backend API handlers
│   ├── configure_console.go  # Main configuration
│   ├── client*.go       # Object Storage client wrappers
│   ├── user_*.go        # User domain logic
│   ├── admin_*.go       # Admin operations
│   └── operations/      # Generated API handlers
│
├── models/              # 📊 Data models (153+ files)
│   └── *.go            # Generated from swagger.yml
│
├── pkg/                 # 🔧 Internal packages
│   ├── auth/           # Authentication system
│   ├── certs/          # TLS certificate management
│   ├── logger/         # Structured logging
│   └── utils/          # Utilities
│
├── web-app/             # ⚛️ React frontend
│   ├── src/
│   │   ├── api/        # Generated TypeScript API
│   │   ├── screens/    # Page components
│   │   ├── common/     # Shared components
│   │   └── store.ts    # Redux store
│   └── build/          # Production build output
│
├── integration/         # 🧪 Integration tests
├── replication/         # 🔄 Replication tests
├── sso-integration/     # 🔐 SSO/LDAP tests
│
├── swagger.yml          # 📋 API specification (2858 lines)
├── Makefile            # 🔨 Build automation
└── Dockerfile          # 🐳 Container image
```

## 📊 Ключевые метрики

### Размер кодовой базы

- **Backend (Go)**: ~50+ файлов в `api/`, ~20 packages в `pkg/`
- **Models**: 153+ сгенерированных моделей
- **Frontend (TS/React)**: Полноценное SPA приложение
- **API Endpoints**: 100+ endpoints в swagger.yml
- **Тесты**: Unit, Integration, E2E, Replication, SSO

### Функциональная сложность

- **Методы аутентификации**: 4 (Form, OAuth2, LDAP, Anonymous)
- **Типы нотификаций**: 10+ (Webhook, AMQP, Redis, Kafka, etc.)
- **Операции с объектами**: 20+ (Upload, Download, Copy, Move, Tag, etc.)
- **Административные функции**: 50+ (Users, Groups, Policies, Config, etc.)

## 🎓 Что вы узнаете, изучив этот проект?

### Backend разработка на C++

- ✅ Современный C++20 (concepts, coroutines, ranges)
- ✅ REST API design с Swagger/OpenAPI
- ✅ Асинхронное программирование (Boost.Asio, coroutines)
- ✅ JWT authentication & authorization
- ✅ WebSocket для real-time коммуникации
- ✅ Работа с S3-совместимыми хранилищами
- ✅ Высокопроизводительные веб-серверы (Drogon, Oat++)
- ✅ RAII patterns и умные указатели
- ✅ Template metaprogramming
- ✅ Testing (Google Test, Catch2, mocking)

### Frontend разработка

- ✅ React + TypeScript production app
- ✅ Redux state management
- ✅ React Router для SPA
- ✅ API client generation из Swagger
- ✅ WebSocket integration
- ✅ Virtual scrolling для больших списков
- ✅ File upload/download с progress
- ✅ Component-based architecture

### DevOps & Infrastructure

- ✅ Docker контейнеризация
- ✅ Multi-stage builds
- ✅ TLS/SSL configuration
- ✅ Environment-based configuration
- ✅ Health checks & monitoring
- ✅ Reverse proxy integration (subpath support)
- ✅ High availability patterns

### Security

- ✅ OAuth2/OIDC integration
- ✅ LDAP/Active Directory
- ✅ IAM policies (AWS-compatible)
- ✅ Encryption (at rest, in transit)
- ✅ WORM (Write Once Read Many)
- ✅ Audit logging

### Software Architecture

- ✅ Clean Architecture principles
- ✅ Monorepo structure (backend + frontend)
- ✅ API-first development
- ✅ Code generation strategies
- ✅ Dependency injection
- ✅ Event-driven architecture

## 🔗 Связь с Object Storage экосистемой

### Object Storage Server

**Object Storage Console** - это UI для управления **Object Storage Server**. Они работают как отдельные процессы:

```
┌─────────────────┐         HTTP/S          ┌─────────────────┐
│                 │ ◄─────────────────────► │                 │
│     Server      │      S3 API Calls       │  Object Browser │
│   (Port 9000)   │                         │   (Port 9090)   │
│                 │                         │                 │
└─────────────────┘                         └─────────────────┘
        │                                            │
        │                                            │
        ▼                                            ▼
  Object Storage                             Web UI (Browser)
  (Buckets/Objects)                          User Management
```

### Object Storage Client (mc)

Object Browser использует библиотеку `github.com/object storage/mc/cmd` для некоторых операций, что обеспечивает консистентность с CLI инструментом `mc`.

### KES (Key Encryption Service)

Для enterprise-grade encryption интегрируется с Object Storage KES через `github.com/object storage/kes`.

## 📄 Лицензия

**AGPL-3.0-or-later** (GNU Affero General Public License v3.0)

### Что это означает?

- ✅ Свободное использование
- ✅ Свободное изменение
- ✅ Свободное распространение
- ⚠️ Copyleft: производные работы должны быть под той же лицензией
- ⚠️ Network use = distribution: если вы предоставляете доступ через сеть, вы должны открыть исходный код

## 🚀 Кто использует?

### Use Cases

1. **Private Cloud Storage** - альтернатива S3 для on-premise
2. **Backup & Archive** - долгосрочное хранение данных
3. **Data Lakes** - хранилище для аналитики и ML
4. **Media Storage** - видео, изображения, статический контент
5. **Application Storage** - backend для приложений
6. **Disaster Recovery** - geo-репликация для DR

### Типы организаций

- Компании с требованиями к data sovereignty
- Финансовые организации (compliance)
- Healthcare (HIPAA)
- Образовательные учреждения
- SaaS провайдеры
- IoT платформы

## 🎯 Следующие шаги

После прочтения этого обзора переходите к:

1. **[01-architecture.md](01-architecture.md)** - понять общую архитектуру
2. **[13-practical-exercises.md](13-practical-exercises.md)** - запустить локально
3. **[14-learning-roadmap.md](14-learning-roadmap.md)** - план обучения

---

**Совет**: Держите этот обзор в голове, когда будете погружаться в детали архитектуры. Понимание "зачем" помогает лучше понять "как".

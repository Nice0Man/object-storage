# 01. Общая архитектура системы

## 🏛️ High-Level архитектура

### Компонентная диаграмма

```
┌──────────────────────────────────────────────────────────────────┐
│                         USER BROWSER                              │
│                    (Chrome, Firefox, Safari)                      │
└────────────────────────┬─────────────────────────────────────────┘
                         │ HTTPS
                         │ Port 9090/9443
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│              OPENMAXIO OBJECT BROWSER (Console)                   │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │                    Static Web UI (React)                     │ │
│ │  - Embedded in Go binary                                     │ │
│ │  - Served via HTTP handler                                   │ │
│ │  - SPA with React Router                                     │ │
│ └──────────────────────────────────────────────────────────────┘ │
│                              │                                    │
│                              │ /api/v1/*                          │
│                              ▼                                    │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │                   API Layer (Go)                             │ │
│ │  ┌────────────┐  ┌────────────┐  ┌────────────┐            │ │
│ │  │   Auth     │  │  Buckets   │  │  Objects   │  ...       │ │
│ │  │ Middleware │  │  Handlers  │  │  Handlers  │            │ │
│ │  └────────────┘  └────────────┘  └────────────┘            │ │
│ │                                                               │ │
│ │  Generated from swagger.yml via go-openapi                   │ │
│ └──────────────────────────────────────────────────────────────┘ │
│                              │                                    │
│                              │ MinIO SDK                          │
│                              ▼                                    │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │              MinIO Client Layer                              │ │
│ │  - minio-go/v7 (S3 API)                                      │ │
│ │  - madmin-go/v3 (Admin API)                                  │ │
│ └──────────────────────────────────────────────────────────────┘ │
└────────────────────────┬─────────────────────────────────────────┘
                         │ HTTP/HTTPS
                         │ Port 9000
                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                      MinIO Server                                 │
│  - Actual object storage                                          │
│  - S3-compatible API                                              │
│  - Admin API                                                      │
│  - Distributed/Standalone mode                                    │
└────────────────────────┬─────────────────────────────────────────┘
                         │
                         ▼
                  ┌──────────────┐
                  │   Storage    │
                  │  (Disk/SSD)  │
                  └──────────────┘
```

## 🎨 Архитектурный стиль

### Monolithic SPA + API

Проект использует архитектуру **Monolithic SPA** с четким разделением:

- **Backend**: Stateless REST API + WebSocket
- **Frontend**: Standalone SPA (React)
- **Deployment**: Single binary с embedded UI

### Преимущества данного подхода

✅ **Простота развертывания** - один бинарник
✅ **Консистентность** - UI всегда совместим с API
✅ **Производительность** - нет network overhead между UI и API
✅ **Версионирование** - единая версия проекта

## 🔄 Request Flow

### Типичный HTTP Request

```
1. User Action (Browser)
   │
   ├─► [React Component]
   │      │
   │      └─► dispatch(action)
   │             │
   │             └─► Redux Thunk/Action
   │                    │
   │                    └─► API Client (generated from Swagger)
   │                           │
   │                           └─► HTTP POST /api/v1/buckets
   │                                  │
   └──────────────────────────────────┘

2. Backend Processing
   │
   ├─► [HTTP Router] - Match route
   │      │
   │      └─► [Auth Middleware] - Validate JWT
   │             │
   │             └─► [Handler] - api/user_buckets.go
   │                    │
   │                    └─► [MinIO Client] - minio.MakeBucket()
   │                           │
   │                           └─► [MinIO Server] - Create bucket
   │                                  │
   └──────────────────────────────────┘

3. Response
   │
   ├─► [MinIO Server] - Returns result
   │      │
   │      └─► [Handler] - Format response
   │             │
   │             └─► [HTTP Response] - JSON
   │                    │
   │                    └─► [API Client] - Parse response
   │                           │
   │                           └─► [Redux Store] - Update state
   │                                  │
   │                                  └─► [React Component] - Re-render
```

### WebSocket Flow (Real-time logs)

```
1. Connection Establishment
   Browser ─────► WS /ws/console ─────► Upgrade HTTP → WebSocket

2. Streaming
   MinIO Server ─────► Console Handler ─────► WebSocket ─────► Browser
   (Server events)      (Filter/Format)        (Push)         (Display)

3. Bidirectional
   Browser ─────► Subscribe to bucket events ─────► MinIO Server
```

## 📂 Слоистая архитектура

### Backend Layers

```
┌─────────────────────────────────────────────────────────┐
│ Layer 1: Entry Point                                    │
│  - cmd/console/main.go                                  │
│  - CLI argument parsing                                 │
│  - Application bootstrap                                │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 2: HTTP Server & Routing                          │
│  - api/configure_console.go                             │
│  - Middleware setup (auth, CORS, gzip)                  │
│  - Route registration                                   │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 3: API Handlers (Domain Logic)                    │
│  - api/user_*.go (user domain)                          │
│  - api/admin_*.go (admin domain)                        │
│  - api/public_*.go (public domain)                      │
│  - Business logic & validation                          │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 4: Client Abstraction                             │
│  - api/client.go (MinioClient interface)                │
│  - api/client-admin.go (AdminClient operations)         │
│  - Abstraction over MinIO SDK                           │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 5: External SDK                                   │
│  - github.com/minio/minio-go/v7                         │
│  - github.com/minio/madmin-go/v3                        │
│  - Direct communication with MinIO Server               │
└─────────────────────────────────────────────────────────┘
```

### Frontend Layers

```
┌─────────────────────────────────────────────────────────┐
│ Layer 1: View Components                                │
│  - web-app/src/screens/                                 │
│  - Presentational components                            │
│  - User interactions                                    │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 2: State Management                               │
│  - Redux store (store.ts)                               │
│  - Slices (systemSlice.ts)                              │
│  - Actions & Reducers                                   │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 3: API Client                                     │
│  - web-app/src/api/consoleApi.ts                        │
│  - Generated from swagger.yml                           │
│  - Type-safe HTTP calls                                 │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Layer 4: HTTP Client                                    │
│  - Superagent                                           │
│  - Request/Response interceptors                        │
│  - Error handling                                       │
└─────────────────────────────────────────────────────────┘
```

## 🔐 Security Architecture

### Authentication Flow

```
┌──────────┐                                           ┌────────────┐
│  Browser │                                           │   MinIO    │
└─────┬────┘                                           └──────┬─────┘
      │                                                        │
      │  1. POST /api/v1/login                               │
      │     { username, password }                            │
      ├────────────────────────────────────►                  │
      │                                     │                 │
      │                                     │  2. Validate    │
      │                                     │     credentials │
      │                                     ├────────────────►│
      │                                     │                 │
      │                                     │  3. Return STS  │
      │                                     │     tokens      │
      │                                     │◄────────────────┤
      │                                     │                 │
      │  4. Generate JWT                    │                 │
      │     Claims: {                       │                 │
      │       STSAccessKeyID,              │                 │
      │       STSSecretAccessKey,          │                 │
      │       exp                          │                 │
      │     }                               │                 │
      │                                     │                 │
      │  5. Set-Cookie: token=JWT          │                 │
      │◄────────────────────────────────────┤                 │
      │                                     │                 │
      │  6. Subsequent requests             │                 │
      │     Authorization: Bearer JWT       │                 │
      ├────────────────────────────────────►│                 │
      │                                     │                 │
      │                                     │  7. Parse JWT   │
      │                                     │     Extract STS │
      │                                     │     tokens      │
      │                                     │                 │
      │                                     │  8. Call MinIO  │
      │                                     │     with STS    │
      │                                     ├────────────────►│
      │                                     │                 │
```

### Authorization Layers

```
1. Network Layer
   ├─► TLS/SSL encryption
   └─► Certificate validation

2. Application Layer
   ├─► JWT validation (KeyAuth middleware)
   └─► Token expiration check

3. MinIO Layer
   ├─► STS credentials validation
   ├─► IAM policy evaluation
   └─► Resource-based permissions

4. Object Layer
   ├─► Bucket policies
   ├─► Object ACLs
   └─► Encryption keys
```

## 🔌 API Design

### RESTful Principles

```
Resource-oriented URLs:
  GET    /api/v1/buckets           - List all buckets
  POST   /api/v1/buckets           - Create bucket
  DELETE /api/v1/buckets/{name}    - Delete bucket
  GET    /api/v1/buckets/{name}/objects  - List objects

Stateless:
  - Each request contains all necessary information
  - JWT in Authorization header
  - No server-side sessions

Standard HTTP codes:
  200 - Success
  201 - Created
  204 - No Content
  400 - Bad Request
  401 - Unauthorized
  403 - Forbidden
  404 - Not Found
  500 - Server Error
```

### API Versioning

- **Path-based versioning**: `/api/v1/...`
- Allows multiple API versions simultaneously
- Easy migration path for clients

## 📦 Packaging & Deployment

### Build Artifact

```
console (binary)
│
├─► Go executable (~50MB)
│
├─► Embedded assets
│   └─► web-app/build/ (React production build)
│       ├─► index.html
│       ├─► static/js/
│       ├─► static/css/
│       └─► static/media/
│
└─► Build info
    ├─► Version
    ├─► Commit ID
    └─► Build time
```

### Runtime Dependencies

**Required:**

- MinIO Server (v1.0.0+)

**Optional:**

- KES (Key Encryption Service) - для enterprise encryption
- OpenLDAP / Active Directory - для LDAP auth
- Identity Provider (Dex, Okta, etc.) - для OAuth2/OIDC
- PostgreSQL / Redis / Kafka - для notification targets

### Deployment Topologies

#### 1. Standalone (Development)

```
┌─────────────────┐
│  Console Server │
│   (Port 9090)   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  MinIO Server   │
│   (Port 9000)   │
└─────────────────┘
```

#### 2. Production (Reverse Proxy)

```
┌─────────────┐
│   Browser   │
└──────┬──────┘
       │ HTTPS (443)
       ▼
┌─────────────────┐
│ Nginx/Traefik   │
│  Reverse Proxy  │
└────────┬────────┘
         │
         ├─► /console/* ──► Console (9090)
         │
         └─► /minio/*   ──► MinIO (9000)
```

#### 3. Kubernetes

```
┌─────────────────────────────────────┐
│         Kubernetes Cluster          │
│                                     │
│  ┌────────────┐    ┌────────────┐ │
│  │  Console   │    │   MinIO    │ │
│  │    Pod     │    │    Pod     │ │
│  └──────┬─────┘    └──────┬─────┘ │
│         │                  │       │
│  ┌──────▼──────────────────▼────┐ │
│  │      Ingress Controller      │ │
│  └──────────────────────────────┘ │
└─────────────────────────────────────┘
```

## 🔧 Configuration Architecture

### Configuration Sources (Priority order)

1. **Command-line flags** (highest priority)
2. **Environment variables**
3. **Config files** (future feature)
4. **Default values** (lowest priority)

### Key Configuration Areas

```go
// Server configuration
CONSOLE_PORT                 // HTTP port (default: 9090)
CONSOLE_TLS_PORT            // HTTPS port (default: 9443)
CONSOLE_HOSTNAME            // Server hostname

// MinIO connection
CONSOLE_MINIO_SERVER        // MinIO endpoint URL

// Security
CONSOLE_PBKDF_PASSPHRASE    // JWT encryption key
CONSOLE_PBKDF_SALT          // JWT salt
CONSOLE_TLS_REDIRECT        // Force HTTPS

// Certificates
CONSOLE_CERTS_DIR           // TLS certificate directory

// Features
CONSOLE_SUBPATH             // Subpath for reverse proxy
CONSOLE_DEV_MODE            // Development mode
CONSOLE_DEBUG_LOGLEVEL      // Debug level (0-6)

// LDAP (optional)
CONSOLE_LDAP_ENABLED        // Enable LDAP auth
```

## 🎯 Design Principles

### 1. Interface Segregation

```go
// Вместо одного большого интерфейса
type MinioClient interface {
    // Bucket operations
    listBucketsWithContext(...)
    makeBucketWithContext(...)

    // Object operations
    listObjects(...)
    putObject(...)

    // Admin operations
    // ... и т.д.
}

// Легко моккировать для тестов
type MockMinioClient struct {
    // Mock implementation
}
```

### 2. Dependency Injection

```go
// Handlers принимают dependencies
func getBucketHandler(client MinioClient) http.HandlerFunc {
    return func(w http.ResponseWriter, r *http.Request) {
        // Use client
    }
}
```

### 3. Single Responsibility

- Каждый handler отвечает за одну операцию
- Каждый package имеет четкую зону ответственности
- Модели отделены от бизнес-логики

### 4. Code Generation

- Models генерируются из Swagger → консистентность
- TypeScript API генерируется из Swagger → type safety
- Меньше ручного кода → меньше ошибок

## 📊 Data Flow

### Read Operation (Get Object)

```
User clicks "Download"
  → Redux action
  → API call /api/v1/buckets/{bucket}/objects/download?prefix={path}
  → Handler validates JWT
  → Extract STS credentials from JWT
  → Call MinIO with STS: GetObject(bucket, path)
  → MinIO returns presigned URL or stream
  → Handler returns to client
  → Browser downloads file
```

### Write Operation (Upload Object)

```
User drags file
  → React component captures file
  → Redux action with File object
  → API call POST /api/v1/buckets/{bucket}/objects/upload
  → Handler validates JWT
  → Multipart form parsing
  → Stream file to MinIO: PutObject(bucket, path, reader)
  → MinIO writes to disk
  → Return success response
  → Update Redux store
  → Re-render component (show success)
```

## 🔍 Observability

### Logging

```
pkg/logger/
  - Structured logging
  - Multiple log levels
  - Request/Response logging (debug level 1-6)
```

### Monitoring

- Health check endpoint: `/api/v1/health`
- Prometheus metrics (через MinIO Server)
- Real-time log streaming via WebSocket

### Profiling

- CPU profiling
- Memory profiling
- Goroutine profiling
- Block profiling

## 🚀 Следующие шаги

После понимания общей архитектуры переходите к:

- **[02-backend-deep-dive.md](02-backend-deep-dive.md)** - детальный разбор backend
- **[03-frontend-architecture.md](03-frontend-architecture.md)** - детальный разбор frontend
- **[04-authentication-authorization.md](04-authentication-authorization.md)** - система безопасности

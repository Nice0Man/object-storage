# 09. API спецификация (Swagger/OpenAPI)

## 📋 Обзор API

**Файл спецификации:** `swagger.yml` (2858 строк)

**Версия:** OpenAPI 2.0 (Swagger)
**Base Path:** `/api/v1`
**Протоколы:** `http`, `https`, `ws` (WebSocket)
**Формат:** JSON

## 🔐 Security Definitions

```yaml
securityDefinitions:
  key:
    type: oauth2
    flow: accessCode
    authorizationUrl: http://min.io
    tokenUrl: http://min.io
  anonymous:
    name: X-Anonymous
    in: header
    type: apiKey

security:
  - key: []  # Applied to all endpoints by default
```

**Исключения:** Login endpoints (security: [])

## 📚 API Categories

### 1. Authentication (`/auth`)

| Endpoint | Method | Description | Security |
|----------|--------|-------------|----------|
| `/login` | GET | Get login strategy (form/sso) | None |
| `/login` | POST | Login with credentials | None |
| `/login/oauth2/auth` | POST | OAuth2 callback | None |
| `/logout` | POST | Logout | Required |
| `/session` | GET | Check session validity | Required |

### 2. Buckets (`/bucket`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/buckets` | GET | List all buckets |
| `/buckets` | POST | Create bucket |
| `/buckets/{name}` | DELETE | Delete bucket |
| `/buckets/{name}` | GET | Get bucket info |
| `/buckets/{name}/policy` | GET/PUT/DELETE | Bucket policy |
| `/buckets/{name}/quota` | GET/PUT | Bucket quota |
| `/buckets/{name}/versioning` | GET/PUT | Versioning config |
| `/buckets/{name}/encryption` | GET/POST/DELETE | Encryption config |
| `/buckets/{name}/object-locking` | GET/POST | Object locking |
| `/buckets/{name}/replication` | GET/PUT/DELETE | Replication rules |
| `/buckets/{name}/tags` | GET/PUT/DELETE | Bucket tags |
| `/buckets/{name}/events` | GET/POST/DELETE | Event notifications |
| `/buckets/{name}/lifecycle` | GET/POST/PUT/DELETE | Lifecycle rules |
| `/buckets/{name}/retention` | GET/POST | Retention config |

### 3. Objects (`/object`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/buckets/{bucket}/objects` | GET | List objects |
| `/buckets/{bucket}/objects/upload` | POST | Upload object |
| `/buckets/{bucket}/objects/download` | GET | Download object |
| `/buckets/{bucket}/objects` | DELETE | Delete object |
| `/buckets/{bucket}/objects/delete-multiple` | POST | Delete multiple |
| `/buckets/{bucket}/objects/copy` | POST | Copy object |
| `/buckets/{bucket}/objects/restore` | POST | Restore from archive |
| `/buckets/{bucket}/objects/preview` | GET | Preview object |
| `/buckets/{bucket}/objects/share` | GET | Get presigned URL |
| `/buckets/{bucket}/objects/metadata` | PUT | Set metadata |
| `/buckets/{bucket}/objects/tags` | GET/PUT/DELETE | Object tags |
| `/buckets/{bucket}/objects/retention` | GET/PUT | Retention settings |
| `/buckets/{bucket}/objects/legal-hold` | GET/PUT | Legal hold |

### 4. Users (`/user`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/users` | GET | List users |
| `/users` | POST | Create user |
| `/users/{name}` | DELETE | Delete user |
| `/users/{name}` | GET | Get user info |
| `/users/{name}` | PUT | Update user |
| `/users/{name}/policies` | GET/PUT | User policies |
| `/users/{name}/groups` | GET/PUT | User groups |
| `/users/{name}/password` | PUT | Change password |
| `/users/{name}/service-accounts` | GET/POST | Service accounts |
| `/service-accounts/{accessKey}` | DELETE/PUT | Manage SA |

### 5. Groups (`/group`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/groups` | GET | List groups |
| `/groups` | POST | Create group |
| `/groups/{name}` | GET | Get group info |
| `/groups/{name}` | PUT | Update group |
| `/groups/{name}` | DELETE | Delete group |
| `/groups/{name}/policy` | PUT | Set group policy |

### 6. Policies (`/policy`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/policies` | GET | List policies |
| `/policies` | POST | Create policy |
| `/policies/{name}` | GET | Get policy |
| `/policies/{name}` | PUT | Update policy |
| `/policies/{name}` | DELETE | Delete policy |
| `/policies/{name}/entities` | GET | Get entities using policy |

### 7. Configuration (`/system`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/configs` | GET | List configurations |
| `/configs` | POST/PUT | Update configuration |
| `/configs/{name}` | GET | Get specific config |
| `/configs/{name}` | DELETE | Reset config |
| `/configs/export` | GET | Export all configs |
| `/configs/import` | POST | Import configs |
| `/notification-endpoints` | GET/POST | Notification targets |
| `/notification-endpoints/{name}` | GET/PUT/DELETE | Manage endpoint |

### 8. Administration (`/admin`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/admin/info` | GET | Server info |
| `/admin/health` | GET | Health check |
| `/admin/trace` | GET | Trace logs (WS) |
| `/admin/profiling/start` | POST | Start profiling |
| `/admin/profiling/stop` | POST | Stop profiling |
| `/admin/site-replication` | GET/POST | Site replication |
| `/admin/tier` | GET/POST | Tiering config |
| `/admin/inspect` | POST | Inspect object |

### 9. KMS (Key Management)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/kms/status` | GET | KMS status |
| `/kms/version` | GET | KMS version |
| `/kms/apis` | GET | Available APIs |
| `/kms/metrics` | GET | KMS metrics |
| `/kms/keys` | GET | List keys |
| `/kms/keys` | POST | Create key |
| `/kms/keys/{name}` | GET/DELETE | Manage key |

### 10. IDP (Identity Provider)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/idp/config` | GET | List IDP configs |
| `/idp/config` | POST | Add IDP |
| `/idp/config/{type}/{name}` | GET/PUT/DELETE | Manage IDP |

## 📝 Request/Response Examples

### Login

**Request:**

```http
POST /api/v1/login HTTP/1.1
Content-Type: application/json

{
  "accessKey": "minioadmin",
  "secretKey": "minioadmin"
}
```

**Response:**

```http
HTTP/1.1 204 No Content
Set-Cookie: token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...; Path=/; HttpOnly
```

### List Buckets

**Request:**

```http
GET /api/v1/buckets HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

**Response:**

```json
{
  "buckets": [
    {
      "name": "my-bucket",
      "creation_date": "2025-11-10T12:00:00Z",
      "size": 1048576,
      "objects": 10
    }
  ],
  "total": 1
}
```

### Create Bucket

**Request:**

```http
POST /api/v1/buckets HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Content-Type: application/json

{
  "name": "new-bucket",
  "region": "us-east-1",
  "locking": false,
  "versioning": {
    "enabled": true
  }
}
```

**Response:**

```json
{
  "bucketName": "new-bucket"
}
```

### Upload Object

**Request:**

```http
POST /api/v1/buckets/my-bucket/objects/upload?prefix=folder/file.txt HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary

------WebKitFormBoundary
Content-Disposition: form-data; name="file"; filename="file.txt"
Content-Type: text/plain

[file content]
------WebKitFormBoundary--
```

**Response:**

```http
HTTP/1.1 200 OK
```

### List Objects

**Request:**

```http
GET /api/v1/buckets/my-bucket/objects?prefix=folder/&recursive=true HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

**Response:**

```json
{
  "objects": [
    {
      "name": "folder/file.txt",
      "size": 1024,
      "last_modified": "2025-11-10T12:00:00Z",
      "content_type": "text/plain",
      "version_id": "null",
      "is_latest": true
    }
  ],
  "total": 1
}
```

### Get Presigned URL

**Request:**

```http
GET /api/v1/buckets/my-bucket/objects/share?prefix=file.txt&expires=3600 HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

**Response:**

```json
{
  "url": "http://localhost:9000/my-bucket/file.txt?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=...",
  "expires": "2025-11-10T13:00:00Z"
}
```

### Create User

**Request:**

```http
POST /api/v1/users HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Content-Type: application/json

{
  "accessKey": "newuser",
  "secretKey": "password123",
  "groups": ["developers"],
  "policies": ["readwrite"]
}
```

**Response:**

```http
HTTP/1.1 201 Created
```

### Create Policy

**Request:**

```http
POST /api/v1/policies HTTP/1.1
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Content-Type: application/json

{
  "name": "my-policy",
  "policy": {
    "Version": "2012-10-17",
    "Statement": [
      {
        "Effect": "Allow",
        "Action": ["s3:GetObject"],
        "Resource": ["arn:aws:s3:::my-bucket/*"]
      }
    ]
  }
}
```

**Response:**

```http
HTTP/1.1 201 Created
```

## 🔄 Pagination

**Query Parameters:**

```
?limit=20     # Items per page
?offset=0     # Starting offset
```

**Example:**

```http
GET /api/v1/users?limit=20&offset=40 HTTP/1.1
```

## 🔍 Filtering

**Example:**

```http
GET /api/v1/buckets/my-bucket/objects?prefix=folder/&recursive=true&with-versions=true
```

## ⚠️ Error Responses

### Error Model

```json
{
  "code": 404,
  "message": "Bucket not found",
  "detail": "The specified bucket does not exist"
}
```

### HTTP Status Codes

| Code | Meaning |
|------|---------|
| 200 | Success |
| 201 | Created |
| 204 | No Content (success, no body) |
| 400 | Bad Request (invalid input) |
| 401 | Unauthorized (invalid/missing token) |
| 403 | Forbidden (no permission) |
| 404 | Not Found |
| 409 | Conflict (resource exists) |
| 500 | Internal Server Error |
| 503 | Service Unavailable |

## 🛠️ Code Generation

### Backend (Go)

```bash
swagger generate server \
  -A console \
  --main-package=management \
  --server-package=api \
  -P models.Principal \
  -f ./swagger.yml \
  -r NOTICE
```

**Генерируется:**

- `models/*.go` - 153+ моделей
- `api/operations/` - Handler interfaces
- `api/server.go` - HTTP server setup

### Frontend (TypeScript)

```bash
npx swagger-typescript-api \
  -p ./swagger.yml \
  -o ./web-app/src/api \
  -n consoleApi.ts \
  --custom-config generator.config.js
```

**Генерируется:**

- `consoleApi.ts` - Type-safe API client
- Все модели с TypeScript types
- HTTP client wrapper

### generator.config.js

```javascript
module.exports = {
  codeGenConstructs: (struct) => {
    return {
      Keyword: {
        Number: "number",
        String: "string",
        Boolean: "boolean",
        Any: "any",
        Void: "void",
        Unknown: "unknown",
        Null: "null",
        Undefined: "undefined",
        Object: "object",
      },
    };
  },
  singleHttpClient: true,
  cleanOutput: false,
  enumNamesAsValues: false,
  moduleNameFirstTag: false,
  generateClient: true,
  generateRouteTypes: false,
  generateResponses: true,
  toJS: false,
  extractRequestParams: false,
  extractRequestBody: false,
  prettier: {
    printWidth: 120,
    tabWidth: 2,
    trailingComma: "all",
    parser: "typescript",
  },
};
```

## 📖 API Documentation

### Viewing Swagger UI

**Option 1:** Swagger Editor

```bash
# Open in browser
https://editor.swagger.io/

# Import swagger.yml
```

**Option 2:** Local Swagger UI

```bash
# Install swagger-ui
npm install -g swagger-ui-watcher

# Serve
swagger-ui-watcher swagger.yml
```

**Option 3:** ReDoc

```bash
npx @redocly/cli preview-docs swagger.yml
```

## 🔗 WebSocket Endpoints

### Console Logs

```yaml
/ws/console:
  get:
    summary: Stream console logs
    schemes: [ws]
    parameters:
      - name: token
        in: query
        type: string
```

**Usage:**

```javascript
const ws = new WebSocket('ws://localhost:9090/ws/console?token=...');
ws.onmessage = (event) => {
  const log = JSON.parse(event.data);
  console.log(log);
};
```

### Bucket Watch

```yaml
/ws/watch/{bucket}:
  get:
    summary: Watch bucket events
    schemes: [ws]
    parameters:
      - name: bucket
        in: path
        type: string
```

## 🎯 Следующие шаги

- **[10-patterns-best-practices.md](10-patterns-best-practices.md)** - API design patterns
- **[11-dependencies-integrations.md](11-dependencies-integrations.md)** - SDK integrations
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика с API

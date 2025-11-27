# 02. Backend Deep Dive (Go)

## 🎯 Структура Backend

### Основные компоненты

```
Backend Architecture:
├── Entry Point (cmd/console/)
├── API Layer (api/)
├── Models (models/)
├── Internal Packages (pkg/)
└── Tests (integration/, replication/)
```

## 📍 Entry Point: cmd/console/

### main.go - Точка входа

```go
package main

func main() {
    args := os.Args
    appName := filepath.Base(args[0])

    // Создание CLI приложения
    if err := newApp(appName).Run(args); err != nil {
        os.Exit(1)
    }
}

func newApp(name string) *cli.App {
    app := cli.NewApp()
    app.Name = name
    app.Version = pkg.Version + " - " + pkg.ShortCommitID
    app.Commands = commands  // Зарегистрированные команды

    return app
}
```

**Ключевые концепции:**

- Использует `github.com/object storage/cli` для CLI
- Command pattern для регистрации команд
- Version info из build-time constants

### app_commands.go - Регистрация команд

```go
var appCmds = []cli.Command{
    serverCmd,    // Основная команда для запуска сервера
    // Другие команды могут быть добавлены
}
```

### server.go - HTTP Server

```go
var serverCmd = cli.Command{
    Name:   "server",
    Usage:  "Start Object Storage Console server",
    Action: serverMain,
    Flags: []cli.Flag{
        cli.StringFlag{
            Name:  "host",
            Usage: "bind to a specific HOST",
        },
        cli.IntFlag{
            Name:  "port",
            Usage: "HTTP port",
        },
        // ... другие флаги
    },
}

func serverMain(ctx *cli.Context) error {
    // 1. Загрузка конфигурации
    // 2. Инициализация API
    // 3. Запуск HTTP сервера
    // 4. Graceful shutdown
}
```

## 🔌 API Layer: api/

### configure_console.go - Центральная конфигурация

**Структура файла (~565 строк):**

```go
// 1. Константы и переменные
const SubPath = "CONSOLE_SUBPATH"
var cfgSubPath = "/"

// 2. Command-line флаги
var additionalServerFlags = struct {
    CertsDir string `long:"certs-dir" description:"path to certs directory"`
}{}

// 3. API Configuration
func configureAPI(api *operations.ConsoleAPI) http.Handler {
    // Настройка аутентификации
    api.KeyAuth = func(token string, scopes []string) (*models.Principal, error) {
        // JWT validation
        claims, err := auth.ParseClaimsFromToken(token)
        if err != nil {
            return nil, errors.New(401, "incorrect api key auth")
        }
        return &models.Principal{
            STSAccessKeyID: claims.STSAccessKeyID,
            STSSecretAccessKey: claims.STSSecretAccessKey,
            // ...
        }, nil
    }

    // Регистрация handlers для каждого endpoint
    registerAuthHandlers(api)
    registerBucketHandlers(api)
    registerObjectHandlers(api)
    registerUserHandlers(api)
    registerAdminHandlers(api)
    // ...

    // Middleware chain setup
    handler := setupGlobalMiddleware(api.Serve(setupMiddlewares))

    return handler
}

// 4. Middleware setup
func setupGlobalMiddleware(handler http.Handler) http.Handler {
    // CORS
    // Compression (gzip)
    // Security headers
    // Static files serving
    // Custom error handling
}
```

**Ключевые функции:**

#### Authentication Middleware

```go
api.KeyAuth = func(token string, scopes []string) (*models.Principal, error) {
    if token == "Anonymous" {
        return &models.Principal{}, nil  // Anonymous access
    }

    // Parse JWT и извлечь claims
    claims, err := auth.ParseClaimsFromToken(token)
    if err != nil {
        return nil, errors.New(401, "incorrect api key auth")
    }

    // Return principal with STS credentials
    return &models.Principal{
        STSAccessKeyID:     claims.STSAccessKeyID,
        STSSecretAccessKey: claims.STSSecretAccessKey,
        STSSessionToken:    claims.STSSessionToken,
        AccountAccessKey:   claims.AccountAccessKey,
        // ...
    }, nil
}
```

#### Handler Registration Pattern

```go
func registerBucketHandlers(api *operations.ConsoleAPI) {
    // List buckets
    api.BucketListBucketsHandler = bucket.ListBucketsHandlerFunc(
        func(params bucket.ListBucketsParams, principal *models.Principal) middleware.Responder {
            return middleware.ResponderFunc(func(w http.ResponseWriter, _ runtime.Producer) {
                listBucketsResponse, err := getListBucketsResponse(session, params)
                if err != nil {
                    // Error handling
                }
                // Success response
            })
        },
    )

    // Make bucket
    api.BucketMakeBucketHandler = bucket.MakeBucketHandlerFunc(...)

    // Delete bucket
    api.BucketDeleteBucketHandler = bucket.DeleteBucketHandlerFunc(...)

    // ... и т.д.
}
```

### Паттерн Handler Function

**Типичная структура handler:**

```go
func getListBucketsResponse(session *models.Principal, params bucket.ListBucketsParams) (*models.ListBucketsResponse, error) {
    // 1. Create context with timeout
    ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
    defer cancel()

    // 2. Create Object Storage client from principal credentials
    mClient, err := newMinioClient(session)
    if err != nil {
        return nil, err
    }

    // 3. Call Object Storage operation
    buckets, err := mClient.listBucketsWithContext(ctx)
    if err != nil {
        return nil, err
    }

    // 4. Transform data to API model
    var bucketList []*models.Bucket
    for _, bucket := range buckets {
        bucketList = append(bucketList, &models.Bucket{
            Name:         swag.String(bucket.Name),
            CreationDate: bucket.CreationDate.String(),
        })
    }

    // 5. Return response
    return &models.ListBucketsResponse{
        Buckets: bucketList,
        Total:   int64(len(bucketList)),
    }, nil
}
```

## 🗂️ API Files Organization

### user_*.go - User Domain

| Файл | Назначение |
|------|-----------|
| `user_login.go` | Login endpoint (POST /login) |
| `user_logout.go` | Logout endpoint (POST /logout) |
| `user_session.go` | Session validation (GET /session) |
| `user_buckets.go` | Bucket CRUD operations |
| `user_objects.go` | Object operations (upload, download, delete) |
| `user_bucket_quota.go` | Bucket quota management |
| `user_watch.go` | Bucket event watching (WebSocket) |

### admin_*.go - Admin Domain

| Файл | Назначение |
|------|-----------|
| `admin_objects.go` | Admin object operations |
| Другие admin файлы | User/Group/Policy management, Config |

### public_*.go - Public Domain

| Файл | Назначение |
|------|-----------|
| `public_objects.go` | Public/shared object access |

### ws_*.go - WebSocket Handlers

| Файл | Назначение |
|------|-----------|
| `ws_handle.go` | WebSocket handler registry |
| `ws_objects.go` | Object streaming via WebSocket |

## 🔧 Client Layer: api/client.go

### MinioClient Interface

**Определение интерфейса (80+ методов):**

```go
type MinioClient interface {
    // Bucket operations
    listBucketsWithContext(ctx context.Context) ([]object storage.BucketInfo, error)
    makeBucketWithContext(ctx context.Context, bucketName, location string, objectLocking bool) error
    removeBucket(ctx context.Context, bucketName string) error
    setBucketPolicyWithContext(ctx context.Context, bucketName, policy string) error
    getBucketPolicy(ctx context.Context, bucketName string) (string, error)

    // Object operations
    listObjects(ctx context.Context, bucket string, opts object storage.ListObjectsOptions) <-chan object storage.ObjectInfo
    putObject(ctx context.Context, bucketName, objectName string, reader io.Reader, objectSize int64, opts object storage.PutObjectOptions) (object storage.UploadInfo, error)
    getObject(ctx context.Context, bucketName, objectName string, opts object storage.GetObjectOptions) (*object storage.Object, error)
    removeObject(ctx context.Context, bucketName, objectName string, opts object storage.RemoveObjectOptions) error
    statObject(ctx context.Context, bucketName, prefix string, opts object storage.GetObjectOptions) (object storage.ObjectInfo, error)

    // Encryption
    setBucketEncryption(ctx context.Context, bucketName string, config *sse.Configuration) error
    getBucketEncryption(ctx context.Context, bucketName string) (*sse.Configuration, error)
    removeBucketEncryption(ctx context.Context, bucketName string) error

    // Tagging
    putObjectTagging(ctx context.Context, bucketName, objectName string, otags *tags.Tags, opts object storage.PutObjectTaggingOptions) error
    getObjectTagging(ctx context.Context, bucketName, objectName string, opts object storage.GetObjectTaggingOptions) (*tags.Tags, error)

    // Retention & Legal Hold
    getObjectRetention(ctx context.Context, bucketName, objectName, versionID string) (*object storage.RetentionMode, *time.Time, error)
    putObjectRetention(ctx context.Context, bucketName, objectName string, opts object storage.PutObjectRetentionOptions) error
    getObjectLegalHold(ctx context.Context, bucketName, objectName string, opts object storage.GetObjectLegalHoldOptions) (*object storage.LegalHoldStatus, error)
    putObjectLegalHold(ctx context.Context, bucketName, objectName string, opts object storage.PutObjectLegalHoldOptions) error

    // Versioning
    getBucketVersioning(ctx context.Context, bucketName string) (object storage.BucketVersioningConfiguration, error)
    setBucketVersioning(ctx context.Context, bucketName string, config object storage.BucketVersioningConfiguration) error

    // Replication
    getBucketReplication(ctx context.Context, bucketName string) (replication.Config, error)
    setBucketReplication(ctx context.Context, bucketName string, cfg replication.Config) error
    removeBucketReplication(ctx context.Context, bucketName string) error

    // Notifications
    getBucketNotification(ctx context.Context, bucketName string) (notification.Configuration, error)
    setBucketNotification(ctx context.Context, bucketName string, config notification.Configuration) error
    removeAllBucketNotification(ctx context.Context, bucketName string) error
    listenBucketNotification(ctx context.Context, bucketName string, prefix, suffix string, events []string) <-chan notification.Info

    // ... и много других методов
}
```

### Реализация MinioClient

```go
// minioClient implements MinioClient interface
type minioClient struct {
    client *object storage.Client  // Actual Object Storage SDK client
}

func (c minioClient) listBucketsWithContext(ctx context.Context) ([]object storage.BucketInfo, error) {
    return c.client.ListBuckets(ctx)
}

func (c minioClient) makeBucketWithContext(ctx context.Context, bucketName, location string, objectLocking bool) error {
    return c.client.MakeBucket(ctx, bucketName, object storage.MakeBucketOptions{
        Region:        location,
        ObjectLocking: objectLocking,
    })
}

// ... остальные методы
```

### Создание клиента

```go
func newMinioClient(session *models.Principal) (*minioClient, error) {
    // Endpoint из конфигурации
    endpoint := getObject StorageServer()

    // Credentials из JWT Principal
    creds := credentials.NewStaticV4(
        session.STSAccessKeyID,
        session.STSSecretAccessKey,
        session.STSSessionToken,
    )

    // Создание Object Storage client
    client, err := object storage.New(endpoint, &object storage.Options{
        Creds:  creds,
        Secure: getObject StorageEndpointIsSecure(),
    })
    if err != nil {
        return nil, err
    }

    // Disable retries (fail fast)
    object storage.MaxRetry = 1

    return &minioClient{client: client}, nil
}
```

## 🏢 Admin Client: api/client-admin.go

### AdminClient Interface

```go
type AdminClient interface {
    // User Management
    addUser(ctx context.Context, accessKey, secretKey string) error
    removeUser(ctx context.Context, accessKey string) error
    getUserInfo(ctx context.Context, accessKey string) (madmin.UserInfo, error)
    setUserStatus(ctx context.Context, accessKey string, status madmin.AccountStatus) error
    listUsers(ctx context.Context) (map[string]madmin.UserInfo, error)

    // Group Management
    updateGroupMembers(ctx context.Context, req madmin.GroupAddRemove) error
    getGroupDescription(ctx context.Context, group string) (*madmin.GroupDesc, error)
    listGroups(ctx context.Context) ([]string, error)

    // Policy Management
    addCannedPolicy(ctx context.Context, policyName, policy string) error
    removeCannedPolicy(ctx context.Context, policyName string) error
    listCannedPolicies(ctx context.Context) (map[string]json.RawMessage, error)
    getPolicy(ctx context.Context, policyName string) (string, error)
    setPolicy(ctx context.Context, policyName, entityName string, isGroup bool) error

    // Service Account Management
    addServiceAccount(ctx context.Context, policy *iampolicy.Policy, user string, accessKey string, secretKey string) (madmin.Credentials, error)
    listServiceAccounts(ctx context.Context, user string) (madmin.ListServiceAccountsResp, error)
    deleteServiceAccount(ctx context.Context, serviceAccount string) error

    // Server Info & Stats
    serverInfo(ctx context.Context) (madmin.InfoMessage, error)
    accountInfo(ctx context.Context) (madmin.AccountInfo, error)
    healthInfo(ctx context.Context, healthDataTypes []madmin.HealthDataType, deadline time.Duration) (*madmin.HealthInfo, string, error)

    // Configuration
    getConfig(ctx context.Context) ([]byte, error)
    setConfig(ctx context.Context, config io.Reader) (restart bool, err error)

    // Logging & Profiling
    startProfiling(ctx context.Context, profiler madmin.ProfilerType) ([]madmin.StartProfilingResult, error)
    stopProfiling(ctx context.Context) (io.ReadCloser, error)
    serverTrace(ctx context.Context, tracingOptions madmin.ServiceTraceOpts) <-chan madmin.ServiceTraceInfo

    // KMS (Key Management Service)
    kmsStatus(ctx context.Context) (madmin.KMSStatus, error)
    kmsAPIs(ctx context.Context) ([]madmin.KMSAPI, error)
    kmsVersion(ctx context.Context) (*madmin.KMSVersion, error)

    // Site Replication
    getSiteReplicationInfo(ctx context.Context) (*madmin.SiteReplicationInfo, error)
    addSiteReplicationInfo(ctx context.Context, sites []madmin.PeerSite) (*madmin.ReplicateAddStatus, error)

    // ... и много других
}
```

## 📊 Models: models/

### Генерация моделей

Все модели генерируются из `swagger.yml`:

```bash
swagger generate server \
  -A console \
  --main-package=management \
  --server-package=api \
  -P models.Principal \
  -f ./swagger.yml
```

**Результат:** 153+ файла в `models/`

### Примеры моделей

#### Bucket Model

```go
// models/bucket.go
type Bucket struct {
    // Name
    Name *string `json:"name,omitempty"`

    // Creation date
    CreationDate string `json:"creation_date,omitempty"`

    // Size in bytes
    Size int64 `json:"size,omitempty"`

    // Objects count
    Objects int64 `json:"objects,omitempty"`

    // Access type
    Access BucketAccess `json:"access,omitempty"`
}
```

#### LoginRequest Model

```go
// models/login_request.go
type LoginRequest struct {
    // Access key
    // Required: true
    AccessKey *string `json:"accessKey"`

    // Secret key
    // Required: true
    SecretKey *string `json:"secretKey"`
}
```

#### Principal Model

```go
// models/principal.go (custom, not generated)
type Principal struct {
    STSAccessKeyID     string
    STSSecretAccessKey string
    STSSessionToken    string
    AccountAccessKey   string
    Actions            []string
    CustomStyleOb      string
    // ... другие поля
}
```

## 📦 Internal Packages: pkg/

### pkg/auth/ - Authentication

**Структура:**

```
pkg/auth/
├── token/
│   ├── token.go       # JWT generation/validation
│   └── token_test.go  # Tests
├── ldap/
│   └── ldap.go        # LDAP authentication
├── idp/
│   └── idp.go         # Identity Provider integration
├── utils/
│   └── utils.go       # Auth utilities
├── token.go           # Main auth logic
└── ldap.go            # LDAP integration
```

#### JWT Token Management (pkg/auth/token/)

```go
// Claims structure
type ClaimsWithCustomFields struct {
    jwt.StandardClaims
    STSAccessKeyID     string `json:"stsAccessKeyID,omitempty"`
    STSSecretAccessKey string `json:"stsSecretAccessKey,omitempty"`
    STSSessionToken    string `json:"stsSessionToken,omitempty"`
    AccountAccessKey   string `json:"accountAccessKey,omitempty"`
    // ...
}

// Generate JWT token
func GenerateJWT(claims ClaimsWithCustomFields) (string, error) {
    // Encrypt claims with PBKDF2
    encryptedClaims, err := EncryptClaims(claims)
    if err != nil {
        return "", err
    }

    // Create JWT token
    token := jwt.NewWithClaims(jwt.SigningMethodHS256, encryptedClaims)

    // Sign with secret
    tokenString, err := token.SignedString(getSigningKey())
    if err != nil {
        return "", err
    }

    return tokenString, nil
}

// Parse and validate JWT
func ParseClaimsFromToken(token string) (*ClaimsWithCustomFields, error) {
    // Parse JWT
    parsedToken, err := jwt.ParseWithClaims(token, &ClaimsWithCustomFields{}, func(token *jwt.Token) (interface{}, error) {
        return getSigningKey(), nil
    })
    if err != nil {
        return nil, err
    }

    // Extract claims
    claims, ok := parsedToken.Claims.(*ClaimsWithCustomFields)
    if !ok || !parsedToken.Valid {
        return nil, errors.New("invalid token")
    }

    // Decrypt claims
    decryptedClaims, err := DecryptClaims(claims)
    if err != nil {
        return nil, err
    }

    return decryptedClaims, nil
}

// Encryption/Decryption using PBKDF2
func EncryptClaims(claims ClaimsWithCustomFields) (string, error) {
    // Serialize to JSON
    data, err := json.Marshal(claims)
    if err != nil {
        return "", err
    }

    // Encrypt with AES-256-GCM
    encrypted, err := encrypt(data, getEncryptionKey())
    if err != nil {
        return "", err
    }

    return base64.StdEncoding.EncodeToString(encrypted), nil
}
```

### pkg/certs/ - Certificate Management

```go
// Load certificates for TLS
func LoadX509KeyPairFromFiles(certFile, keyFile string) (tls.Certificate, error) {
    cert, err := tls.LoadX509KeyPair(certFile, keyFile)
    if err != nil {
        return tls.Certificate{}, err
    }
    return cert, nil
}

// Multi-domain certificate support
func LoadCertificates(certsDir string) ([]tls.Certificate, error) {
    // Load default certificate
    defaultCert, err := LoadX509KeyPairFromFiles(
        filepath.Join(certsDir, "public.crt"),
        filepath.Join(certsDir, "private.key"),
    )
    if err != nil {
        return nil, err
    }

    certs := []tls.Certificate{defaultCert}

    // Load domain-specific certificates
    // certsDir/example.com/public.crt + private.key
    // certsDir/another.com/public.crt + private.key

    return certs, nil
}
```

### pkg/logger/ - Structured Logging

```go
package logger

import "log"

var debugLevel = 0 // 0-6

func SetDebugLevel(level int) {
    debugLevel = level
}

func Info(format string, v ...interface{}) {
    log.Printf("[INFO] "+format, v...)
}

func Error(format string, v ...interface{}) {
    log.Printf("[ERROR] "+format, v...)
}

func Debug(level int, format string, v ...interface{}) {
    if debugLevel >= level {
        log.Printf("[DEBUG] "+format, v...)
    }
}
```

## 🧪 Testing Backend

### Unit Tests (api/*_test.go)

**Пример: api/user_buckets_test.go**

```go
func TestListBuckets(t *testing.T) {
    // Setup mock client
    mockClient := &MockMinioClient{
        listBucketsFunc: func(ctx context.Context) ([]object storage.BucketInfo, error) {
            return []object storage.BucketInfo{
                {Name: "bucket1", CreationDate: time.Now()},
                {Name: "bucket2", CreationDate: time.Now()},
            }, nil
        },
    }

    // Call function
    response, err := getListBucketsResponse(mockClient, session, params)

    // Assertions
    assert.NoError(t, err)
    assert.Equal(t, 2, len(response.Buckets))
    assert.Equal(t, "bucket1", *response.Buckets[0].Name)
}
```

### Integration Tests (integration/)

**Пример: integration/buckets_test.go**

```go
func TestCreateBucket(t *testing.T) {
    // Start Object Storage in Docker
    // Start Console server
    // Make HTTP request to /api/v1/buckets
    // Verify bucket created in Object Storage
}
```

## 🔍 Error Handling

### Error Types

```go
// api/errors.go
type ErrorWithCode struct {
    Code    int
    Message string
}

func (e *ErrorWithCode) Error() string {
    return e.Message
}

// Helper functions
func ErrorWithContext(ctx context.Context, err error) *ErrorWithCode {
    // Extract error details
    // Map to HTTP status code
    // Return structured error
}
```

### HTTP Error Responses

```go
// All errors return ApiError model
type ApiError struct {
    Code    int    `json:"code"`
    Message string `json:"message"`
    Detail  string `json:"detail,omitempty"`
}
```

## 🚀 Следующие шаги

- **[03-frontend-architecture.md](03-frontend-architecture.md)** - Frontend архитектура
- **[04-authentication-authorization.md](04-authentication-authorization.md)** - Детали аутентификации
- **[08-testing.md](08-testing.md)** - Подробнее о тестировании

# 10. Паттерны и Best Practices

## 🏛️ Архитектурные паттерны

### 1. Clean Architecture (Layered Architecture)

```
Presentation Layer (API Handlers)
        ↓
Business Logic Layer (Domain Logic)
        ↓
Data Access Layer (Object Storage Client)
        ↓
External Services (Object Storage Server)
```

**Преимущества:**

- ✅ Separation of Concerns
- ✅ Testability
- ✅ Maintainability
- ✅ Flexibility

**Пример реализации:**

```go
// Presentation Layer
func (h *BucketHandler) ListBucketsHandler(params bucket.ListBucketsParams, principal *models.Principal) middleware.Responder {
    response, err := h.service.ListBuckets(ctx, principal)
    if err != nil {
        return bucket.NewListBucketsDefault(500).WithPayload(&models.ApiError{
            Message: err.Error(),
        })
    }
    return bucket.NewListBucketsOK().WithPayload(response)
}

// Business Logic Layer
func (s *BucketService) ListBuckets(ctx context.Context, principal *models.Principal) (*models.ListBucketsResponse, error) {
    // Validation
    if err := s.validatePrincipal(principal); err != nil {
        return nil, err
    }

    // Business logic
    buckets, err := s.repository.List(ctx, principal)
    if err != nil {
        return nil, err
    }

    // Transform
    return s.transformBuckets(buckets), nil
}

// Data Access Layer
func (r *BucketRepository) List(ctx context.Context, principal *models.Principal) ([]object storage.BucketInfo, error) {
    client, err := r.createClient(principal)
    if err != nil {
        return nil, err
    }
    return client.listBucketsWithContext(ctx)
}
```

### 2. Repository Pattern

```go
// Interface definition
type BucketRepository interface {
    List(ctx context.Context, principal *models.Principal) ([]object storage.BucketInfo, error)
    Create(ctx context.Context, principal *models.Principal, bucket *models.Bucket) error
    Delete(ctx context.Context, principal *models.Principal, name string) error
    Get(ctx context.Context, principal *models.Principal, name string) (*object storage.BucketInfo, error)
}

// Implementation
type minioBucketRepository struct {
    clientFactory ClientFactory
}

func NewBucketRepository(factory ClientFactory) BucketRepository {
    return &minioBucketRepository{
        clientFactory: factory,
    }
}

func (r *minioBucketRepository) List(ctx context.Context, principal *models.Principal) ([]object storage.BucketInfo, error) {
    client, err := r.clientFactory.Create(principal)
    if err != nil {
        return nil, err
    }
    return client.ListBuckets(ctx)
}
```

**Преимущества:**

- ✅ Abstraction over data source
- ✅ Easy to mock for testing
- ✅ Swap implementations
- ✅ Centralized data access logic

### 3. Dependency Injection

```go
// Container
type Container struct {
    BucketService  BucketService
    ObjectService  ObjectService
    UserService    UserService
    // ...
}

// Wire dependencies
func NewContainer() *Container {
    // Repositories
    bucketRepo := NewBucketRepository(clientFactory)
    objectRepo := NewObjectRepository(clientFactory)

    // Services
    bucketService := NewBucketService(bucketRepo)
    objectService := NewObjectService(objectRepo)

    return &Container{
        BucketService: bucketService,
        ObjectService: objectService,
    }
}

// Handler injection
func RegisterHandlers(api *operations.ConsoleAPI, container *Container) {
    api.BucketListBucketsHandler = bucket.ListBucketsHandlerFunc(
        func(params bucket.ListBucketsParams, principal *models.Principal) middleware.Responder {
            return container.BucketService.List(params, principal)
        },
    )
}
```

### 4. Factory Pattern

```go
// Client Factory
type ClientFactory interface {
    CreateMinioClient(principal *models.Principal) (MinioClient, error)
    CreateAdminClient(principal *models.Principal) (AdminClient, error)
}

type minioClientFactory struct {
    endpoint string
    secure   bool
}

func NewClientFactory(endpoint string, secure bool) ClientFactory {
    return &minioClientFactory{
        endpoint: endpoint,
        secure:   secure,
    }
}

func (f *minioClientFactory) CreateMinioClient(principal *models.Principal) (MinioClient, error) {
    creds := credentials.NewStaticV4(
        principal.STSAccessKeyID,
        principal.STSSecretAccessKey,
        principal.STSSessionToken,
    )

    client, err := object storage.New(f.endpoint, &object storage.Options{
        Creds:  creds,
        Secure: f.secure,
    })
    if err != nil {
        return nil, err
    }

    return &minioClient{client: client}, nil
}
```

### 5. Strategy Pattern (для разных auth методов)

```go
// Auth Strategy Interface
type AuthStrategy interface {
    Authenticate(credentials interface{}) (*models.Principal, error)
}

// Form Auth Strategy
type FormAuthStrategy struct {
    minioEndpoint string
}

func (s *FormAuthStrategy) Authenticate(creds interface{}) (*models.Principal, error) {
    loginReq := creds.(*models.LoginRequest)
    // Authenticate with Object Storage
    // Return principal
}

// OAuth2 Strategy
type OAuth2AuthStrategy struct {
    oauth2Config *oauth2.Config
}

func (s *OAuth2AuthStrategy) Authenticate(creds interface{}) (*models.Principal, error) {
    code := creds.(string)
    // Exchange code for token
    // Return principal
}

// LDAP Strategy
type LDAPAuthStrategy struct {
    ldapConfig *LDAPConfig
}

func (s *LDAPAuthStrategy) Authenticate(creds interface{}) (*models.Principal, error) {
    loginReq := creds.(*models.LoginRequest)
    // Authenticate with LDAP
    // Return principal
}

// Auth Service
type AuthService struct {
    strategies map[string]AuthStrategy
}

func (s *AuthService) Login(authType string, credentials interface{}) (*models.Principal, error) {
    strategy, exists := s.strategies[authType]
    if !exists {
        return nil, errors.New("unknown auth type")
    }
    return strategy.Authenticate(credentials)
}
```

## 🔒 Security Best Practices

### 1. Input Validation

```go
// Validate bucket name
func validateBucketName(name string) error {
    if len(name) < 3 || len(name) > 63 {
        return errors.New("bucket name must be between 3 and 63 characters")
    }

    // Only lowercase, numbers, hyphens
    matched, _ := regexp.MatchString("^[a-z0-9][a-z0-9-]*[a-z0-9]$", name)
    if !matched {
        return errors.New("invalid bucket name format")
    }

    // No consecutive hyphens
    if strings.Contains(name, "--") {
        return errors.New("bucket name cannot contain consecutive hyphens")
    }

    return nil
}

// Sanitize user input
func sanitizeInput(input string) string {
    // Remove special characters
    input = strings.TrimSpace(input)
    input = html.EscapeString(input)
    return input
}
```

### 2. Context с Timeout

```go
func handleRequest(w http.ResponseWriter, r *http.Request) {
    // Always use context with timeout
    ctx, cancel := context.WithTimeout(r.Context(), 30*time.Second)
    defer cancel()

    // Pass context to all operations
    result, err := performOperation(ctx)
    if err != nil {
        if ctx.Err() == context.DeadlineExceeded {
            http.Error(w, "Request timeout", http.StatusGatewayTimeout)
            return
        }
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    json.NewEncoder(w).Encode(result)
}
```

### 3. Secure JWT Handling

```go
// JWT с шифрованием sensitive данных
func generateSecureJWT(credentials *credentials.Value) (string, error) {
    // 1. Create claims
    claims := ClaimsWithCustomFields{
        StandardClaims: jwt.StandardClaims{
            ExpiresAt: time.Now().Add(12 * time.Hour).Unix(),
            IssuedAt:  time.Now().Unix(),
            NotBefore: time.Now().Unix(),
            Issuer:    "console",
        },
        STSAccessKeyID:     credentials.AccessKeyID,
        STSSecretAccessKey: credentials.SecretAccessKey,
        STSSessionToken:    credentials.SessionToken,
    }

    // 2. Encrypt sensitive fields
    encryptedClaims, err := encryptClaims(claims)
    if err != nil {
        return "", err
    }

    // 3. Sign JWT
    token := jwt.NewWithClaims(jwt.SigningMethodHS256, encryptedClaims)
    tokenString, err := token.SignedString(getJWTSecret())
    if err != nil {
        return "", err
    }

    return tokenString, nil
}

// Rotate JWT secret periodically
func rotateJWTSecret() {
    ticker := time.NewTicker(24 * time.Hour)
    defer ticker.Stop()

    for range ticker.C {
        newSecret := generateRandomSecret()
        updateJWTSecret(newSecret)
        logger.Info("JWT secret rotated")
    }
}
```

### 4. Rate Limiting

```go
// Simple rate limiter middleware
type RateLimiter struct {
    requests map[string]*rate.Limiter
    mu       sync.RWMutex
    rate     rate.Limit
    burst    int
}

func NewRateLimiter(r rate.Limit, b int) *RateLimiter {
    return &RateLimiter{
        requests: make(map[string]*rate.Limiter),
        rate:     r,
        burst:    b,
    }
}

func (rl *RateLimiter) getLimiter(ip string) *rate.Limiter {
    rl.mu.Lock()
    defer rl.mu.Unlock()

    limiter, exists := rl.requests[ip]
    if !exists {
        limiter = rate.NewLimiter(rl.rate, rl.burst)
        rl.requests[ip] = limiter
    }

    return limiter
}

func (rl *RateLimiter) Middleware(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        ip := getClientIP(r)
        limiter := rl.getLimiter(ip)

        if !limiter.Allow() {
            http.Error(w, "Rate limit exceeded", http.StatusTooManyRequests)
            return
        }

        next.ServeHTTP(w, r)
    })
}
```

## 🎯 Performance Optimization

### 1. Connection Pooling

```go
// Object Storage client с connection pooling
type ClientPool struct {
    pool    *sync.Pool
    factory ClientFactory
}

func NewClientPool(factory ClientFactory) *ClientPool {
    return &ClientPool{
        pool: &sync.Pool{
            New: func() interface{} {
                // Pool будет создавать новые клиенты по требованию
                return nil
            },
        },
        factory: factory,
    }
}

func (p *ClientPool) Get(principal *models.Principal) (MinioClient, error) {
    // Try to get from pool
    if client := p.pool.Get(); client != nil {
        return client.(MinioClient), nil
    }

    // Create new client
    return p.factory.CreateMinioClient(principal)
}

func (p *ClientPool) Put(client MinioClient) {
    p.pool.Put(client)
}
```

### 2. Caching

```go
// Simple in-memory cache
type Cache struct {
    data map[string]CacheEntry
    mu   sync.RWMutex
    ttl  time.Duration
}

type CacheEntry struct {
    Value      interface{}
    Expiration time.Time
}

func NewCache(ttl time.Duration) *Cache {
    c := &Cache{
        data: make(map[string]CacheEntry),
        ttl:  ttl,
    }

    // Cleanup expired entries
    go c.cleanup()

    return c
}

func (c *Cache) Get(key string) (interface{}, bool) {
    c.mu.RLock()
    defer c.mu.RUnlock()

    entry, exists := c.data[key]
    if !exists || time.Now().After(entry.Expiration) {
        return nil, false
    }

    return entry.Value, true
}

func (c *Cache) Set(key string, value interface{}) {
    c.mu.Lock()
    defer c.mu.Unlock()

    c.data[key] = CacheEntry{
        Value:      value,
        Expiration: time.Now().Add(c.ttl),
    }
}

func (c *Cache) cleanup() {
    ticker := time.NewTicker(time.Minute)
    defer ticker.Stop()

    for range ticker.C {
        c.mu.Lock()
        now := time.Now()
        for key, entry := range c.data {
            if now.After(entry.Expiration) {
                delete(c.data, key)
            }
        }
        c.mu.Unlock()
    }
}
```

### 3. Parallel Processing

```go
// Получение статистики buckets параллельно
func getBucketsWithStats(ctx context.Context, client MinioClient, buckets []object storage.BucketInfo) []BucketWithStats {
    results := make([]BucketWithStats, len(buckets))
    var wg sync.WaitGroup

    for i, bucket := range buckets {
        wg.Add(1)
        go func(index int, b object storage.BucketInfo) {
            defer wg.Done()

            // Get bucket size and object count
            size, count := getBucketStats(ctx, client, b.Name)

            results[index] = BucketWithStats{
                BucketInfo:   b,
                Size:         size,
                ObjectsCount: count,
            }
        }(i, bucket)
    }

    wg.Wait()
    return results
}
```

### 4. Streaming для больших файлов

```go
// Stream object download без загрузки в память
func streamObjectDownload(w http.ResponseWriter, r *http.Request, client MinioClient, bucket, object string) error {
    ctx := r.Context()

    // Get object
    obj, err := client.getObject(ctx, bucket, object, object storage.GetObjectOptions{})
    if err != nil {
        return err
    }
    defer obj.Close()

    // Get object info for headers
    stat, err := obj.Stat()
    if err != nil {
        return err
    }

    // Set headers
    w.Header().Set("Content-Type", stat.ContentType)
    w.Header().Set("Content-Length", fmt.Sprintf("%d", stat.Size))
    w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%s", object))

    // Stream content
    _, err = io.Copy(w, obj)
    return err
}
```

## 📝 Code Quality Best Practices

### 1. Error Handling

```go
// Wrap errors с контекстом
func getUserInfo(ctx context.Context, username string) (*UserInfo, error) {
    user, err := fetchUser(ctx, username)
    if err != nil {
        return nil, fmt.Errorf("failed to fetch user %s: %w", username, err)
    }

    groups, err := fetchUserGroups(ctx, username)
    if err != nil {
        return nil, fmt.Errorf("failed to fetch groups for user %s: %w", username, err)
    }

    return &UserInfo{
        User:   user,
        Groups: groups,
    }, nil
}

// Custom error types
type NotFoundError struct {
    Resource string
    ID       string
}

func (e *NotFoundError) Error() string {
    return fmt.Sprintf("%s with ID %s not found", e.Resource, e.ID)
}

func (e *NotFoundError) HTTPStatusCode() int {
    return http.StatusNotFound
}
```

### 2. Logging

```go
// Structured logging
logger.Info("User logged in",
    "username", username,
    "ip", clientIP,
    "timestamp", time.Now(),
)

logger.Error("Failed to create bucket",
    "bucket", bucketName,
    "user", username,
    "error", err,
)

// Log levels
// DEBUG - Детальная информация для отладки
// INFO - Общая информация о работе
// WARN - Предупреждения
// ERROR - Ошибки
// FATAL - Критические ошибки (завершение работы)
```

### 3. Testing Best Practices

```go
// Table-driven tests
func TestValidateBucketName(t *testing.T) {
    tests := []struct {
        name        string
        bucketName  string
        expectError bool
    }{
        {"valid lowercase", "valid-bucket", false},
        {"invalid uppercase", "InvalidBucket", true},
        {"too short", "ab", true},
        {"too long", strings.Repeat("a", 64), true},
        {"consecutive hyphens", "bucket--name", true},
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            err := validateBucketName(tt.bucketName)
            if tt.expectError && err == nil {
                t.Errorf("Expected error for %s", tt.bucketName)
            }
            if !tt.expectError && err != nil {
                t.Errorf("Unexpected error for %s: %v", tt.bucketName, err)
            }
        })
    }
}

// Test helpers
func createTestServer(t *testing.T) *httptest.Server {
    handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        // Mock responses
    })
    return httptest.NewServer(handler)
}

// Cleanup
func TestWithCleanup(t *testing.T) {
    server := createTestServer(t)
    defer server.Close()

    // Test logic
}
```

## 🎨 Frontend Best Practices

### 1. Component Composition

```tsx
// Small, reusable components
const Button = ({ onClick, children, variant = 'primary' }) => (
    <button className={`btn btn-${variant}`} onClick={onClick}>
        {children}
    </button>
);

// Composition
const BucketActions = ({ bucket }) => (
    <div className="bucket-actions">
        <Button variant="primary" onClick={() => download(bucket)}>
            Download
        </Button>
        <Button variant="secondary" onClick={() => share(bucket)}>
            Share
        </Button>
        <Button variant="danger" onClick={() => deleteBucket(bucket)}>
            Delete
        </Button>
    </div>
);
```

### 2. Custom Hooks

```tsx
// Custom hook for API calls
const useAPI = (apiCall) => {
    const [data, setData] = useState(null);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);

    const execute = async (...args) => {
        try {
            setLoading(true);
            setError(null);
            const result = await apiCall(...args);
            setData(result);
            return result;
        } catch (err) {
            setError(err);
            throw err;
        } finally {
            setLoading(false);
        }
    };

    return { data, loading, error, execute };
};

// Usage
const BucketsList = () => {
    const api = new ConsoleApi();
    const { data: buckets, loading, error, execute } = useAPI(api.listBuckets);

    useEffect(() => {
        execute();
    }, []);

    if (loading) return <Loading />;
    if (error) return <Error message={error.message} />;

    return <BucketGrid buckets={buckets} />;
};
```

### 3. Error Boundaries

```tsx
class ErrorBoundary extends React.Component {
    constructor(props) {
        super(props);
        this.state = { hasError: false, error: null };
    }

    static getDerivedStateFromError(error) {
        return { hasError: true, error };
    }

    componentDidCatch(error, errorInfo) {
        console.error('Error caught by boundary:', error, errorInfo);
        // Log to error tracking service
    }

    render() {
        if (this.state.hasError) {
            return (
                <div className="error-boundary">
                    <h2>Something went wrong</h2>
                    <p>{this.state.error.message}</p>
                    <button onClick={() => window.location.reload()}>
                        Reload
                    </button>
                </div>
            );
        }

        return this.props.children;
    }
}

// Usage
<ErrorBoundary>
    <App />
</ErrorBoundary>
```

## 🚀 Следующие шаги

- **[11-dependencies-integrations.md](11-dependencies-integrations.md)** - Интеграции
- **[12-advanced-topics.md](12-advanced-topics.md)** - Продвинутые темы
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика

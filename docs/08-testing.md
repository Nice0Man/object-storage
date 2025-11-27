# 08. Тестирование

## 🧪 Обзор тестовой стратегии

Object Storage Console использует **многоуровневую стратегию тестирования**:

1. **Unit Tests** - Тестирование отдельных функций/компонентов
2. **Integration Tests** - Тестирование взаимодействия с Object Storage
3. **E2E Tests** - End-to-end тестирование через UI
4. **Replication Tests** - Тестирование multi-site репликации
5. **SSO Tests** - Тестирование SSO/LDAP интеграции

## 📂 Структура тестов

```
openmaxio-object-browser/
├── api/
│   ├── *_test.go              # Unit tests для API handlers
│   ├── admin_objects_test.go
│   ├── user_buckets_test.go
│   └── ...
├── pkg/
│   └── auth/
│       └── token_test.go      # Unit tests для auth
├── integration/               # Integration tests
│   ├── login_test.go
│   ├── buckets_test.go
│   ├── objects_test.go
│   └── ...
├── replication/              # Replication tests
│   └── replication_test.go
├── sso-integration/          # SSO tests
│   └── sso_test.go
└── web-app/
    ├── tests/                # Frontend test scenarios
    ├── e2e/                  # E2E tests
    └── playwright/           # Playwright config
```

## 🔬 Backend Unit Tests

### Unit Test Pattern

```go
// api/user_buckets_test.go
package api

import (
    "context"
    "testing"
    "time"

    "github.com/object storage/object storage-go/v7"
    "github.com/stretchr/testify/assert"
)

// Mock Object Storage Client
type MockMinioClient struct {
    listBucketsFunc func(ctx context.Context) ([]object storage.BucketInfo, error)
    makeBucketFunc  func(ctx context.Context, bucketName, location string, objectLocking bool) error
}

func (m *MockMinioClient) listBucketsWithContext(ctx context.Context) ([]object storage.BucketInfo, error) {
    if m.listBucketsFunc != nil {
        return m.listBucketsFunc(ctx)
    }
    return nil, nil
}

func (m *MockMinioClient) makeBucketWithContext(ctx context.Context, bucketName, location string, objectLocking bool) error {
    if m.makeBucketFunc != nil {
        return m.makeBucketFunc(ctx, bucketName, location, objectLocking)
    }
    return nil
}

// Test function
func TestListBuckets(t *testing.T) {
    // Setup mock
    mockClient := &MockMinioClient{
        listBucketsFunc: func(ctx context.Context) ([]object storage.BucketInfo, error) {
            return []object storage.BucketInfo{
                {
                    Name:         "bucket1",
                    CreationDate: time.Now(),
                },
                {
                    Name:         "bucket2",
                    CreationDate: time.Now(),
                },
            }, nil
        },
    }

    // Create session
    session := &models.Principal{
        STSAccessKeyID:     "test-access-key",
        STSSecretAccessKey: "test-secret-key",
    }

    // Call function (you need to inject mockClient)
    // response, err := getListBucketsResponse(mockClient, session, params)

    // Assertions
    // assert.NoError(t, err)
    // assert.Equal(t, 2, len(response.Buckets))
    // assert.Equal(t, "bucket1", *response.Buckets[0].Name)
}

func TestCreateBucket(t *testing.T) {
    tests := []struct {
        name        string
        bucketName  string
        expectError bool
    }{
        {
            name:        "Valid bucket name",
            bucketName:  "valid-bucket",
            expectError: false,
        },
        {
            name:        "Invalid bucket name (uppercase)",
            bucketName:  "InvalidBucket",
            expectError: true,
        },
        {
            name:        "Invalid bucket name (too short)",
            bucketName:  "ab",
            expectError: true,
        },
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            // Test logic
            err := validateBucketName(tt.bucketName)

            if tt.expectError {
                assert.Error(t, err)
            } else {
                assert.NoError(t, err)
            }
        })
    }
}
```

### Running Unit Tests

```bash
# Run all unit tests in api/
cd api
go test -v ./...

# Run specific test
go test -v -run TestListBuckets

# Run with coverage
go test -v -coverprofile=coverage.out ./...
go tool cover -html=coverage.out
```

### Coverage Target

```makefile
# Makefile
test:
 @echo "execute test and get coverage"
 @(cd api && mkdir -p coverage && GO111MODULE=on go test ./... -test.v -coverprofile=coverage/coverage.out)

test-pkg:
 @echo "execute test and get coverage"
 @(cd pkg && mkdir -p coverage && GO111MODULE=on go test ./... -test.v -coverprofile=coverage/coverage-pkg.out)
```

## 🔗 Integration Tests

### Integration Test Setup

**Location:** `integration/`

**Подход:** Запуск реального Object Storage в Docker

```go
// integration/buckets_test.go
package integration

import (
    "testing"
    "net/http"
    "encoding/json"
)

// Setup: Start Object Storage and Console in Docker
// See Makefile target: test-integration

func TestIntegration_CreateBucket(t *testing.T) {
    // 1. Login to get JWT token
    loginResp := login(t, "minioadmin", "minioadmin")
    token := loginResp.SessionID

    // 2. Create bucket via API
    createReq := &models.MakeBucketRequest{
        Name:   swag.String("test-bucket"),
        Region: "us-east-1",
    }

    body, _ := json.Marshal(createReq)
    req, _ := http.NewRequest("POST", "http://localhost:9090/api/v1/buckets", bytes.NewReader(body))
    req.Header.Set("Authorization", "Bearer "+token)
    req.Header.Set("Content-Type", "application/json")

    resp, err := http.DefaultClient.Do(req)
    assert.NoError(t, err)
    assert.Equal(t, http.StatusOK, resp.StatusCode)

    // 3. Verify bucket exists in Object Storage
    listReq, _ := http.NewRequest("GET", "http://localhost:9090/api/v1/buckets", nil)
    listReq.Header.Set("Authorization", "Bearer "+token)

    listResp, err := http.DefaultClient.Do(listReq)
    assert.NoError(t, err)

    var bucketsResp models.ListBucketsResponse
    json.NewDecoder(listResp.Body).Decode(&bucketsResp)

    found := false
    for _, bucket := range bucketsResp.Buckets {
        if *bucket.Name == "test-bucket" {
            found = true
            break
        }
    }
    assert.True(t, found, "Bucket should exist")

    // 4. Cleanup: Delete bucket
    deleteReq, _ := http.NewRequest("DELETE", "http://localhost:9090/api/v1/buckets/test-bucket", nil)
    deleteReq.Header.Set("Authorization", "Bearer "+token)
    http.DefaultClient.Do(deleteReq)
}

func TestIntegration_UploadDownloadObject(t *testing.T) {
    token := login(t, "minioadmin", "minioadmin").SessionID

    // Create bucket
    createBucket(t, token, "test-uploads")

    // Upload file
    file := bytes.NewReader([]byte("test content"))
    uploadReq, _ := http.NewRequest(
        "POST",
        "http://localhost:9090/api/v1/buckets/test-uploads/objects/upload?prefix=test.txt",
        file,
    )
    uploadReq.Header.Set("Authorization", "Bearer "+token)
    uploadResp, err := http.DefaultClient.Do(uploadReq)
    assert.NoError(t, err)
    assert.Equal(t, http.StatusOK, uploadResp.StatusCode)

    // Download file
    downloadReq, _ := http.NewRequest(
        "GET",
        "http://localhost:9090/api/v1/buckets/test-uploads/objects/download?prefix=test.txt",
        nil,
    )
    downloadReq.Header.Set("Authorization", "Bearer "+token)
    downloadResp, err := http.DefaultClient.Do(downloadReq)
    assert.NoError(t, err)

    content, _ := ioutil.ReadAll(downloadResp.Body)
    assert.Equal(t, "test content", string(content))

    // Cleanup
    deleteBucket(t, token, "test-uploads")
}
```

### Running Integration Tests

```bash
# Makefile автоматизирует setup
make test-integration
```

**Что делает Makefile:**

1. Создает Docker network
2. Запускает Object Storage в Docker
3. Запускает Console server
4. Выполняет тесты
5. Останавливает контейнеры
6. Очищает resources

```makefile
test-integration:
 @(docker network create mynet123)
 @(docker run -d --name object storage --network mynet123 -p 9000:9000 \
   -e MINIO_KMS_SECRET_KEY=my-key:xxx \
   quay.io/object storage/object storage:latest server /data{1...4} --console-address ':9091')
 @(sleep 5)
 @(cd integration && go test -coverpkg=../api -c -tags testrunmain . && \
   ./integration.test -test.v -test.run "^Test*" -test.coverprofile=coverage/system.out)
 @(docker stop object storage)
 @(docker network rm mynet123)
```

## 🔄 Replication Tests

**Location:** `replication/`

### Multi-Site Replication Test

```go
// replication/replication_test.go
func TestReplication_SiteSetup(t *testing.T) {
    // Setup: 3 Object Storage instances (object storage, minio1, minio2)
    // Each running on different port (9000, 9001, 9002)

    // 1. Create admin credentials for all sites
    sites := []madmin.PeerSite{
        {
            Name:      "site1",
            Endpoint:  "http://object storage:9000",
            AccessKey: "minioadmin",
            SecretKey: "minioadmin",
        },
        {
            Name:      "site2",
            Endpoint:  "http://minio1:9001",
            AccessKey: "minioadmin",
            SecretKey: "minioadmin",
        },
        {
            Name:      "site3",
            Endpoint:  "http://minio2:9002",
            AccessKey: "minioadmin",
            SecretKey: "minioadmin",
        },
    }

    // 2. Initialize site replication
    token := login(t, "minioadmin", "minioadmin").SessionID

    addSiteReqBody := &models.SiteReplicationAddRequest{
        Sites: sites,
    }

    addResp := makeRequest(t, "POST", "/api/v1/admin/site-replication", token, addSiteReqBody)
    assert.Equal(t, http.StatusOK, addResp.StatusCode)

    // 3. Verify replication is configured
    statusResp := makeRequest(t, "GET", "/api/v1/admin/site-replication/status", token, nil)
    assert.Equal(t, http.StatusOK, statusResp.StatusCode)

    // 4. Test data replication
    // Create bucket on site1
    createBucket(t, token, "replicated-bucket")

    // Upload object to site1
    uploadObject(t, token, "replicated-bucket", "test.txt", "content")

    // Wait for replication
    time.Sleep(5 * time.Second)

    // Verify object exists on site2 and site3
    // (requires connecting to other Object Storage instances)
}
```

### Running Replication Tests

```bash
make test-replication
```

## 🔐 SSO Integration Tests

**Location:** `sso-integration/`

### SSO Test Setup

**Components:**

1. OpenLDAP (user database)
2. Dex (OIDC provider)
3. Object Storage (configured with OIDC)
4. Console

```go
// sso-integration/sso_test.go
func TestSSO_LoginWithDex(t *testing.T) {
    // 1. Access Console login page
    loginPageResp, err := http.Get("http://localhost:9090/login")
    assert.NoError(t, err)

    // 2. Get login details (should show SSO option)
    loginDetails := getLoginDetails(t)
    assert.True(t, loginDetails.IsSSO)
    assert.NotEmpty(t, loginDetails.RedirectURL)

    // 3. Simulate OAuth2 flow
    // (requires browser automation or manual steps)

    // For automation, using Python script with BeautifulSoup
    // See: sso-integration/dex-requests.py

    // 4. Get authorization code from callback
    code := simulateOAuthFlow(t)

    // 5. Exchange code for token
    tokenResp := exchangeCodeForToken(t, code)
    assert.NotEmpty(t, tokenResp.SessionID)

    // 6. Use token to make authenticated request
    bucketsResp := listBuckets(t, tokenResp.SessionID)
    assert.NotNil(t, bucketsResp)
}
```

### Running SSO Tests

```bash
make test-sso-integration
```

**Что делает Makefile:**

1. Запускает OpenLDAP container
2. Запускает Dex container (OIDC provider)
3. Настраивает Object Storage с OIDC
4. Создает test пользователя в LDAP
5. Назначает политики
6. Выполняет SSO тесты
7. Очищает контейнеры

## 🎭 Frontend E2E Tests

### Playwright Tests

**Location:** `web-app/playwright/`

```typescript
// playwright/tests/login.spec.ts
import { test, expect } from '@playwright/test';

test.describe('Login Flow', () => {
    test('should login with valid credentials', async ({ page }) => {
        // Navigate to login page
        await page.goto('http://localhost:5005/login');

        // Fill in credentials
        await page.fill('[name="accessKey"]', 'minioadmin');
        await page.fill('[name="secretKey"]', 'minioadmin');

        // Submit form
        await page.click('button[type="submit"]');

        // Wait for navigation to console
        await expect(page).toHaveURL(/.*console/);

        // Verify dashboard is visible
        await expect(page.locator('.dashboard')).toBeVisible();
    });

    test('should show error with invalid credentials', async ({ page }) => {
        await page.goto('http://localhost:5005/login');

        await page.fill('[name="accessKey"]', 'invalid');
        await page.fill('[name="secretKey"]', 'invalid');
        await page.click('button[type="submit"]');

        // Should show error message
        await expect(page.locator('.error-message')).toBeVisible();
        await expect(page.locator('.error-message')).toContainText('Invalid credentials');
    });
});

test.describe('Bucket Operations', () => {
    test.beforeEach(async ({ page }) => {
        // Login before each test
        await page.goto('http://localhost:5005/login');
        await page.fill('[name="accessKey"]', 'minioadmin');
        await page.fill('[name="secretKey"]', 'minioadmin');
        await page.click('button[type="submit"]');
        await page.waitForURL(/.*console/);
    });

    test('should create new bucket', async ({ page }) => {
        // Navigate to buckets
        await page.click('[href="/console/buckets"]');

        // Click create button
        await page.click('button:has-text("Create Bucket")');

        // Fill in bucket name
        await page.fill('[name="bucketName"]', 'test-bucket');

        // Submit
        await page.click('button:has-text("Create")');

        // Verify bucket appears in list
        await expect(page.locator('text=test-bucket')).toBeVisible();
    });

    test('should upload file to bucket', async ({ page }) => {
        // Navigate to bucket
        await page.click('[href="/console/buckets"]');
        await page.click('text=test-bucket');

        // Upload file
        const fileInput = await page.locator('input[type="file"]');
        await fileInput.setInputFiles('test-file.txt');

        // Wait for upload to complete
        await expect(page.locator('text=test-file.txt')).toBeVisible();
    });
});
```

### Running Playwright Tests

```bash
cd web-app

# Install browsers (первый раз)
npx playwright install

# Run tests
npx playwright test

# Run with UI mode
npx playwright test --ui

# Run specific test
npx playwright test login.spec.ts
```

### Test Configuration

```typescript
// playwright.config.ts
import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
    testDir: './playwright/tests',
    timeout: 30000,
    retries: 2,
    use: {
        baseURL: 'http://localhost:5005',
        screenshot: 'only-on-failure',
        video: 'retain-on-failure',
    },
    projects: [
        {
            name: 'chromium',
            use: { ...devices['Desktop Chrome'] },
        },
        {
            name: 'firefox',
            use: { ...devices['Desktop Firefox'] },
        },
        {
            name: 'webkit',
            use: { ...devices['Desktop Safari'] },
        },
    ],
});
```

## 🎯 Permission Tests

**Location:** `web-app/tests/permissions-*/`

Тестируют различные сценарии с разными IAM политиками:

```bash
# Test with read-only access
make test-permissions-1

# Test with write access
make test-permissions-2

# Test with admin access
make test-permissions-3
```

## 📊 Test Coverage

### Coverage Goals

| Component | Target Coverage |
|-----------|----------------|
| API Handlers | 80%+ |
| Auth Package | 90%+ |
| Utils | 70%+ |
| Integration | Key flows |

### Viewing Coverage

```bash
# Backend coverage
cd api
go test -coverprofile=coverage.out ./...
go tool cover -html=coverage.out

# Frontend coverage
cd web-app
yarn test --coverage
```

## 🚀 CI/CD Integration

### GitHub Actions Example

```yaml
# .github/workflows/test.yml
name: Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  backend-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Set up Go
        uses: actions/setup-go@v4
        with:
          go-version: '1.23'

      - name: Run unit tests
        run: make test

      - name: Run integration tests
        run: make test-integration

      - name: Upload coverage
        uses: codecov/codecov-action@v3
        with:
          files: ./api/coverage/coverage.out

  frontend-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Set up Node
        uses: actions/setup-node@v3
        with:
          node-version: '20'

      - name: Install dependencies
        run: cd web-app && yarn install

      - name: Run tests
        run: cd web-app && yarn test

      - name: Run Playwright tests
        run: cd web-app && npx playwright test
```

## 🎯 Следующие шаги

- **[09-api-specification.md](09-api-specification.md)** - API документация
- **[10-patterns-best-practices.md](10-patterns-best-practices.md)** - Testing patterns
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика написания тестов

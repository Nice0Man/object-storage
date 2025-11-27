# 04. Аутентификация и авторизация

## 🔐 Обзор системы безопасности

### Методы аутентификации

Object Storage Console поддерживает **4 метода аутентификации**:

1. **Form-based (Username/Password)** - стандартная форма логина
2. **OAuth2/OIDC (SSO)** - Single Sign-On через Identity Provider
3. **LDAP/Active Directory** - корпоративная аутентификация
4. **Anonymous Access** - публичный доступ (read-only)

## 🎫 JWT Token-Based Authentication

### Архитектура JWT Flow

```
┌─────────┐                                          ┌──────────┐
│ Browser │                                          │  Object Storage   │
└────┬────┘                                          └────┬─────┘
     │                                                    │
     │ 1. POST /api/v1/login                             │
     │    { accessKey, secretKey }                       │
     ├──────────────────────────────────►                │
     │                                   │                │
     │                                   │ 2. Authenticate│
     │                                   ├───────────────►│
     │                                   │                │
     │                                   │ 3. Return STS  │
     │                                   │◄───────────────┤
     │                                   │                │
     │ 4. Generate JWT with STS          │                │
     │    Claims (encrypted):            │                │
     │    {                              │                │
     │      STSAccessKeyID,              │                │
     │      STSSecretAccessKey,          │                │
     │      STSSessionToken,             │                │
     │      exp: 12h                     │                │
     │    }                              │                │
     │                                   │                │
     │ 5. Response:                      │                │
     │    Set-Cookie: token=JWT          │                │
     │◄──────────────────────────────────┤                │
     │                                                    │
     │ 6. Subsequent requests:                            │
     │    Authorization: Bearer JWT                       │
     ├──────────────────────────────────►                │
     │                                   │                │
     │                                   │ 7. Validate JWT│
     │                                   │    Extract STS │
     │                                   │                │
     │                                   │ 8. Call Object Storage  │
     │                                   ├───────────────►│
     │                                   │                │
```

### JWT Token Structure

```go
// pkg/auth/token/token.go

type ClaimsWithCustomFields struct {
    jwt.StandardClaims

    // Object Storage STS credentials (encrypted)
    STSAccessKeyID     string `json:"stsAccessKeyID,omitempty"`
    STSSecretAccessKey string `json:"stsSecretAccessKey,omitempty"`
    STSSessionToken    string `json:"stsSessionToken,omitempty"`

    // Account info
    AccountAccessKey   string `json:"accountAccessKey,omitempty"`

    // Permissions
    Actions            []string `json:"actions,omitempty"`

    // Custom fields
    CustomStyleOb      string `json:"customStyleOb,omitempty"`
}
```

### Token Generation

```go
// api/user_login.go

func generateJWT(stsCredentials *credentials.Value) (string, error) {
    // Create claims
    claims := ClaimsWithCustomFields{
        StandardClaims: jwt.StandardClaims{
            ExpiresAt: time.Now().Add(12 * time.Hour).Unix(),
            Issuer:    "console",
        },
        STSAccessKeyID:     stsCredentials.AccessKeyID,
        STSSecretAccessKey: stsCredentials.SecretAccessKey,
        STSSessionToken:    stsCredentials.SessionToken,
    }

    // Encrypt sensitive fields with PBKDF2
    encryptedClaims, err := auth.EncryptClaims(claims)
    if err != nil {
        return "", err
    }

    // Sign JWT
    token := jwt.NewWithClaims(jwt.SigningMethodHS256, encryptedClaims)
    tokenString, err := token.SignedString(getJWTSecret())
    if err != nil {
        return "", err
    }

    return tokenString, nil
}
```

### Token Validation

```go
// api/configure_console.go

api.KeyAuth = func(token string, scopes []string) (*models.Principal, error) {
    // Handle anonymous access
    if token == "Anonymous" {
        return &models.Principal{}, nil
    }

    // Parse and validate JWT
    claims, err := auth.ParseClaimsFromToken(token)
    if err != nil {
        api.Logger("Token validation failed: %v", err)
        return nil, errors.New(401, "incorrect api key auth")
    }

    // Check expiration
    if claims.ExpiresAt < time.Now().Unix() {
        return nil, errors.New(401, "token expired")
    }

    // Return principal with decrypted credentials
    return &models.Principal{
        STSAccessKeyID:     claims.STSAccessKeyID,
        STSSecretAccessKey: claims.STSSecretAccessKey,
        STSSessionToken:    claims.STSSessionToken,
        AccountAccessKey:   claims.AccountAccessKey,
        Actions:            claims.Actions,
    }, nil
}
```

### Encryption (PBKDF2)

```go
// pkg/auth/token/token.go

func EncryptClaims(claims ClaimsWithCustomFields) (string, error) {
    // Serialize to JSON
    data, err := json.Marshal(claims)
    if err != nil {
        return "", err
    }

    // Derive key from passphrase using PBKDF2
    key := pbkdf2.Key(
        []byte(getPassphrase()),
        []byte(getSalt()),
        4096,  // iterations
        32,    // key length (256 bits)
        sha256.New,
    )

    // Encrypt with AES-256-GCM
    block, err := aes.NewCipher(key)
    if err != nil {
        return "", err
    }

    gcm, err := cipher.NewGCM(block)
    if err != nil {
        return "", err
    }

    nonce := make([]byte, gcm.NonceSize())
    if _, err := io.ReadFull(rand.Reader, nonce); err != nil {
        return "", err
    }

    ciphertext := gcm.Seal(nonce, nonce, data, nil)

    return base64.StdEncoding.EncodeToString(ciphertext), nil
}
```

## 🔑 Form-Based Authentication

### Login Flow

```
User enters credentials
  ↓
POST /api/v1/login { accessKey, secretKey }
  ↓
Backend validates with Object Storage
  ↓
Object Storage returns STS credentials
  ↓
Generate JWT with STS embedded
  ↓
Return JWT as cookie + header
  ↓
Frontend stores in localStorage
  ↓
All subsequent requests include JWT
```

### Backend Implementation

```go
// api/user_login.go

func getLoginResponse(lr *models.LoginRequest) (*string, error) {
    // Extract credentials
    accessKey := *lr.AccessKey
    secretKey := *lr.SecretKey

    // Create Object Storage credentials
    creds := credentials.NewStaticV4(accessKey, secretKey, "")

    // Create Object Storage client
    client, err := newMinioClient(creds)
    if err != nil {
        return nil, err
    }

    // Test connection by listing buckets
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()

    _, err = client.listBucketsWithContext(ctx)
    if err != nil {
        return nil, errors.New("invalid credentials")
    }

    // Get STS credentials from Object Storage
    stsCredentials, err := getSTSCredentials(accessKey, secretKey)
    if err != nil {
        return nil, err
    }

    // Generate JWT
    token, err := generateJWT(stsCredentials)
    if err != nil {
        return nil, err
    }

    return &token, nil
}
```

### Frontend Implementation

```tsx
// screens/LoginPage/LoginPage.tsx

const LoginPage = () => {
  const [accessKey, setAccessKey] = useState('');
  const [secretKey, setSecretKey] = useState('');
  const [error, setError] = useState('');
  const dispatch = useDispatch();
  const navigate = useNavigate();

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    try {
      const api = new ConsoleApi();
      const response = await api.login({
        accessKey,
        secretKey,
      });

      // Store token
      localStorage.setItem('token', response.data.token);

      // Update Redux store
      dispatch(setLogin(true));

      // Navigate to console
      navigate('/console');
    } catch (error) {
      setError('Invalid credentials');
    }
  };

  return (
    <form onSubmit={handleLogin}>
      <InputBox
        label="Access Key"
        value={accessKey}
        onChange={(e) => setAccessKey(e.target.value)}
      />
      <InputBox
        type="password"
        label="Secret Key"
        value={secretKey}
        onChange={(e) => setSecretKey(e.target.value)}
      />
      {error && <ErrorMessage>{error}</ErrorMessage>}
      <Button type="submit">Login</Button>
    </form>
  );
};
```

## 🌐 OAuth2/OIDC (SSO)

### OAuth2 Flow

```
1. User clicks "Login with SSO"
   ↓
2. Redirect to Identity Provider (IDP)
   GET https://idp.example.com/authorize?
       client_id=console&
       redirect_uri=http://console/oauth_callback&
       response_type=code&
       scope=openid profile email
   ↓
3. User authenticates at IDP
   ↓
4. IDP redirects back with authorization code
   GET http://console/oauth_callback?code=ABC123
   ↓
5. Console exchanges code for access token
   POST https://idp.example.com/token
   { code: ABC123, client_id, client_secret }
   ↓
6. IDP returns access token + id_token
   ↓
7. Console validates id_token (JWT)
   ↓
8. Extract user info from id_token
   ↓
9. Get Object Storage STS credentials for OIDC user
   ↓
10. Generate Console JWT
   ↓
11. Return JWT to frontend
```

### Backend Configuration

```go
// Environment variables
CONSOLE_IDP_CLIENT_ID=console-app
CONSOLE_IDP_CLIENT_SECRET=secret
CONSOLE_IDP_URL=https://dex.example.com
CONSOLE_IDP_CALLBACK=http://localhost:9090/oauth_callback

// Object Storage server must also be configured
MINIO_IDENTITY_OPENID_CLIENT_ID=console-app
MINIO_IDENTITY_OPENID_CLIENT_SECRET=secret
MINIO_IDENTITY_OPENID_CONFIG_URL=https://dex.example.com/.well-known/openid-configuration
MINIO_IDENTITY_OPENID_REDIRECT_URI=http://localhost:9090/oauth_callback
```

### OAuth2 Handler

```go
// api/user_login.go

func getLoginOauth2AuthResponse(lr *models.LoginOauth2AuthRequest) (*models.LoginResponse, error) {
    // Extract authorization code
    code := *lr.Code
    state := *lr.State

    // Validate state (CSRF protection)
    if !validateState(state) {
        return nil, errors.New("invalid state")
    }

    // Exchange code for tokens
    oauth2Config := getOAuth2Config()
    token, err := oauth2Config.Exchange(context.Background(), code)
    if err != nil {
        return nil, err
    }

    // Extract id_token
    rawIDToken, ok := token.Extra("id_token").(string)
    if !ok {
        return nil, errors.New("no id_token in response")
    }

    // Verify id_token
    idToken, err := verifyIDToken(rawIDToken)
    if err != nil {
        return nil, err
    }

    // Extract user info
    var claims struct {
        Email         string `json:"email"`
        EmailVerified bool   `json:"email_verified"`
        Name          string `json:"name"`
    }
    if err := idToken.Claims(&claims); err != nil {
        return nil, err
    }

    // Get Object Storage STS credentials for OIDC user
    stsCredentials, err := getSTSCredentialsForOIDC(rawIDToken)
    if err != nil {
        return nil, err
    }

    // Generate Console JWT
    token, err := generateJWT(stsCredentials)
    if err != nil {
        return nil, err
    }

    return &models.LoginResponse{
        SessionID: token,
    }, nil
}
```

## 🏢 LDAP Authentication

### LDAP Flow

```
1. User enters username/password
   ↓
2. Console binds to LDAP server
   LDAP Bind: DN=cn=admin,dc=example,dc=org
   ↓
3. Search for user
   LDAP Search: uid=user,dc=example,dc=org
   ↓
4. Verify password
   LDAP Bind: DN=uid=user,dc=example,dc=org, Password
   ↓
5. Get user groups
   LDAP Search: memberOf attributes
   ↓
6. Get Object Storage STS credentials for LDAP user
   ↓
7. Generate Console JWT
   ↓
8. Return JWT to frontend
```

### LDAP Configuration

```bash
# Environment variables
CONSOLE_LDAP_ENABLED=on

# Object Storage server LDAP configuration
MINIO_IDENTITY_LDAP_SERVER_ADDR=ldap.example.com:389
MINIO_IDENTITY_LDAP_USERNAME_FORMAT=uid=%s,dc=example,dc=org
MINIO_IDENTITY_LDAP_USERNAME_SEARCH_FILTER=(|(objectclass=posixAccount)(uid=%s))
MINIO_IDENTITY_LDAP_GROUP_SEARCH_BASE_DN=dc=example,dc=org
MINIO_IDENTITY_LDAP_GROUP_SEARCH_FILTER=(&(objectclass=groupOfNames)(member=%d))
MINIO_IDENTITY_LDAP_TLS_SKIP_VERIFY=on
```

### LDAP Handler

```go
// pkg/auth/ldap/ldap.go

func authenticateLDAP(username, password string) (*credentials.Value, error) {
    // LDAP server configuration
    ldapServer := getLDAPServer()
    ldapBaseDN := getLDAPBaseDN()

    // Connect to LDAP
    conn, err := ldap.Dial("tcp", ldapServer)
    if err != nil {
        return nil, err
    }
    defer conn.Close()

    // Bind with user credentials
    userDN := fmt.Sprintf("uid=%s,%s", username, ldapBaseDN)
    err = conn.Bind(userDN, password)
    if err != nil {
        return nil, errors.New("invalid credentials")
    }

    // Search for user groups
    searchRequest := ldap.NewSearchRequest(
        ldapBaseDN,
        ldap.ScopeWholeSubtree,
        ldap.NeverDerefAliases,
        0, 0, false,
        fmt.Sprintf("(uid=%s)", username),
        []string{"memberOf"},
        nil,
    )

    result, err := conn.Search(searchRequest)
    if err != nil {
        return nil, err
    }

    // Get Object Storage STS credentials for LDAP user
    stsCredentials, err := getSTSCredentialsForLDAP(userDN)
    if err != nil {
        return nil, err
    }

    return stsCredentials, nil
}
```

## 👤 Anonymous Access

### Anonymous Flow

```
1. User accesses public bucket URL
   GET /browser/public-bucket
   ↓
2. Frontend sends request with Anonymous token
   Authorization: Bearer Anonymous
   ↓
3. Backend recognizes Anonymous principal
   ↓
4. Create Object Storage client with Anonymous credentials
   ↓
5. List objects (read-only)
   ↓
6. Return public objects
```

### Backend Implementation

```go
// api/configure_console.go

api.KeyAuth = func(token string, scopes []string) (*models.Principal, error) {
    if token == "Anonymous" {
        // Return anonymous principal
        return &models.Principal{
            Anonymous: true,
        }, nil
    }
    // ... regular JWT validation
}

// api/public_objects.go

func getPublicObjectsResponse(principal *models.Principal, params object.ListObjectsParams) (*models.ListObjectsResponse, error) {
    // Create anonymous Object Storage client
    client, err := newAnonymousMinioClient()
    if err != nil {
        return nil, err
    }

    // List objects (only works if bucket is public)
    objects, err := client.listObjects(ctx, bucketName, object storage.ListObjectsOptions{
        Prefix:    prefix,
        Recursive: true,
    })
    if err != nil {
        return nil, err
    }

    return &models.ListObjectsResponse{
        Objects: objects,
    }, nil
}
```

## 🛡️ Authorization (IAM Policies)

### IAM Policy Structure

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:PutObject",
        "s3:DeleteObject"
      ],
      "Resource": [
        "arn:aws:s3:::my-bucket/*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::my-bucket"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "admin:ServerInfo"
      ]
    }
  ]
}
```

### Policy Evaluation

```go
// Object Storage evaluates policies for each request

// 1. User requests operation
//    GET /api/v1/buckets/my-bucket/objects

// 2. Extract STS credentials from JWT

// 3. Object Storage client uses STS credentials

// 4. Object Storage server evaluates:
//    - User's attached policies
//    - Group policies (if LDAP)
//    - Bucket policies
//    - Resource policies

// 5. Decision: Allow or Deny
```

### Frontend Authorization Check

```tsx
// common/SecureComponent/SecureComponent.tsx

const usePermission = (resource: string, action: string) => {
  const permissions = useSelector(selectUserPermissions);

  // Check if user has permission for resource:action
  return permissions.some(p =>
    p.resource === resource && p.actions.includes(action)
  );
};

// Usage
const BucketActions = () => {
  return (
    <>
      <SecureComponent resource="buckets" action="read">
        <ListBucketsButton />
      </SecureComponent>

      <SecureComponent resource="buckets" action="create">
        <CreateBucketButton />
      </SecureComponent>

      <SecureComponent resource="buckets" action="delete">
        <DeleteBucketButton />
      </SecureComponent>
    </>
  );
};
```

## 🔒 Security Best Practices

### 1. Token Storage

```typescript
// ✅ Good: HttpOnly cookies + localStorage
// Cookie for CSRF protection
document.cookie = `token=${jwt}; HttpOnly; Secure; SameSite=Strict`;

// localStorage для client-side access
localStorage.setItem('token', jwt);
```

### 2. Token Expiration

```go
// Short-lived tokens (12 hours)
ExpiresAt: time.Now().Add(12 * time.Hour).Unix()

// Automatic refresh before expiration
// Frontend checks expiry and renews
```

### 3. HTTPS Only

```go
// Force HTTPS redirect
if !isHTTPS(r) && shouldRedirect() {
    redirectToHTTPS(w, r)
    return
}
```

### 4. CSRF Protection

```go
// State parameter in OAuth2 flow
state := generateRandomState()
storeState(state)

// Validate on callback
if receivedState != storedState {
    return errors.New("CSRF attack detected")
}
```

### 5. Input Validation

```go
// Validate all inputs
if !isValidUsername(username) {
    return errors.New("invalid username format")
}

if len(password) < 8 {
    return errors.New("password too short")
}
```

## 🚀 Следующие шаги

- **[05-functional-modules.md](05-functional-modules.md)** - Функциональные модули
- **[10-patterns-best-practices.md](10-patterns-best-practices.md)** - Security patterns
- **[08-testing.md](08-testing.md)** - SSO integration tests

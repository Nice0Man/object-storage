# 07. Сборка и развертывание

## 🔨 Build System (Makefile)

### Основные targets

```makefile
# Главные команды
make console          # Собрать Go binary
make assets           # Собрать frontend + embed
make swagger-gen      # Регенерировать API код
make test             # Запустить backend тесты
make test-integration # Integration тесты
make docker           # Собрать Docker image
make lint             # Линтинг
make crosscompile     # Multi-platform builds
make clean            # Очистка
```

## 🏗️ Build Process

### 1. Frontend Build

```bash
cd web-app
yarn install          # Установка зависимостей
yarn build            # Production build
```

**Процесс:**

1. TypeScript compilation
2. React optimization (minification, tree-shaking)
3. CSS processing
4. Asset optimization
5. Generate `build/` directory

**Результат:**

```
web-app/build/
├── index.html              # Entry point
├── static/
│   ├── js/
│   │   ├── main.[hash].js      # Main bundle
│   │   ├── [chunk].[hash].js   # Code-split chunks
│   │   └── *.js.map            # Source maps
│   ├── css/
│   │   ├── main.[hash].css     # Styles
│   │   └── *.css.map           # Source maps
│   └── media/
│       └── [assets]            # Images, fonts
└── asset-manifest.json     # Asset mapping
```

### 2. Asset Embedding

```go
// web-app/assets.go
package portal_ui

import "embed"

//go:embed build/*
var Assets embed.FS
```

**Преимущества:**

- ✅ Single binary deployment
- ✅ No external file dependencies
- ✅ Simplified distribution
- ✅ Version consistency

### 3. Backend Build

```makefile
# Makefile
console:
 @echo "Building Console binary to './console'"
 @GO111MODULE=on CGO_ENABLED=0 go build \
  -trimpath \
  --tags=kqueue \
  --ldflags "-s -w" \
  -o console ./cmd/console
```

**Build flags:**

- `-trimpath` - Remove file system paths from binary
- `--tags=kqueue` - Build tags для platform-specific code
- `-ldflags "-s -w"` - Strip debug info (smaller binary)
- `CGO_ENABLED=0` - Static linking (portable binary)

**Инжектирование build info:**

```makefile
BUILD_VERSION := $(shell git describe --exact-match --tags 2>/dev/null || git rev-parse --abbrev-ref HEAD)
BUILD_TIME := $(shell date)

go build --ldflags "-X 'github.com/minio/console/pkg.Version=$(BUILD_VERSION)' \
                     -X 'github.com/minio/console/pkg.ReleaseTime=$(BUILD_TIME)'"
```

### 4. Combined Build (assets + console)

```makefile
assets:
 @(cd web-app && yarn install && yarn build)

console: assets
 @GO111MODULE=on CGO_ENABLED=0 go build \
  -trimpath \
  --tags=kqueue \
  --ldflags "-s -w" \
  -o console ./cmd/console
```

## 🐳 Docker Build

### Dockerfile (Multi-stage)

```dockerfile
# Stage 1: Build frontend
FROM node:20-alpine AS frontend-builder
WORKDIR /app/web-app
COPY web-app/package.json web-app/yarn.lock ./
RUN yarn install --frozen-lockfile
COPY web-app/ ./
RUN yarn build

# Stage 2: Build backend
FROM golang:1.23-alpine AS backend-builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . ./
COPY --from=frontend-builder /app/web-app/build ./web-app/build
RUN CGO_ENABLED=0 go build \
    -trimpath \
    --tags=kqueue \
    --ldflags "-s -w" \
    -o console ./cmd/console

# Stage 3: Runtime
FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=backend-builder /app/console .

EXPOSE 9090 9443
ENTRYPOINT ["./console"]
CMD ["server"]
```

### Build Docker Image

```bash
# Build for current platform
make docker

# Build for multiple platforms
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t openmaxio/console:latest \
  --push \
  .
```

### Docker Compose

```yaml
# docker-compose.yml
version: '3.8'

services:
  minio:
    image: quay.io/minio/minio:latest
    command: server /data --console-address :9001
    environment:
      MINIO_ROOT_USER: minioadmin
      MINIO_ROOT_PASSWORD: minioadmin
    ports:
      - "9000:9000"
      - "9001:9001"
    volumes:
      - minio-data:/data

  console:
    image: openmaxio/console:latest
    environment:
      CONSOLE_MINIO_SERVER: http://minio:9000
      CONSOLE_PBKDF_PASSPHRASE: secret-passphrase
      CONSOLE_PBKDF_SALT: secret-salt
    ports:
      - "9090:9090"
    depends_on:
      - minio

volumes:
  minio-data:
```

## 🌐 Environment Variables

### Обязательные переменные

```bash
# MinIO connection
export CONSOLE_MINIO_SERVER=http://localhost:9000

# JWT encryption (важно!)
export CONSOLE_PBKDF_PASSPHRASE=your-secret-passphrase
export CONSOLE_PBKDF_SALT=your-secret-salt
```

### Опциональные переменные

#### Server Configuration

```bash
CONSOLE_PORT=9090                    # HTTP port
CONSOLE_TLS_PORT=9443               # HTTPS port
CONSOLE_HOSTNAME=localhost          # Server hostname
CONSOLE_SUBPATH=/                   # Subpath для reverse proxy
```

#### TLS/SSL

```bash
CONSOLE_TLS_REDIRECT=on             # Force HTTPS
CONSOLE_CERTS_DIR=~/.console/certs  # Certificate directory
```

#### Development

```bash
CONSOLE_DEV_MODE=on                 # Development mode
CONSOLE_DEBUG_LOGLEVEL=6            # Debug level (0-6)
```

#### LDAP (если используется)

```bash
CONSOLE_LDAP_ENABLED=on
```

#### Features

```bash
CONSOLE_PROMETHEUS_URL=http://prometheus:9090  # Prometheus integration
```

## 🚀 Deployment Strategies

### 1. Standalone Binary

**Простейший deployment:**

```bash
# 1. Build
make console

# 2. Configure
export CONSOLE_MINIO_SERVER=http://localhost:9000
export CONSOLE_PBKDF_PASSPHRASE=secret
export CONSOLE_PBKDF_SALT=secret

# 3. Run
./console server

# Access: http://localhost:9090
```

**Преимущества:**

- ✅ Нет dependencies
- ✅ Быстрый старт
- ✅ Простая отладка

### 2. Systemd Service

**Unit file:** `/etc/systemd/system/console.service`

```ini
[Unit]
Description=OpenMaxIO Console Server
After=network.target minio.service
Wants=minio.service

[Service]
Type=simple
User=minio
Group=minio
EnvironmentFile=/etc/default/console
ExecStart=/usr/local/bin/console server
Restart=on-failure
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

**Environment file:** `/etc/default/console`

```bash
CONSOLE_MINIO_SERVER=http://localhost:9000
CONSOLE_PBKDF_PASSPHRASE=secret-passphrase
CONSOLE_PBKDF_SALT=secret-salt
CONSOLE_PORT=9090
```

**Управление:**

```bash
sudo systemctl enable console
sudo systemctl start console
sudo systemctl status console
sudo journalctl -u console -f  # Logs
```

### 3. Docker Deployment

```bash
# Run Console container
docker run -d \
  --name console \
  -p 9090:9090 \
  -e CONSOLE_MINIO_SERVER=http://minio:9000 \
  -e CONSOLE_PBKDF_PASSPHRASE=secret \
  -e CONSOLE_PBKDF_SALT=secret \
  openmaxio/console:latest

# View logs
docker logs -f console
```

### 4. Kubernetes Deployment

**Deployment:**

```yaml
# console-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: console
  namespace: minio
spec:
  replicas: 2
  selector:
    matchLabels:
      app: console
  template:
    metadata:
      labels:
        app: console
    spec:
      containers:
      - name: console
        image: openmaxio/console:latest
        ports:
        - containerPort: 9090
        env:
        - name: CONSOLE_MINIO_SERVER
          value: "http://minio:9000"
        - name: CONSOLE_PBKDF_PASSPHRASE
          valueFrom:
            secretKeyRef:
              name: console-secret
              key: passphrase
        - name: CONSOLE_PBKDF_SALT
          valueFrom:
            secretKeyRef:
              name: console-secret
              key: salt
        livenessProbe:
          httpGet:
            path: /api/v1/health
            port: 9090
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/v1/health
            port: 9090
          initialDelaySeconds: 5
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: console
  namespace: minio
spec:
  selector:
    app: console
  ports:
  - port: 9090
    targetPort: 9090
  type: LoadBalancer
---
apiVersion: v1
kind: Secret
metadata:
  name: console-secret
  namespace: minio
type: Opaque
stringData:
  passphrase: your-secret-passphrase
  salt: your-secret-salt
```

**Ingress (с TLS):**

```yaml
# console-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: console
  namespace: minio
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  tls:
  - hosts:
    - console.example.com
    secretName: console-tls
  rules:
  - host: console.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: console
            port:
              number: 9090
```

**Deploy:**

```bash
kubectl apply -f console-deployment.yaml
kubectl apply -f console-ingress.yaml
```

### 5. Reverse Proxy (Nginx)

**Nginx configuration:**

```nginx
# /etc/nginx/sites-available/console
upstream console_backend {
    server localhost:9090;
}

server {
    listen 80;
    server_name console.example.com;

    # Redirect HTTP to HTTPS
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name console.example.com;

    # SSL certificates
    ssl_certificate /etc/letsencrypt/live/console.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/console.example.com/privkey.pem;

    # SSL optimization
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;  # codespell:ignore aNULL
    ssl_prefer_server_ciphers on;

    # Proxy settings
    location / {
        proxy_pass http://console_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # Timeouts
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    # Static assets caching
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        proxy_pass http://console_backend;
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

**Subpath configuration:**

```nginx
# Running console under /console/ subpath
location /console/ {
    proxy_pass http://console_backend/;
    proxy_set_header Host $host;
    # ... остальные headers
}
```

**Console environment:**

```bash
CONSOLE_SUBPATH=/console/
```

## 🔒 TLS/SSL Configuration

### Certificate Setup

**Directory structure:**

```
~/.console/certs/
├── public.crt           # Default certificate
├── private.key          # Default private key
├── example.com/         # Domain-specific cert
│   ├── public.crt
│   └── private.key
└── CAs/                 # CA certificates для MinIO
    └── ca.crt
```

### Self-signed Certificate (Development)

```bash
# Generate self-signed certificate
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout ~/.console/certs/private.key \
  -out ~/.console/certs/public.crt \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"
```

### Let's Encrypt (Production)

```bash
# Install certbot
sudo apt-get install certbot

# Get certificate
sudo certbot certonly --standalone \
  -d console.example.com \
  --email admin@example.com \
  --agree-tos

# Copy to Console certs directory
sudo cp /etc/letsencrypt/live/console.example.com/fullchain.pem \
  ~/.console/certs/public.crt
sudo cp /etc/letsencrypt/live/console.example.com/privkey.pem \
  ~/.console/certs/private.key
```

## 📊 Monitoring & Health Checks

### Health Check Endpoint

```bash
# Check if Console is healthy
curl http://localhost:9090/api/v1/health

# Response:
# HTTP/1.1 200 OK
```

### Prometheus Metrics

Console может экспонировать метрики через MinIO:

```bash
# MinIO Prometheus endpoint
curl http://localhost:9000/minio/v2/metrics/cluster
```

### Logging

**Console logs:**

```bash
# With systemd
sudo journalctl -u console -f

# Docker
docker logs -f console

# Direct binary
CONSOLE_DEBUG_LOGLEVEL=6 ./console server
```

## 🔧 Troubleshooting

### Common Issues

#### 1. Cannot connect to MinIO

```bash
# Check MinIO is running
curl http://localhost:9000/minio/health/live

# Verify environment variable
echo $CONSOLE_MINIO_SERVER

# Check network connectivity
telnet localhost 9000
```

#### 2. JWT encryption errors

```bash
# Ensure secrets are set
echo $CONSOLE_PBKDF_PASSPHRASE
echo $CONSOLE_PBKDF_SALT

# They must be non-empty and consistent
```

#### 3. TLS certificate errors

```bash
# Check certificate files exist
ls -la ~/.console/certs/

# Verify certificate validity
openssl x509 -in ~/.console/certs/public.crt -text -noout

# Test TLS connection
openssl s_client -connect localhost:9443
```

#### 4. WebSocket connection fails

```bash
# Check if upgrade header is preserved by proxy
# Nginx needs:
proxy_http_version 1.1;
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

## 🚀 Production Checklist

- [ ] Use strong PBKDF2 passphrase and salt
- [ ] Enable TLS/HTTPS
- [ ] Use valid SSL certificates (Let's Encrypt)
- [ ] Set up reverse proxy (Nginx/Traefik)
- [ ] Configure firewall rules
- [ ] Set up log rotation
- [ ] Configure health checks
- [ ] Set up monitoring (Prometheus/Grafana)
- [ ] Configure backup strategy
- [ ] Document deployment procedure
- [ ] Set up CI/CD pipeline
- [ ] Test disaster recovery

## 🎯 Следующие шаги

- **[08-testing.md](08-testing.md)** - Тестирование
- **[12-advanced-topics.md](12-advanced-topics.md)** - HA, scaling
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика deployment

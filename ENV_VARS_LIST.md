# Environment Variables for Object Storage Console

## Required Environment Variables

### Server Configuration
- `CONSOLE_HOST` - Server bind address (default: 0.0.0.0)
- `CONSOLE_PORT` - Server port (default: 9090)
- `CONSOLE_THREADS` - Number of worker threads (default: CPU count)
- `CONSOLE_LOG_LEVEL` - Logging level: debug/info/warn/error (default: info)

### Storage Configuration  
- `STORAGE_ROOT_PATH` - Root directory for file storage (default: ./storage)

### Database Configuration
- `DATABASE_PATH` - SQLite database file path (default: console.db)

### CORS Configuration
- `CORS_ALLOWED_ORIGINS` - Comma-separated list of allowed origins (default: http://localhost:3000)
- `CORS_ALLOWED_METHODS` - Allowed HTTP methods (default: GET,POST,PUT,DELETE,OPTIONS)
- `CORS_ALLOWED_HEADERS` - Allowed HTTP headers (default: Content-Type,Authorization)

### Authentication
- `JWT_SECRET` - **REQUIRED** Secret key for JWT token signing (no default, must be set!)
- `JWT_EXPIRATION` - JWT token expiration time in seconds (default: 86400 = 24h)

### Default Admin Account
- `DEFAULT_ADMIN_USERNAME` - Default admin username (default: admin)
- `DEFAULT_ADMIN_PASSWORD` - **IMPORTANT** Default admin password (default: changeme - CHANGE THIS!)
- `DEFAULT_ADMIN_ACCOUNT_NAME` - Display name for admin account (default: Administrator)
- `DEFAULT_ADMIN_ENABLED` - Enable default admin account creation (default: true)

### S3 Configuration (optional, for future MinIO integration)
- `S3_ENDPOINT` - MinIO/S3 endpoint URL
- `S3_ACCESS_KEY` - S3 access key
- `S3_SECRET_KEY` - S3 secret key  
- `S3_REGION` - S3 region (default: us-east-1)
- `S3_USE_SSL` - Use SSL for S3 connections (default: true)

## Security Recommendations

1. **ALWAYS** set `JWT_SECRET` to a strong random value in production
2. **ALWAYS** change `DEFAULT_ADMIN_PASSWORD` from default value
3. Consider disabling default admin by setting `DEFAULT_ADMIN_ENABLED=false` after creating other users
4. Use environment-specific config files or secrets management (Kubernetes secrets, AWS Secrets Manager, etc.)

## Example .env file

```bash
# Server
CONSOLE_HOST=0.0.0.0
CONSOLE_PORT=9090

# Storage
STORAGE_ROOT_PATH=/var/lib/object-storage/data
DATABASE_PATH=/var/lib/object-storage/console.db

# CORS
CORS_ALLOWED_ORIGINS=https://console.example.com,https://app.example.com
CORS_ALLOWED_METHODS=GET,POST,PUT,DELETE,OPTIONS,PATCH
CORS_ALLOWED_HEADERS=Content-Type,Authorization,X-Requested-With

# Security - CHANGE THESE!
JWT_SECRET=$(openssl rand -hex 32)
DEFAULT_ADMIN_USERNAME=admin
DEFAULT_ADMIN_PASSWORD=$(openssl rand -base64 24)

# Logging
CONSOLE_LOG_LEVEL=info
```

## Docker Example

```bash
docker run -d \
  -p 9090:9090 \
  -e JWT_SECRET="your-super-secret-key-here" \
  -e DEFAULT_ADMIN_PASSWORD="your-secure-password" \
  -e CORS_ALLOWED_ORIGINS="https://your-domain.com" \
  -v /path/to/storage:/var/lib/object-storage/data \
  -v /path/to/db:/var/lib/object-storage/db \
  object-storage-console
```

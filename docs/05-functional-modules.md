# 05. Функциональные модули

## 📦 Bucket Management

### Операции с Buckets

| Операция | Endpoint | Метод | Описание |
|----------|----------|-------|----------|
| List Buckets | `/api/v1/buckets` | GET | Список всех buckets |
| Create Bucket | `/api/v1/buckets` | POST | Создание нового bucket |
| Delete Bucket | `/api/v1/buckets/{name}` | DELETE | Удаление bucket |
| Get Bucket Info | `/api/v1/buckets/{name}` | GET | Детальная информация |
| Set Bucket Policy | `/api/v1/buckets/{name}/policy` | PUT | Установка IAM policy |
| Get Bucket Policy | `/api/v1/buckets/{name}/policy` | GET | Получение policy |
| Set Bucket Quota | `/api/v1/buckets/{name}/quota` | PUT | Установка квоты |
| Get Bucket Quota | `/api/v1/buckets/{name}/quota` | GET | Получение квоты |
| Set Bucket Versioning | `/api/v1/buckets/{name}/versioning` | PUT | Включение versioning |
| Get Bucket Versioning | `/api/v1/buckets/{name}/versioning` | GET | Статус versioning |
| Set Bucket Encryption | `/api/v1/buckets/{name}/encryption` | POST | Настройка шифрования |
| Get Bucket Encryption | `/api/v1/buckets/{name}/encryption` | GET | Конфигурация шифрования |
| Set Object Locking | `/api/v1/buckets/{name}/object-locking` | PUT | WORM настройки |
| Get Object Locking | `/api/v1/buckets/{name}/object-locking` | GET | WORM конфигурация |
| Set Bucket Tags | `/api/v1/buckets/{name}/tags` | PUT | Теги bucket |
| Get Bucket Tags | `/api/v1/buckets/{name}/tags` | GET | Получение тегов |
| Set Bucket Replication | `/api/v1/buckets/{name}/replication` | PUT | Настройка репликации |
| Get Bucket Replication | `/api/v1/buckets/{name}/replication` | GET | Конфигурация репликации |

### Backend Implementation: api/user_buckets.go

```go
// List buckets
func getListBucketsResponse(session *models.Principal, params bucket.ListBucketsParams) (*models.ListBucketsResponse, error) {
    ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
    defer cancel()
    
    mClient, err := newMinioClient(session)
    if err != nil {
        return nil, err
    }
    
    buckets, err := mClient.listBucketsWithContext(ctx)
    if err != nil {
        return nil, err
    }
    
    var bucketList []*models.Bucket
    for _, bucket := range buckets {
        // Get bucket size and object count (parallel)
        size, objectsCount := getBucketStats(ctx, mClient, bucket.Name)
        
        bucketList = append(bucketList, &models.Bucket{
            Name:         swag.String(bucket.Name),
            CreationDate: bucket.CreationDate.Format(time.RFC3339),
            Size:         size,
            Objects:      objectsCount,
        })
    }
    
    return &models.ListBucketsResponse{
        Buckets: bucketList,
        Total:   int64(len(bucketList)),
    }, nil
}

// Create bucket
func createBucket(session *models.Principal, params bucket.MakeBucketParams) error {
    ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
    defer cancel()
    
    mClient, err := newMinioClient(session)
    if err != nil {
        return err
    }
    
    // Validate bucket name
    if !isValidBucketName(*params.Body.Name) {
        return errors.New("invalid bucket name")
    }
    
    // Create bucket
    err = mClient.makeBucketWithContext(
        ctx,
        *params.Body.Name,
        params.Body.Region,
        params.Body.Locking != nil && *params.Body.Locking,
    )
    if err != nil {
        return err
    }
    
    // Set bucket policy if provided
    if params.Body.Policy != nil {
        err = mClient.setBucketPolicyWithContext(ctx, *params.Body.Name, *params.Body.Policy)
        if err != nil {
            return err
        }
    }
    
    return nil
}
```

### Frontend Implementation

```tsx
// BucketsList.tsx
const BucketsList = () => {
  const [buckets, setBuckets] = useState<Bucket[]>([]);
  const [loading, setLoading] = useState(true);
  
  useEffect(() => {
    fetchBuckets();
  }, []);
  
  const fetchBuckets = async () => {
    try {
      setLoading(true);
      const api = new ConsoleApi();
      const response = await api.listBuckets();
      setBuckets(response.data.buckets || []);
    } catch (error) {
      console.error('Failed to fetch buckets:', error);
    } finally {
      setLoading(false);
    }
  };
  
  return (
    <div>
      <Button onClick={() => setShowCreateModal(true)}>
        Create Bucket
      </Button>
      {loading ? (
        <LoadingComponent />
      ) : (
        <BucketGrid buckets={buckets} />
      )}
    </div>
  );
};
```

### Bucket Features

#### 1. Versioning
```go
// Enable versioning
func setBucketVersioning(ctx context.Context, client MinioClient, bucketName string, enabled bool) error {
    config := minio.BucketVersioningConfiguration{
        Status: "Enabled",
    }
    if !enabled {
        config.Status = "Suspended"
    }
    
    return client.setBucketVersioning(ctx, bucketName, config)
}
```

#### 2. Object Locking (WORM)
```go
// Set object locking
func setObjectLocking(ctx context.Context, client MinioClient, bucketName string, mode string, days int) error {
    retentionMode := minio.Governance
    if mode == "compliance" {
        retentionMode = minio.Compliance
    }
    
    validity := uint(days)
    unit := minio.Days
    
    return client.setObjectLockConfig(ctx, bucketName, &retentionMode, &validity, &unit)
}
```

#### 3. Encryption
```go
// Set bucket encryption
func setBucketEncryption(ctx context.Context, client MinioClient, bucketName, algorithm string) error {
    config := &sse.Configuration{
        Rules: []sse.Rule{
            {
                ApplyServerSideEncryptionByDefault: sse.ApplySSEByDefault{
                    SSEAlgorithm: algorithm, // AES256 or aws:kms
                },
            },
        },
    }
    
    return client.setBucketEncryption(ctx, bucketName, config)
}
```

## 📄 Object Management

### Операции с Objects

| Операция | Endpoint | Метод | Описание |
|----------|----------|-------|----------|
| List Objects | `/api/v1/buckets/{bucket}/objects` | GET | Список объектов |
| Upload Object | `/api/v1/buckets/{bucket}/objects/upload` | POST | Загрузка файла |
| Download Object | `/api/v1/buckets/{bucket}/objects/download` | GET | Скачивание файла |
| Delete Object | `/api/v1/buckets/{bucket}/objects` | DELETE | Удаление объекта |
| Delete Multiple | `/api/v1/buckets/{bucket}/objects/delete-multiple` | POST | Массовое удаление |
| Copy Object | `/api/v1/buckets/{bucket}/objects/copy` | POST | Копирование |
| Move Object | `/api/v1/buckets/{bucket}/objects/move` | POST | Перемещение |
| Get Object Info | `/api/v1/buckets/{bucket}/objects/info` | GET | Метаданные |
| Set Object Tags | `/api/v1/buckets/{bucket}/objects/tags` | PUT | Установка тегов |
| Get Object Tags | `/api/v1/buckets/{bucket}/objects/tags` | GET | Получение тегов |
| Set Retention | `/api/v1/buckets/{bucket}/objects/retention` | PUT | Установка retention |
| Get Retention | `/api/v1/buckets/{bucket}/objects/retention` | GET | Получение retention |
| Set Legal Hold | `/api/v1/buckets/{bucket}/objects/legal-hold` | PUT | Legal hold |
| Get Legal Hold | `/api/v1/buckets/{bucket}/objects/legal-hold` | GET | Статус legal hold |
| Share Object | `/api/v1/buckets/{bucket}/objects/share` | GET | Presigned URL |

### Backend Implementation: api/user_objects.go

#### List Objects
```go
func listObjects(session *models.Principal, params object.ListObjectsParams) (*models.ListObjectsResponse, error) {
    ctx, cancel := context.WithTimeout(context.Background(), 60*time.Second)
    defer cancel()
    
    mClient, err := newMinioClient(session)
    if err != nil {
        return nil, err
    }
    
    // List options
    opts := minio.ListObjectsOptions{
        Prefix:    params.Prefix,
        Recursive: params.Recursive != nil && *params.Recursive,
        WithVersions: params.WithVersions != nil && *params.WithVersions,
    }
    
    // List objects
    objectCh := mClient.listObjects(ctx, params.BucketName, opts)
    
    var objects []*models.BucketObject
    for object := range objectCh {
        if object.Err != nil {
            return nil, object.Err
        }
        
        objects = append(objects, &models.BucketObject{
            Name:         swag.String(object.Key),
            Size:         object.Size,
            LastModified: object.LastModified.Format(time.RFC3339),
            ContentType:  object.ContentType,
            VersionID:    object.VersionID,
            IsLatest:     object.IsLatest,
        })
    }
    
    return &models.ListObjectsResponse{
        Objects: objects,
        Total:   int64(len(objects)),
    }, nil
}
```

#### Upload Object
```go
func uploadObject(session *models.Principal, params object.UploadObjectParams) error {
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Minute)
    defer cancel()
    
    mClient, err := newMinioClient(session)
    if err != nil {
        return err
    }
    
    // Parse multipart form
    err = params.HTTPRequest.ParseMultipartForm(10 << 20) // 10 MB
    if err != nil {
        return err
    }
    
    file, handler, err := params.HTTPRequest.FormFile("file")
    if err != nil {
        return err
    }
    defer file.Close()
    
    // Upload options
    opts := minio.PutObjectOptions{
        ContentType: handler.Header.Get("Content-Type"),
        UserMetadata: map[string]string{
            "uploaded-by": session.AccountAccessKey,
        },
    }
    
    // Upload to MinIO
    _, err = mClient.putObject(
        ctx,
        params.BucketName,
        params.Prefix + handler.Filename,
        file,
        handler.Size,
        opts,
    )
    
    return err
}
```

#### Download Object
```go
func downloadObject(session *models.Principal, params object.DownloadObjectParams) (io.ReadCloser, error) {
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Minute)
    defer cancel()
    
    mClient, err := newMinioClient(session)
    if err != nil {
        return nil, err
    }
    
    // Get object
    opts := minio.GetObjectOptions{}
    if params.VersionID != nil {
        opts.VersionID = *params.VersionID
    }
    
    object, err := mClient.getObject(ctx, params.BucketName, params.Prefix, opts)
    if err != nil {
        return nil, err
    }
    
    return object, nil
}
```

### Frontend Implementation

#### Upload with Progress
```tsx
const ObjectUpload = ({ bucketName, prefix }: Props) => {
  const [uploadProgress, setUploadProgress] = useState<Record<string, number>>({});
  
  const uploadFile = async (file: File) => {
    const formData = new FormData();
    formData.append('file', file);
    
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      
      xhr.upload.onprogress = (event) => {
        if (event.lengthComputable) {
          const progress = (event.loaded / event.total) * 100;
          setUploadProgress(prev => ({
            ...prev,
            [file.name]: progress,
          }));
        }
      };
      
      xhr.onload = () => {
        if (xhr.status === 200) {
          resolve(xhr.response);
        } else {
          reject(new Error('Upload failed'));
        }
      };
      
      xhr.onerror = () => reject(new Error('Network error'));
      
      xhr.open('POST', `/api/v1/buckets/${bucketName}/objects/upload?prefix=${prefix}`);
      xhr.setRequestHeader('Authorization', `Bearer ${getToken()}`);
      xhr.send(formData);
    });
  };
  
  const { getRootProps, getInputProps } = useDropzone({
    onDrop: async (files) => {
      for (const file of files) {
        try {
          await uploadFile(file);
        } catch (error) {
          console.error(`Failed to upload ${file.name}:`, error);
        }
      }
    },
  });
  
  return (
    <div {...getRootProps()}>
      <input {...getInputProps()} />
      <p>Drag & drop files here</p>
      {Object.entries(uploadProgress).map(([filename, progress]) => (
        <div key={filename}>
          <span>{filename}</span>
          <ProgressBar value={progress} />
        </div>
      ))}
    </div>
  );
};
```

## 👥 User & Group Management

### User Operations

| Операция | Endpoint | Метод | Описание |
|----------|----------|-------|----------|
| List Users | `/api/v1/users` | GET | Список пользователей |
| Create User | `/api/v1/users` | POST | Создание пользователя |
| Delete User | `/api/v1/users/{name}` | DELETE | Удаление |
| Get User Info | `/api/v1/users/{name}` | GET | Информация о пользователе |
| Update User | `/api/v1/users/{name}` | PUT | Обновление |
| Set User Policy | `/api/v1/users/{name}/policies` | PUT | Назначение политик |
| Get User Groups | `/api/v1/users/{name}/groups` | GET | Группы пользователя |
| Update User Groups | `/api/v1/users/{name}/groups` | PUT | Обновление групп |
| Change Password | `/api/v1/users/{name}/password` | PUT | Смена пароля |
| List Service Accounts | `/api/v1/users/{name}/service-accounts` | GET | Service accounts |
| Create Service Account | `/api/v1/users/{name}/service-accounts` | POST | Создание SA |

### Backend Implementation

```go
// Create user
func addUser(ctx context.Context, client AdminClient, accessKey, secretKey string) error {
    // Validate credentials
    if len(accessKey) < 3 || len(secretKey) < 8 {
        return errors.New("invalid credentials format")
    }
    
    // Create user in MinIO
    err := client.addUser(ctx, accessKey, secretKey)
    if err != nil {
        return err
    }
    
    return nil
}

// Set user policy
func setUserPolicy(ctx context.Context, client AdminClient, username, policyName string) error {
    return client.setPolicy(ctx, policyName, username, false)
}

// Update user groups
func updateUserGroups(ctx context.Context, client AdminClient, username string, groups []string) error {
    // Remove from all existing groups
    existingGroups, err := getUserGroups(ctx, client, username)
    if err != nil {
        return err
    }
    
    for _, group := range existingGroups {
        err = client.updateGroupMembers(ctx, madmin.GroupAddRemove{
            Group:   group,
            Members: []string{username},
            IsRemove: true,
        })
        if err != nil {
            return err
        }
    }
    
    // Add to new groups
    for _, group := range groups {
        err = client.updateGroupMembers(ctx, madmin.GroupAddRemove{
            Group:   group,
            Members: []string{username},
        })
        if err != nil {
            return err
        }
    }
    
    return nil
}
```

### Group Operations

| Операция | Endpoint | Метод | Описание |
|----------|----------|-------|----------|
| List Groups | `/api/v1/groups` | GET | Список групп |
| Create Group | `/api/v1/groups` | POST | Создание группы |
| Delete Group | `/api/v1/groups/{name}` | DELETE | Удаление |
| Get Group Info | `/api/v1/groups/{name}` | GET | Информация |
| Update Group | `/api/v1/groups/{name}` | PUT | Обновление |
| Set Group Policy | `/api/v1/groups/{name}/policy` | PUT | Назначение политики |

## 📋 Policy Management

### Policy Operations

| Операция | Endpoint | Метод | Описание |
|----------|----------|-------|----------|
| List Policies | `/api/v1/policies` | GET | Список политик |
| Create Policy | `/api/v1/policies` | POST | Создание политики |
| Delete Policy | `/api/v1/policies/{name}` | DELETE | Удаление |
| Get Policy | `/api/v1/policies/{name}` | GET | Получение политики |
| Update Policy | `/api/v1/policies/{name}` | PUT | Обновление |

### Backend Implementation

```go
// Create policy
func addPolicy(ctx context.Context, client AdminClient, policyName, policyJSON string) error {
    // Validate policy JSON
    var policy iampolicy.Policy
    err := json.Unmarshal([]byte(policyJSON), &policy)
    if err != nil {
        return errors.New("invalid policy JSON")
    }
    
    // Validate policy structure
    if policy.Version != "2012-10-17" {
        return errors.New("invalid policy version")
    }
    
    // Create policy in MinIO
    err = client.addCannedPolicy(ctx, policyName, policyJSON)
    if err != nil {
        return err
    }
    
    return nil
}
```

### Policy Examples

#### Read-Only Policy
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::my-bucket/*",
        "arn:aws:s3:::my-bucket"
      ]
    }
  ]
}
```

#### Read-Write Policy
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:*"
      ],
      "Resource": [
        "arn:aws:s3:::my-bucket/*",
        "arn:aws:s3:::my-bucket"
      ]
    }
  ]
}
```

## ⚙️ Configuration Management

### Configuration Areas

| Область | Endpoint | Описание |
|---------|----------|----------|
| Server Config | `/api/v1/configs` | Общая конфигурация |
| Notification Targets | `/api/v1/notification-endpoints` | Webhook, AMQP, Redis, etc. |
| IDP Config | `/api/v1/idp/config` | Identity Provider |
| KMS Config | `/api/v1/kms/status` | Key Management |
| Site Replication | `/api/v1/admin/site-replication` | Multi-site setup |

### Backend Implementation

```go
// Get server configuration
func getConfig(ctx context.Context, client AdminClient) ([]byte, error) {
    config, err := client.getConfig(ctx)
    if err != nil {
        return nil, err
    }
    
    return config, nil
}

// Set server configuration
func setConfig(ctx context.Context, client AdminClient, configData []byte) error {
    reader := bytes.NewReader(configData)
    restart, err := client.setConfig(ctx, reader)
    if err != nil {
        return err
    }
    
    if restart {
        // Notify that server restart is required
        logger.Info("Server restart required for configuration changes")
    }
    
    return nil
}
```

## 🔔 Notification & Events

### Notification Targets

Поддерживаемые targets:
- **Webhook** - HTTP callback
- **AMQP** - RabbitMQ, etc.
- **Redis** - Pub/Sub
- **NATS** - Messaging system
- **PostgreSQL** - Database notifications
- **MySQL** - Database notifications
- **Kafka** - Message broker
- **Elasticsearch** - Search engine

### Backend Implementation

```go
// Set bucket notification
func setBucketNotification(ctx context.Context, client MinioClient, bucketName string, config notification.Configuration) error {
    return client.setBucketNotification(ctx, bucketName, config)
}

// Example: Webhook notification
config := notification.Configuration{
    QueueConfigs: []notification.Queue{
        {
            Queue: notification.Arn{
                Partition: "minio",
                Service:   "webhook",
                Region:    "",
                AccountID: "1",
            },
            Events: []notification.EventType{
                notification.ObjectCreatedAll,
                notification.ObjectRemovedAll,
            },
            Prefix: "uploads/",
            Suffix: ".jpg",
        },
    },
}
```

## 🚀 Следующие шаги

- **[06-websocket-architecture.md](06-websocket-architecture.md)** - WebSocket для real-time
- **[12-advanced-topics.md](12-advanced-topics.md)** - Site replication, monitoring
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика


# 03. Frontend архитектура (React + TypeScript)

## 🎨 Структура Frontend

```
web-app/
├── src/
│   ├── api/              # Generated API client
│   ├── screens/          # Page components
│   ├── common/           # Shared components
│   ├── utils/            # Utility functions
│   ├── websockets/       # WebSocket client
│   ├── store.ts          # Redux store
│   ├── systemSlice.ts    # Redux slice
│   ├── MainRouter.tsx    # Route configuration
│   ├── ProtectedRoutes.tsx  # Auth guards
│   └── index.tsx         # Entry point
├── public/               # Static assets
├── build/                # Production build
├── tests/                # Test scenarios
├── e2e/                  # E2E tests
├── playwright/           # Playwright config
├── package.json          # Dependencies
└── tsconfig.json         # TypeScript config
```

## 🚀 Entry Point: index.tsx

```tsx
import React from 'react';
import ReactDOM from 'react-dom/client';
import { Provider } from 'react-redux';
import { BrowserRouter } from 'react-router-dom';
import { store } from './store';
import MainRouter from './MainRouter';
import './index.css';

const root = ReactDOM.createRoot(
  document.getElementById('root') as HTMLElement
);

root.render(
  <React.StrictMode>
    <Provider store={store}>
      <BrowserRouter>
        <MainRouter />
      </BrowserRouter>
    </Provider>
  </React.StrictMode>
);
```

## 🗺️ Routing: MainRouter.tsx

```tsx
import { Routes, Route, Navigate } from 'react-router-dom';
import LoginPage from './screens/LoginPage';
import Console from './screens/Console';
import ProtectedRoutes from './ProtectedRoutes';

const MainRouter = () => {
  return (
    <Routes>
      {/* Public routes */}
      <Route path="/login" element={<LoginPage />} />

      {/* Protected routes */}
      <Route element={<ProtectedRoutes />}>
        <Route path="/console/*" element={<Console />} />
      </Route>

      {/* Default redirect */}
      <Route path="*" element={<Navigate to="/console" replace />} />
    </Routes>
  );
};
```

### ProtectedRoutes.tsx - Auth Guard

```tsx
import { Navigate, Outlet } from 'react-router-dom';
import { useSelector } from 'react-redux';
import { selectIsLoggedIn } from './systemSlice';

const ProtectedRoutes = () => {
  const isLoggedIn = useSelector(selectIsLoggedIn);

  if (!isLoggedIn) {
    return <Navigate to="/login" replace />;
  }

  return <Outlet />;
};
```

## 🏗️ State Management: Redux

### store.ts - Redux Store Configuration

```tsx
import { configureStore } from '@reduxjs/toolkit';
import systemReducer from './systemSlice';

export const store = configureStore({
  reducer: {
    system: systemReducer,
    // Другие slices могут быть добавлены
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
```

### systemSlice.ts - System State

```tsx
import { createSlice, PayloadAction } from '@reduxjs/toolkit';

interface SystemState {
  isLoggedIn: boolean;
  user: User | null;
  sessionExpiry: string | null;
  features: string[];
  serverNeedsRestart: boolean;
  distributedSetup: boolean;
}

const initialState: SystemState = {
  isLoggedIn: false,
  user: null,
  sessionExpiry: null,
  features: [],
  serverNeedsRestart: false,
  distributedSetup: false,
};

export const systemSlice = createSlice({
  name: 'system',
  initialState,
  reducers: {
    setLogin: (state, action: PayloadAction<boolean>) => {
      state.isLoggedIn = action.payload;
    },
    setUser: (state, action: PayloadAction<User>) => {
      state.user = action.payload;
    },
    setSessionExpiry: (state, action: PayloadAction<string>) => {
      state.sessionExpiry = action.payload;
    },
    logout: (state) => {
      state.isLoggedIn = false;
      state.user = null;
      state.sessionExpiry = null;
    },
  },
});

export const { setLogin, setUser, setSessionExpiry, logout } = systemSlice.actions;
export const selectIsLoggedIn = (state: RootState) => state.system.isLoggedIn;
export default systemSlice.reducer;
```

## 📱 Screens Structure

```
screens/
├── LoginPage/
│   ├── LoginPage.tsx           # Login form
│   └── styles.tsx              # Styled components
├── LogoutPage/
│   └── LogoutPage.tsx          # Logout handler
├── Console/
│   ├── Console.tsx             # Main console layout
│   ├── Dashboard/              # Dashboard view
│   ├── Buckets/                # Bucket management
│   │   ├── BucketsList.tsx
│   │   ├── BucketDetails.tsx
│   │   └── CreateBucket.tsx
│   ├── Objects/                # Object browser
│   │   ├── ObjectsList.tsx
│   │   ├── ObjectDetails.tsx
│   │   └── UploadObjects.tsx
│   ├── Users/                  # User management
│   ├── Groups/                 # Group management
│   ├── Policies/               # Policy management
│   ├── Configuration/          # Server configuration
│   ├── Logs/                   # Real-time logs
│   └── Monitoring/             # System monitoring
└── AnonymousAccess/            # Public bucket access
```

### Console.tsx - Main Layout

```tsx
import { Routes, Route } from 'react-router-dom';
import Navigation from './Navigation';
import Dashboard from './Dashboard';
import Buckets from './Buckets';
import Users from './Users';
// ...

const Console = () => {
  return (
    <div className="console-container">
      <Navigation />
      <main className="console-content">
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route path="/buckets/*" element={<Buckets />} />
          <Route path="/users/*" element={<Users />} />
          {/* ... другие маршруты */}
        </Routes>
      </main>
    </div>
  );
};
```

## 🔌 API Integration

### Generated API Client: api/consoleApi.ts

**Генерируется автоматически из swagger.yml:**

```bash
npx swagger-typescript-api \
  -p ./swagger.yml \
  -o ./web-app/src/api \
  -n consoleApi.ts \
  --custom-config generator.config.js
```

**Результат:**

```typescript
// api/consoleApi.ts (generated)
export class ConsoleApi {
  http: HttpClient;

  constructor(http: HttpClient) {
    this.http = http;
  }

  // Auth endpoints
  login = (data: LoginRequest, params?: RequestParams) =>
    this.http.request<void, ApiError>({
      path: `/api/v1/login`,
      method: "POST",
      body: data,
      type: ContentType.Json,
      ...params,
    });

  logout = (data: LogoutRequest, params?: RequestParams) =>
    this.http.request<void, ApiError>({
      path: `/api/v1/logout`,
      method: "POST",
      body: data,
      ...params,
    });

  sessionCheck = (params?: RequestParams) =>
    this.http.request<SessionResponse, ApiError>({
      path: `/api/v1/session`,
      method: "GET",
      secure: true,
      ...params,
    });

  // Bucket endpoints
  listBuckets = (params?: RequestParams) =>
    this.http.request<ListBucketsResponse, ApiError>({
      path: `/api/v1/buckets`,
      method: "GET",
      secure: true,
      ...params,
    });

  makeBucket = (data: MakeBucketRequest, params?: RequestParams) =>
    this.http.request<MakeBucketsResponse, ApiError>({
      path: `/api/v1/buckets`,
      method: "POST",
      body: data,
      secure: true,
      type: ContentType.Json,
      ...params,
    });

  // ... сотни других методов
}
```

### Использование API в компонентах

```tsx
import { useEffect, useState } from 'react';
import { ConsoleApi } from '../api/consoleApi';

const BucketsList = () => {
  const [buckets, setBuckets] = useState<Bucket[]>([]);
  const [loading, setLoading] = useState(true);
  const api = new ConsoleApi(/* http client */);

  useEffect(() => {
    const fetchBuckets = async () => {
      try {
        setLoading(true);
        const response = await api.listBuckets();
        setBuckets(response.data.buckets || []);
      } catch (error) {
        console.error('Failed to fetch buckets:', error);
      } finally {
        setLoading(false);
      }
    };

    fetchBuckets();
  }, []);

  return (
    <div>
      {loading ? (
        <LoadingComponent />
      ) : (
        <BucketList buckets={buckets} />
      )}
    </div>
  );
};
```

## 🎨 UI Components: MDS (Object Storage Design System)

### Dependency

```json
"dependencies": {
  "mds": "https://github.com/openmaxio/mds.git#a1369db"
}
```

### Примеры компонентов

```tsx
import { Button, Grid, InputBox, Box } from "mds";

const CreateBucketForm = () => {
  return (
    <Grid container>
      <Grid item xs={12}>
        <InputBox
          id="bucket-name"
          label="Bucket Name"
          placeholder="Enter bucket name"
        />
      </Grid>
      <Grid item xs={12}>
        <Box>
          <Button
            variant="callAction"
            label="Create"
            onClick={handleCreate}
          />
        </Box>
      </Grid>
    </Grid>
  );
};
```

### Styled Components

```tsx
import styled from 'styled-components';

const Container = styled.div`
  padding: 20px;
  background: #fff;
`;

const Title = styled.h2`
  font-size: 24px;
  color: #000;
  margin-bottom: 16px;
`;

const BucketCard = styled.div`
  border: 1px solid #e0e0e0;
  border-radius: 8px;
  padding: 16px;
  cursor: pointer;

  &:hover {
    background: #f5f5f5;
  }
`;
```

## 🔄 WebSocket Integration

### websockets/ - WebSocket Client

```typescript
// websockets/ws-client.ts
export class WebSocketClient {
  private ws: WebSocket | null = null;
  private url: string;
  private reconnectInterval: number = 5000;

  constructor(url: string) {
    this.url = url;
  }

  connect(onMessage: (data: any) => void) {
    this.ws = new WebSocket(this.url);

    this.ws.onopen = () => {
      console.log('WebSocket connected');
    };

    this.ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      onMessage(data);
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };

    this.ws.onclose = () => {
      console.log('WebSocket closed, reconnecting...');
      setTimeout(() => this.connect(onMessage), this.reconnectInterval);
    };
  }

  send(data: any) {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    }
  }

  disconnect() {
    this.ws?.close();
  }
}
```

### Использование WebSocket в компонентах

```tsx
const LogsViewer = () => {
  const [logs, setLogs] = useState<string[]>([]);
  const wsClient = useRef<WebSocketClient | null>(null);

  useEffect(() => {
    wsClient.current = new WebSocketClient('ws://localhost:9090/ws/console');

    wsClient.current.connect((data) => {
      setLogs((prev) => [...prev, data.message]);
    });

    return () => {
      wsClient.current?.disconnect();
    };
  }, []);

  return (
    <div className="logs-container">
      {logs.map((log, index) => (
        <div key={index} className="log-entry">{log}</div>
      ))}
    </div>
  );
};
```

## 📤 File Upload/Download

### Upload с Progress

```tsx
import { useState } from 'react';
import { useDropzone } from 'react-dropzone';

const ObjectUpload = ({ bucketName }: { bucketName: string }) => {
  const [uploadProgress, setUploadProgress] = useState<number>(0);

  const onDrop = async (files: File[]) => {
    for (const file of files) {
      const formData = new FormData();
      formData.append('file', file);

      try {
        const xhr = new XMLHttpRequest();

        xhr.upload.onprogress = (event) => {
          if (event.lengthComputable) {
            const progress = (event.loaded / event.total) * 100;
            setUploadProgress(progress);
          }
        };

        xhr.onload = () => {
          if (xhr.status === 200) {
            console.log('Upload successful');
          }
        };

        xhr.open('POST', `/api/v1/buckets/${bucketName}/objects/upload`);
        xhr.setRequestHeader('Authorization', `Bearer ${getToken()}`);
        xhr.send(formData);
      } catch (error) {
        console.error('Upload failed:', error);
      }
    }
  };

  const { getRootProps, getInputProps } = useDropzone({ onDrop });

  return (
    <div {...getRootProps()} className="dropzone">
      <input {...getInputProps()} />
      <p>Drag & drop files here, or click to select</p>
      {uploadProgress > 0 && (
        <div className="progress-bar">
          <div style={{ width: `${uploadProgress}%` }} />
        </div>
      )}
    </div>
  );
};
```

### Download

```tsx
const downloadObject = async (bucketName: string, objectPath: string) => {
  try {
    const response = await fetch(
      `/api/v1/buckets/${bucketName}/objects/download?prefix=${encodeURIComponent(objectPath)}`,
      {
        headers: {
          'Authorization': `Bearer ${getToken()}`,
        },
      }
    );

    const blob = await response.blob();
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = objectPath.split('/').pop() || 'download';
    document.body.appendChild(a);
    a.click();
    a.remove();
    window.URL.revokeObjectURL(url);
  } catch (error) {
    console.error('Download failed:', error);
  }
};
```

## 🎭 Virtual Scrolling для больших списков

```tsx
import { FixedSizeList } from 'react-window';
import InfiniteLoader from 'react-window-infinite-loader';

interface ObjectsListProps {
  objects: BucketObject[];
  hasMore: boolean;
  loadMore: () => void;
}

const ObjectsList = ({ objects, hasMore, loadMore }: ObjectsListProps) => {
  const isItemLoaded = (index: number) => !hasMore || index < objects.length;

  const loadMoreItems = () => {
    if (hasMore) {
      loadMore();
    }
  };

  const Row = ({ index, style }: { index: number; style: React.CSSProperties }) => {
    if (!isItemLoaded(index)) {
      return <div style={style}>Loading...</div>;
    }

    const object = objects[index];
    return (
      <div style={style} className="object-row">
        <span>{object.name}</span>
        <span>{object.size}</span>
        <span>{object.last_modified}</span>
      </div>
    );
  };

  return (
    <InfiniteLoader
      isItemLoaded={isItemLoaded}
      itemCount={hasMore ? objects.length + 1 : objects.length}
      loadMoreItems={loadMoreItems}
    >
      {({ onItemsRendered, ref }) => (
        <FixedSizeList
          height={600}
          itemCount={objects.length}
          itemSize={50}
          onItemsRendered={onItemsRendered}
          ref={ref}
          width="100%"
        >
          {Row}
        </FixedSizeList>
      )}
    </InfiniteLoader>
  );
};
```

## 🔒 SecureComponent - Conditional Rendering

```tsx
// common/SecureComponent/SecureComponent.tsx
interface SecureComponentProps {
  resource: string;
  action: string;
  children: React.ReactNode;
  errorProps?: {
    message: string;
  };
}

const SecureComponent = ({
  resource,
  action,
  children,
  errorProps
}: SecureComponentProps) => {
  const hasPermission = usePermission(resource, action);

  if (!hasPermission) {
    return errorProps ? (
      <div className="no-permission">
        {errorProps.message}
      </div>
    ) : null;
  }

  return <>{children}</>;
};

// Usage
<SecureComponent resource="buckets" action="create">
  <Button onClick={createBucket}>Create Bucket</Button>
</SecureComponent>
```

## 🧪 Frontend Testing

### Unit Tests (Jest + React Testing Library)

```tsx
import { render, screen, fireEvent } from '@testing-library/react';
import { Provider } from 'react-redux';
import { BrowserRouter } from 'react-router-dom';
import { store } from '../store';
import LoginPage from './LoginPage';

test('renders login form', () => {
  render(
    <Provider store={store}>
      <BrowserRouter>
        <LoginPage />
      </BrowserRouter>
    </Provider>
  );

  expect(screen.getByLabelText(/access key/i)).toBeInTheDocument();
  expect(screen.getByLabelText(/secret key/i)).toBeInTheDocument();
  expect(screen.getByRole('button', { name: /login/i })).toBeInTheDocument();
});

test('submits login form', async () => {
  render(<LoginPage />);

  const accessKeyInput = screen.getByLabelText(/access key/i);
  const secretKeyInput = screen.getByLabelText(/secret key/i);
  const submitButton = screen.getByRole('button', { name: /login/i });

  fireEvent.change(accessKeyInput, { target: { value: 'minioadmin' } });
  fireEvent.change(secretKeyInput, { target: { value: 'minioadmin' } });
  fireEvent.click(submitButton);

  // Assertions...
});
```

### E2E Tests (Playwright)

```typescript
// playwright/tests/login.spec.ts
import { test, expect } from '@playwright/test';

test('user can login', async ({ page }) => {
  await page.goto('http://localhost:5005/login');

  await page.fill('[name="accessKey"]', 'minioadmin');
  await page.fill('[name="secretKey"]', 'minioadmin');
  await page.click('button[type="submit"]');

  await expect(page).toHaveURL(/.*console/);
  await expect(page.locator('.dashboard')).toBeVisible();
});
```

## 📦 Build Process

### Development

```bash
cd web-app
yarn install
yarn start    # Runs on port 5005 with proxy to backend
```

**package.json:**

```json
{
  "proxy": "http://localhost:9090/",
  "scripts": {
    "start": "PORT=5005 react-scripts start"
  }
}
```

### Production

```bash
yarn build    # Creates optimized build in build/
```

**Результат:**

```
build/
├── index.html
├── static/
│   ├── js/
│   │   ├── main.[hash].js
│   │   └── [chunk].[hash].js
│   ├── css/
│   │   └── main.[hash].css
│   └── media/
│       └── [assets]
└── asset-manifest.json
```

### Embedding в Go

```go
// web-app/assets.go
package portal_ui

import "embed"

//go:embed build/*
var Assets embed.FS
```

## 🎯 Следующие шаги

- **[04-authentication-authorization.md](04-authentication-authorization.md)** - Аутентификация на frontend
- **[06-websocket-architecture.md](06-websocket-architecture.md)** - Детали WebSocket
- **[09-api-specification.md](09-api-specification.md)** - API спецификация

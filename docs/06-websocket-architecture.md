# 06. WebSocket архитектура

## 🔌 Обзор WebSocket в Console

OpenMaxIO Object Browser использует WebSocket для **real-time коммуникации**:

- Real-time log streaming
- Bucket event notifications
- Live metrics updates
- Upload/Download progress

## 🏗️ WebSocket Architecture

```
┌──────────┐                          ┌─────────────┐
│ Browser  │                          │   Console   │
│          │                          │   Server    │
└────┬─────┘                          └──────┬──────┘
     │                                        │
     │  1. HTTP Upgrade Request              │
     │     GET /ws/console                    │
     │     Connection: Upgrade               │
     │     Upgrade: websocket                │
     ├───────────────────────────────────────►│
     │                                        │
     │  2. Switching Protocols (101)         │
     │     HTTP/1.1 101 Switching Protocols  │
     │◄───────────────────────────────────────┤
     │                                        │
     │  3. WebSocket Connection Established  │
     │═══════════════════════════════════════►│
     │                                        │
     │  4. Bidirectional Messages            │
     │◄──────────────────────────────────────►│
     │                                        │
```

## 📡 WebSocket Endpoints

### 1. Console Logs (`/ws/console`)

**Purpose:** Real-time log streaming from MinIO server

**Flow:**

```
Client connects to /ws/console
  ↓
Server establishes MinIO trace connection
  ↓
MinIO streams logs to Console
  ↓
Console forwards to WebSocket client
  ↓
Browser displays logs in real-time
```

### 2. Bucket Watch (`/ws/watch/{bucket}`)

**Purpose:** Real-time bucket event notifications

**Events:**

- Object created (`s3:ObjectCreated:*`)
- Object removed (`s3:ObjectRemoved:*`)
- Object accessed (`s3:ObjectAccessed:*`)

## 🔧 Backend Implementation

### WebSocket Handler Registry: api/ws_handle.go

```go
package api

import (
    "net/http"
    "github.com/minio/websocket"
)

// WebSocket handlers registry
var wsHandlers = map[string]http.HandlerFunc{}

// Register WebSocket handler
func registerWSHandler(path string, handler http.HandlerFunc) {
    wsHandlers[path] = handler
}

// Initialize WebSocket handlers
func initWSHandlers() {
    registerWSHandler("/ws/console", wsConsoleLogsHandler)
    registerWSHandler("/ws/watch", wsWatchBucketHandler)
}

// Upgrade HTTP connection to WebSocket
func upgradeToWebSocket(w http.ResponseWriter, r *http.Request) (*websocket.Conn, error) {
    upgrader := websocket.Upgrader{
        ReadBufferSize:  1024,
        WriteBufferSize: 1024,
        CheckOrigin: func(r *http.Request) bool {
            // Allow all origins in dev mode
            // In production, validate origin
            return true
        },
    }

    conn, err := upgrader.Upgrade(w, r, nil)
    if err != nil {
        return nil, err
    }

    return conn, nil
}
```

### Console Logs Handler: api/user_watch.go

```go
// WebSocket handler for console logs
func wsConsoleLogsHandler(w http.ResponseWriter, r *http.Request) {
    // Validate authentication
    session, err := getSessionFromRequest(r)
    if err != nil {
        http.Error(w, "Unauthorized", http.StatusUnauthorized)
        return
    }

    // Upgrade to WebSocket
    conn, err := upgradeToWebSocket(w, r)
    if err != nil {
        logger.Error("Failed to upgrade to WebSocket: %v", err)
        return
    }
    defer conn.Close()

    // Create MinIO admin client
    adminClient, err := newAdminClient(session)
    if err != nil {
        logger.Error("Failed to create admin client: %v", err)
        return
    }

    // Start trace
    ctx := r.Context()
    traceCh := adminClient.serverTrace(ctx, madmin.ServiceTraceOpts{
        S3:       true,
        Storage:  true,
        OS:       true,
        Scanner:  true,
        Threshold: 100 * time.Millisecond,
    })

    // Stream trace events to WebSocket
    for traceInfo := range traceCh {
        if traceInfo.Err != nil {
            logger.Error("Trace error: %v", traceInfo.Err)
            break
        }

        // Format trace message
        message := formatTraceMessage(traceInfo)

        // Send to WebSocket client
        err = conn.WriteJSON(message)
        if err != nil {
            logger.Error("Failed to send message: %v", err)
            break
        }
    }
}

// Format trace message
func formatTraceMessage(trace madmin.ServiceTraceInfo) map[string]interface{} {
    return map[string]interface{}{
        "timestamp": trace.Time.Format(time.RFC3339),
        "type":      trace.TraceType,
        "message":   trace.Trace.Message,
        "path":      trace.Trace.Path,
        "method":    trace.Trace.HTTP.Method,
        "status":    trace.Trace.HTTP.StatusCode,
        "duration":  trace.Trace.Duration.String(),
    }
}
```

### Bucket Watch Handler

```go
// WebSocket handler for bucket events
func wsWatchBucketHandler(w http.ResponseWriter, r *http.Request) {
    // Validate authentication
    session, err := getSessionFromRequest(r)
    if err != nil {
        http.Error(w, "Unauthorized", http.StatusUnauthorized)
        return
    }

    // Get bucket name from query params
    bucketName := r.URL.Query().Get("bucket")
    if bucketName == "" {
        http.Error(w, "Bucket name required", http.StatusBadRequest)
        return
    }

    // Upgrade to WebSocket
    conn, err := upgradeToWebSocket(w, r)
    if err != nil {
        return
    }
    defer conn.Close()

    // Create MinIO client
    mClient, err := newMinioClient(session)
    if err != nil {
        return
    }

    // Listen for bucket notifications
    ctx := r.Context()
    eventCh := mClient.listenBucketNotification(
        ctx,
        bucketName,
        "",  // prefix
        "",  // suffix
        []string{
            string(notification.ObjectCreatedAll),
            string(notification.ObjectRemovedAll),
        },
    )

    // Stream events to WebSocket
    for notificationInfo := range eventCh {
        if notificationInfo.Err != nil {
            logger.Error("Notification error: %v", notificationInfo.Err)
            break
        }

        // Format event
        for _, record := range notificationInfo.Records {
            event := map[string]interface{}{
                "eventName":  record.EventName,
                "eventTime":  record.EventTime,
                "bucket":     record.S3.Bucket.Name,
                "object":     record.S3.Object.Key,
                "size":       record.S3.Object.Size,
                "etag":       record.S3.Object.ETag,
                "versionId":  record.S3.Object.VersionID,
            }

            // Send to client
            err = conn.WriteJSON(event)
            if err != nil {
                return
            }
        }
    }
}
```

## 🌐 Frontend WebSocket Client

### WebSocket Client Class

```typescript
// websockets/ws-client.ts

export interface WebSocketMessage {
    type: string;
    data: any;
    timestamp: string;
}

export class WebSocketClient {
    private ws: WebSocket | null = null;
    private url: string;
    private reconnectInterval: number = 5000;
    private maxReconnectAttempts: number = 5;
    private reconnectAttempts: number = 0;
    private listeners: Map<string, Set<(data: any) => void>> = new Map();

    constructor(url: string) {
        this.url = url;
    }

    connect(): Promise<void> {
        return new Promise((resolve, reject) => {
            try {
                // Add token to URL
                const token = localStorage.getItem('token');
                const wsUrl = `${this.url}?token=${token}`;

                this.ws = new WebSocket(wsUrl);

                this.ws.onopen = () => {
                    console.log('WebSocket connected');
                    this.reconnectAttempts = 0;
                    resolve();
                };

                this.ws.onmessage = (event) => {
                    try {
                        const message: WebSocketMessage = JSON.parse(event.data);
                        this.handleMessage(message);
                    } catch (error) {
                        console.error('Failed to parse WebSocket message:', error);
                    }
                };

                this.ws.onerror = (error) => {
                    console.error('WebSocket error:', error);
                    reject(error);
                };

                this.ws.onclose = (event) => {
                    console.log('WebSocket closed:', event.code, event.reason);
                    this.handleReconnect();
                };
            } catch (error) {
                reject(error);
            }
        });
    }

    private handleMessage(message: WebSocketMessage) {
        const listeners = this.listeners.get(message.type);
        if (listeners) {
            listeners.forEach(listener => listener(message.data));
        }

        // Also notify wildcard listeners
        const wildcardListeners = this.listeners.get('*');
        if (wildcardListeners) {
            wildcardListeners.forEach(listener => listener(message));
        }
    }

    private handleReconnect() {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            console.log(`Reconnecting... (${this.reconnectAttempts}/${this.maxReconnectAttempts})`);

            setTimeout(() => {
                this.connect().catch(err => {
                    console.error('Reconnect failed:', err);
                });
            }, this.reconnectInterval);
        } else {
            console.error('Max reconnect attempts reached');
        }
    }

    on(eventType: string, callback: (data: any) => void) {
        if (!this.listeners.has(eventType)) {
            this.listeners.set(eventType, new Set());
        }
        this.listeners.get(eventType)!.add(callback);
    }

    off(eventType: string, callback: (data: any) => void) {
        const listeners = this.listeners.get(eventType);
        if (listeners) {
            listeners.delete(callback);
        }
    }

    send(data: any) {
        if (this.ws?.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(data));
        } else {
            console.error('WebSocket is not open');
        }
    }

    disconnect() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
    }

    isConnected(): boolean {
        return this.ws?.readyState === WebSocket.OPEN;
    }
}
```

### React Hook для WebSocket

```tsx
// websockets/useWebSocket.ts

import { useEffect, useRef, useState } from 'react';
import { WebSocketClient } from './ws-client';

export const useWebSocket = (url: string) => {
    const [connected, setConnected] = useState(false);
    const [error, setError] = useState<Error | null>(null);
    const clientRef = useRef<WebSocketClient | null>(null);

    useEffect(() => {
        // Create WebSocket client
        const client = new WebSocketClient(url);
        clientRef.current = client;

        // Connect
        client.connect()
            .then(() => setConnected(true))
            .catch(err => setError(err));

        // Cleanup on unmount
        return () => {
            client.disconnect();
        };
    }, [url]);

    const subscribe = (eventType: string, callback: (data: any) => void) => {
        clientRef.current?.on(eventType, callback);
    };

    const unsubscribe = (eventType: string, callback: (data: any) => void) => {
        clientRef.current?.off(eventType, callback);
    };

    const send = (data: any) => {
        clientRef.current?.send(data);
    };

    return {
        connected,
        error,
        subscribe,
        unsubscribe,
        send,
    };
};
```

## 📺 Frontend Components Using WebSocket

### Real-time Logs Viewer

```tsx
// screens/Console/Logs/LogsViewer.tsx

import { useEffect, useState } from 'react';
import { useWebSocket } from '../../../websockets/useWebSocket';

interface LogEntry {
    timestamp: string;
    type: string;
    message: string;
    path: string;
    method: string;
    status: number;
    duration: string;
}

const LogsViewer = () => {
    const [logs, setLogs] = useState<LogEntry[]>([]);
    const [autoScroll, setAutoScroll] = useState(true);
    const logsEndRef = useRef<HTMLDivElement>(null);

    const { connected, subscribe, unsubscribe } = useWebSocket(
        'ws://localhost:9090/ws/console'
    );

    useEffect(() => {
        const handleLog = (log: LogEntry) => {
            setLogs(prev => [...prev, log].slice(-1000)); // Keep last 1000 logs
        };

        subscribe('log', handleLog);

        return () => {
            unsubscribe('log', handleLog);
        };
    }, [subscribe, unsubscribe]);

    useEffect(() => {
        if (autoScroll) {
            logsEndRef.current?.scrollIntoView({ behavior: 'smooth' });
        }
    }, [logs, autoScroll]);

    return (
        <div className="logs-viewer">
            <div className="logs-header">
                <h2>Real-time Logs</h2>
                <div className="logs-controls">
                    <span className={`status ${connected ? 'connected' : 'disconnected'}`}>
                        {connected ? '● Connected' : '○ Disconnected'}
                    </span>
                    <label>
                        <input
                            type="checkbox"
                            checked={autoScroll}
                            onChange={(e) => setAutoScroll(e.target.checked)}
                        />
                        Auto-scroll
                    </label>
                    <button onClick={() => setLogs([])}>Clear</button>
                </div>
            </div>

            <div className="logs-container">
                {logs.map((log, index) => (
                    <div key={index} className="log-entry">
                        <span className="timestamp">{log.timestamp}</span>
                        <span className="type">{log.type}</span>
                        <span className="method">{log.method}</span>
                        <span className="path">{log.path}</span>
                        <span className={`status status-${Math.floor(log.status / 100)}`}>
                            {log.status}
                        </span>
                        <span className="duration">{log.duration}</span>
                        <span className="message">{log.message}</span>
                    </div>
                ))}
                <div ref={logsEndRef} />
            </div>
        </div>
    );
};
```

### Bucket Events Watcher

```tsx
// screens/Console/Buckets/BucketEventsWatcher.tsx

const BucketEventsWatcher = ({ bucketName }: { bucketName: string }) => {
    const [events, setEvents] = useState<BucketEvent[]>([]);

    const { connected, subscribe, unsubscribe } = useWebSocket(
        `ws://localhost:9090/ws/watch?bucket=${bucketName}`
    );

    useEffect(() => {
        const handleEvent = (event: BucketEvent) => {
            setEvents(prev => [event, ...prev].slice(0, 50)); // Keep last 50 events

            // Show notification
            showNotification({
                title: event.eventName,
                message: `${event.object} (${formatBytes(event.size)})`,
            });
        };

        subscribe('event', handleEvent);

        return () => {
            unsubscribe('event', handleEvent);
        };
    }, [subscribe, unsubscribe, bucketName]);

    return (
        <div className="bucket-events">
            <h3>Real-time Events for {bucketName}</h3>
            <div className="connection-status">
                {connected ? '● Live' : '○ Disconnected'}
            </div>

            <ul className="events-list">
                {events.map((event, index) => (
                    <li key={index} className={`event event-${event.eventName}`}>
                        <div className="event-icon">
                            {getEventIcon(event.eventName)}
                        </div>
                        <div className="event-details">
                            <div className="event-name">{event.eventName}</div>
                            <div className="event-object">{event.object}</div>
                            <div className="event-meta">
                                {formatBytes(event.size)} • {formatTime(event.eventTime)}
                            </div>
                        </div>
                    </li>
                ))}
            </ul>
        </div>
    );
};
```

## 🔒 WebSocket Security

### Authentication

```go
// Validate token from WebSocket connection
func getSessionFromRequest(r *http.Request) (*models.Principal, error) {
    // Try to get token from query parameter (for WebSocket)
    token := r.URL.Query().Get("token")

    // Fallback to Authorization header
    if token == "" {
        authHeader := r.Header.Get("Authorization")
        if strings.HasPrefix(authHeader, "Bearer ") {
            token = strings.TrimPrefix(authHeader, "Bearer ")
        }
    }

    if token == "" {
        return nil, errors.New("no token provided")
    }

    // Validate JWT
    claims, err := auth.ParseClaimsFromToken(token)
    if err != nil {
        return nil, err
    }

    return &models.Principal{
        STSAccessKeyID:     claims.STSAccessKeyID,
        STSSecretAccessKey: claims.STSSecretAccessKey,
        STSSessionToken:    claims.STSSessionToken,
    }, nil
}
```

### Origin Validation

```go
upgrader := websocket.Upgrader{
    CheckOrigin: func(r *http.Request) bool {
        origin := r.Header.Get("Origin")

        // In production, validate against allowed origins
        allowedOrigins := []string{
            "http://localhost:9090",
            "https://console.example.com",
        }

        for _, allowed := range allowedOrigins {
            if origin == allowed {
                return true
            }
        }

        return false
    },
}
```

## 🎯 Best Practices

### 1. Heartbeat/Ping-Pong

```go
// Send periodic ping to keep connection alive
go func() {
    ticker := time.NewTicker(30 * time.Second)
    defer ticker.Stop()

    for {
        select {
        case <-ticker.C:
            err := conn.WriteMessage(websocket.PingMessage, nil)
            if err != nil {
                return
            }
        case <-ctx.Done():
            return
        }
    }
}()
```

### 2. Graceful Shutdown

```go
// Handle context cancellation
select {
case <-ctx.Done():
    conn.WriteMessage(
        websocket.CloseMessage,
        websocket.FormatCloseMessage(websocket.CloseNormalClosure, ""),
    )
    return
}
```

### 3. Error Handling

```typescript
this.ws.onerror = (error) => {
    console.error('WebSocket error:', error);

    // Notify user
    showNotification({
        type: 'error',
        message: 'Lost connection to server. Reconnecting...',
    });
};
```

## 🚀 Следующие шаги

- **[07-build-deployment.md](07-build-deployment.md)** - Build & Deployment
- **[12-advanced-topics.md](12-advanced-topics.md)** - Advanced WebSocket usage
- **[13-practical-exercises.md](13-practical-exercises.md)** - Практика с WebSocket

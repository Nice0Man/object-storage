# Настройка и запуск клиентской части (Web Frontend)

## 📝 Описание

Реализована полноценная клиентская часть на React + TypeScript для управления Object Storage системой.

## 🎯 Реализованные компоненты

### ✅ API клиент
- **`src/api/client.ts`** - Axios клиент с полным набором методов для работы с backend API
- **`src/api/types.ts`** - TypeScript типы данных на основе swagger.json

### ✅ Redux State Management
- **`src/store/authSlice.ts`** - Управление аутентификацией (login, logout, session)
- **`src/store/bucketsSlice.ts`** - Управление бакетами (CRUD операции)
- **`src/store/objectsSlice.ts`** - Управление объектами (загрузка, скачивание, удаление)
- **`src/store/usersSlice.ts`** - Управление пользователями

### ✅ Компоненты
- **`MainLayout`** - Главный layout с навигацией и header
- **`ProtectedRoute`** - HOC для защиты маршрутов
- **`Loader`** - Компонент загрузки
- **`ErrorAlert`** - Компонент отображения ошибок
- **`FileUploader`** - Drag & drop загрузчик файлов

### ✅ Страницы
- **`LoginPage`** - Страница входа с формой аутентификации
- **`DashboardPage`** - Главная панель со статистикой
- **`BucketsPage`** - Управление бакетами
- **`ObjectsPage`** - Просмотр и управление объектами
- **`UsersPage`** - Управление пользователями

## 🚀 Установка и запуск

### 1. Перейти в директорию web-app

```bash
cd /mnt/c/Cpp/object-storage/web-app
```

### 2. Установить зависимости

```bash
npm install --legacy-peer-deps
```

**Примечание**: Флаг `--legacy-peer-deps` необходим из-за конфликта версий TypeScript между react-scripts и другими зависимостями.

### 3. Запустить backend сервер

В отдельном терминале:

```bash
cd /mnt/c/Cpp/object-storage/build
./ObjectStorageConsole
```

Backend должен быть запущен на `http://localhost:9090`

### 4. Запустить frontend development сервер

```bash
npm start
```

Приложение откроется автоматически по адресу: http://localhost:3000

## 🔑 Тестовые учетные данные

Используйте учетные данные, настроенные в вашем backend:

```
Access Key: minioadmin
Secret Key: minioadmin
```

(или другие учетные данные, настроенные в конфигурации backend)

## 📦 Production сборка

### 1. Создать production build

```bash
cd /mnt/c/Cpp/object-storage/web-app
npm run build
```

Оптимизированные файлы будут созданы в директории `build/`

### 2. Интеграция с C++ backend

Скопируйте содержимое `build/` в директорию, которую отдает ваш C++ backend как статические файлы:

```bash
cp -r build/* /path/to/backend/static/
```

Или настройте Drogon для отдачи статических файлов из `build/`:

```cpp
// В main.cpp
app().setDocumentRoot("./web-app/build");
```

## 🔧 Конфигурация

### Изменение URL backend

Отредактируйте `src/api/client.ts`:

```typescript
constructor(baseURL: string = 'http://localhost:9090') {
```

Или настройте через переменную окружения (создайте `.env` в `web-app/`):

```env
REACT_APP_API_URL=http://your-backend-url:port
```

И обновите `src/api/client.ts`:

```typescript
constructor(baseURL: string = process.env.REACT_APP_API_URL || 'http://localhost:9090') {
```

## 🎨 Особенности реализации

### JWT Authentication
- Токен сохраняется в `localStorage`
- Автоматически добавляется в заголовок всех запросов
- При 401 ошибке пользователь перенаправляется на `/login`

### File Upload
- Поддержка drag & drop
- Отображение прогресса загрузки
- Множественная загрузка файлов

### Breadcrumb Navigation
- Навигация по папкам в объектах
- Поддержка префиксов S3

### Responsive Design
- Адаптивный дизайн для мобильных устройств
- Material-UI компоненты
- Drawer navigation на мобильных

## 📝 Доступные скрипты

```bash
npm start          # Запуск development сервера
npm run build      # Production сборка
npm test           # Запуск тестов
npm run lint       # Проверка кода ESLint
npm run lint:fix   # Автоматическое исправление линтинга
npm run format     # Форматирование кода Prettier
npm run type-check # Проверка типов TypeScript
```

## 🐛 Устранение неполадок

### CORS ошибки

Если возникают CORS ошибки, убедитесь что backend настроен для приема запросов с `http://localhost:3000`:

```cpp
// В конфигурации Drogon
app().registerBeginningAdvice([](const drogon::HttpRequestPtr &req) {
    req->addHeader("Access-Control-Allow-Origin", "http://localhost:3000");
    req->addHeader("Access-Control-Allow-Credentials", "true");
    // ...
});
```

### Ошибки установки зависимостей

Если `npm install` не работает, попробуйте:

```bash
rm -rf node_modules package-lock.json
npm install --legacy-peer-deps
```

### Proxy не работает

Убедитесь что:
1. Backend запущен на `http://localhost:9090`
2. Файл `src/setupProxy.js` существует
3. Development сервер перезапущен после изменений

## 📚 Структура маршрутов

```
/                  - Dashboard (защищен)
/login             - Страница входа (публичная)
/buckets           - Список бакетов (защищен)
/buckets/:name     - Детали бакета (защищен)
/objects           - Просмотр объектов (защищен)
/users             - Управление пользователями (защищен)
```

## 🔐 Security заметки

1. JWT токены хранятся в `localStorage` - для production рассмотрите использование httpOnly cookies
2. Все чувствительные операции требуют аутентификации
3. Токены автоматически обновляются через `/api/v1/auth/refresh`

## 📈 Дальнейшие улучшения

Возможные направления для развития:

- [ ] Добавить WebSocket для real-time обновлений
- [ ] Реализовать управление политиками и группами
- [ ] Добавить настройку versioning и lifecycle для бакетов
- [ ] Реализовать preview для изображений и документов
- [ ] Добавить поиск и фильтрацию
- [ ] Реализовать batch операции для объектов
- [ ] Добавить темную тему
- [ ] Реализовать i18n (мультиязычность)

## 🤝 Поддержка

При возникновении проблем:

1. Проверьте консоль браузера на наличие ошибок
2. Проверьте логи backend сервера
3. Убедитесь что все API endpoints доступны через http://localhost:9090/docs

---

**Автор**: AI Assistant  
**Дата создания**: 2025-11-12  
**Версия**: 1.0.0


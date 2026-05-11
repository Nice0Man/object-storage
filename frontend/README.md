# Object Storage Console - Web Frontend

Современный веб-интерфейс для управления S3-совместимым объектным хранилищем.

## 🚀 Технологический стек

- **React 18.2** - Библиотека для построения пользовательских интерфейсов
- **TypeScript 5.3** - Статическая типизация
- **Redux Toolkit** - Управление состоянием приложения
- **Material-UI (MUI) 5** - Компонентная библиотека
- **React Router 6** - Маршрутизация
- **Axios** - HTTP клиент

## 📁 Структура проекта

```
src/
├── api/                    # API клиент и типы
│   ├── client.ts          # Axios клиент с методами API
│   └── types.ts           # TypeScript типы данных
├── components/            # Переиспользуемые компоненты
│   ├── Common/           # Общие компоненты (Loader, ErrorAlert, FileUploader)
│   ├── Layout/           # Layout компоненты (MainLayout)
│   └── ProtectedRoute.tsx # HOC для защиты маршрутов
├── hooks/                # Custom hooks
│   ├── useAppDispatch.ts
│   └── useAppSelector.ts
├── pages/                # Страницы приложения
│   ├── LoginPage.tsx
│   ├── DashboardPage.tsx
│   ├── BucketsPage.tsx
│   ├── ObjectsPage.tsx
│   └── UsersPage.tsx
├── store/                # Redux store
│   ├── index.ts          # Конфигурация store
│   ├── authSlice.ts      # Аутентификация
│   ├── bucketsSlice.ts   # Управление бакетами
│   ├── objectsSlice.ts   # Управление объектами
│   └── usersSlice.ts     # Управление пользователями
├── App.tsx               # Главный компонент
├── index.tsx             # Entry point
└── setupProxy.js         # Proxy для development

```

## 🛠️ Установка и запуск

### Установка зависимостей

```bash
npm install --legacy-peer-deps
```

### Development сервер

```bash
npm start
```

Приложение будет доступно по адресу: http://localhost:3000

API запросы проксируются на backend: http://localhost:9090

### Production build

```bash
npm run build
```

Собранные файлы будут в директории `build/`

## 🔐 Аутентификация

Приложение использует JWT токены для аутентификации:

1. При успешном логине токен сохраняется в `localStorage`
2. Токен автоматически добавляется в заголовок `Authorization` всех запросов
3. При ошибке 401 пользователь перенаправляется на страницу входа
4. При выходе токен удаляется из `localStorage`

## 📋 Основные функции

### Dashboard

- Общая статистика (количество бакетов, объектов, пользователей)
- Быстрый доступ к основным разделам
- Список последних бакетов

### Buckets

- Просмотр списка всех бакетов
- Создание нового бакета (с настройками versioning и object locking)
- Удаление бакета
- Просмотр информации о бакете

### Objects

- Навигация по объектам в бакете (поддержка префиксов/папок)
- Загрузка файлов (drag & drop)
- Скачивание объектов
- Удаление объектов (одиночное и массовое)
- Отображение прогресса загрузки

### Users

- Просмотр списка пользователей
- Создание нового пользователя
- Назначение политик и групп
- Удаление пользователя

## 🔧 Конфигурация

### API endpoint

По умолчанию используется `http://localhost:9090`. Для изменения отредактируйте `src/api/client.ts`:

```typescript
const apiClient = new ApiClient('http://your-backend-url');
```

### Proxy настройки

Для development сервера proxy настраивается в `src/setupProxy.js`:

```javascript
target: 'http://localhost:9090',  // Адрес backend сервера
```

## 🎨 Темизация

Приложение использует Material-UI с настраиваемой темой в `src/index.tsx`:

```typescript
const theme = createTheme({
  palette: {
    primary: {
      main: '#1976d2',
    },
    secondary: {
      main: '#dc004e',
    },
  },
});
```

## 📦 Сборка для production

После сборки:

```bash
npm run build
```

Статические файлы будут в `build/` и могут быть размещены на любом веб-сервере или интегрированы в C++ backend.

## 🧪 Тестирование

```bash
npm test
```

## 🔍 Линтинг

```bash
npm run lint
npm run lint:fix
```

## 💅 Форматирование

```bash
npm run format
```

## 🐛 Отладка

### Redux DevTools

Приложение поддерживает [Redux DevTools Extension](https://github.com/reduxjs/redux-devtools) для отладки состояния.

### Network requests

Используйте инструменты разработчика браузера для просмотра API запросов.

## 📝 API документация

Backend предоставляет Swagger UI документацию по адресу:
http://localhost:9090/docs

## 🤝 Интеграция с Backend

Frontend взаимодействует с C++ backend через REST API:

- **Base URL**: `http://localhost:9090/api/v1`
- **Authentication**: JWT Bearer tokens
- **Content-Type**: `application/json`
- **Multipart uploads**: `multipart/form-data`

## ⚠️ Известные проблемы

1. При использовании TypeScript 5.3 с react-scripts 5.0.1 требуется флаг `--legacy-peer-deps`
2. Для production сборки рекомендуется настроить CORS на backend стороне

## 📚 Дополнительные ресурсы

- [React Documentation](https://react.dev/)
- [Redux Toolkit Documentation](https://redux-toolkit.js.org/)
- [Material-UI Documentation](https://mui.com/)
- [TypeScript Documentation](https://www.typescriptlang.org/)

## 📄 Лицензия

MIT

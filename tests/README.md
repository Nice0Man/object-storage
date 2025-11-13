# 🎭 E2E Testing with Playwright

Автоматические end-to-end тесты для Object Storage Console.

## 📋 Структура тестов

```
tests/e2e/
├── auth.spec.ts        # Тесты аутентификации
├── dashboard.spec.ts   # Тесты dashboard и навигации
└── buckets.spec.ts     # Тесты управления buckets
```

## 🚀 Быстрый старт

### 1. Запустить backend

```bash
cd /mnt/c/Cpp/object-storage/build
./bin/console
```

### 2. Запустить тесты

```bash
cd /mnt/c/Cpp/object-storage

# Все тесты (frontend запустится автоматически)
npx playwright test

# С UI для отладки
npx playwright test --ui

# Показать браузер
npx playwright test --headed

# Конкретный файл
npx playwright test auth.spec.ts

# Конкретный тест
npx playwright test -g "should login"
```

## 📊 Отчеты и отладка

### HTML отчет

```bash
# Сгенерировать и открыть отчет
npx playwright show-report
```

### Трейсы

При падении теста автоматически сохраняется trace:

```bash
# Открыть trace для отладки
npx playwright show-trace trace.zip
```

### Скриншоты

Скриншоты сохраняются автоматически при ошибках в:

```
test-results/
```

## 🧪 Описание тестов

### Authentication (`auth.spec.ts`)

- ✅ Отображение страницы логина
- ✅ Вход с валидными credentials
- ✅ Ошибка при невалидных credentials

### Dashboard (`dashboard.spec.ts`)

- ✅ Отображение статистики
- ✅ Навигация к Buckets
- ✅ Навигация к Objects
- ✅ Навигация к Users
- ✅ Logout

### Buckets (`buckets.spec.ts`)

- ✅ Отображение страницы buckets
- ✅ Создание нового bucket
- ✅ Поиск buckets
- ✅ Просмотр деталей bucket

## ⚙️ Конфигурация

### `playwright.config.ts`

- **baseURL**: http://localhost:3000
- **timeout**: 30 секунд
- **retries**: 2 (в CI)
- **workers**: Параллельное выполнение
- **webServer**: Автозапуск frontend

### Окружение

```bash
# Установить все зависимости
npm install

# Установить браузеры
npx playwright install
```

## 🔧 Отладка

### Debug конкретного теста

```bash
npx playwright test auth.spec.ts --debug
```

### Показать трейс-вьювер

```bash
npx playwright show-trace
```

### Запись видео

Включено по умолчанию при падении теста.

## 📝 Написание новых тестов

### Пример теста

```typescript
import { test, expect } from '@playwright/test';

test('my test', async ({ page }) => {
  // Navigate
  await page.goto('/');

  // Interact
  await page.getByRole('button', { name: 'Click me' }).click();

  // Assert
  await expect(page.getByText('Success')).toBeVisible();
});
```

### Хелпер для логина

```typescript
async function login(page: any) {
  await page.goto('/');
  await page.getByLabel('Access Key').fill('minioadmin');
  await page.getByLabel('Secret Key').fill('minioadmin');
  await page.getByRole('button', { name: /sign in/i }).click();
  await page.waitForURL('/', { timeout: 10000 });
}

test.beforeEach(async ({ page }) => {
  await login(page);
});
```

## 🎯 CI/CD Integration

### GitHub Actions

```yaml
- name: Install Playwright
  run: npx playwright install --with-deps

- name: Run tests
  run: npx playwright test

- name: Upload report
  uses: actions/upload-artifact@v3
  if: always()
  with:
    name: playwright-report
    path: playwright-report/
```

## 📚 Документация

- [Playwright Docs](https://playwright.dev)
- [Best Practices](https://playwright.dev/docs/best-practices)
- [API Reference](https://playwright.dev/docs/api/class-playwright)

## 🐛 Troubleshooting

### Тесты падают с timeout

Увеличьте timeout в `playwright.config.ts`:

```typescript
use: {
  actionTimeout: 10000,
},
```

### Frontend не запускается

Проверьте что порт 3000 свободен:

```bash
lsof -i :3000
```

### Backend не отвечает

Убедитесь что backend запущен на порту 9090:

```bash
curl http://localhost:9090/api/v1/health
```

## ✨ Best Practices

1. **Используйте data-testid** для стабильных селекторов
2. **Избегайте sleep** - используйте `waitFor`
3. **Изолируйте тесты** - каждый тест независим
4. **Очищайте данные** после теста
5. **Используйте fixtures** для setup/teardown

## 📈 Метрики

Текущее покрытие:

- ✅ Аутентификация: 100%
- ✅ Навигация: 100%
- ⚠️  Buckets CRUD: 75%
- ⚠️  Objects CRUD: 0%
- ⚠️  Users CRUD: 0%

## 🎯 Roadmap

- [ ] Тесты для Objects
- [ ] Тесты для Users
- [ ] Тесты для Groups
- [ ] Тесты для Policies
- [ ] Тесты для upload/download
- [ ] Performance тесты
- [ ] Accessibility тесты

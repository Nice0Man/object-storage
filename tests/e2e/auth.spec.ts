import { test, expect } from '@playwright/test';

test.describe('Authentication Flow', () => {
  test('should display login page', async ({ page }) => {
    await page.goto('/');

    // Check page title
    await expect(page).toHaveTitle(/Object Storage/);

    // Check login form elements
    await expect(page.getByText('Object Storage')).toBeVisible();
    await expect(page.getByText('Console')).toBeVisible();
    await expect(page.getByLabel('Access Key')).toBeVisible();
    await expect(page.getByLabel('Secret Key')).toBeVisible();
    await expect(page.getByRole('button', { name: /sign in/i })).toBeVisible();
  });

  test('should login with valid credentials', async ({ page }) => {
    await page.goto('/');

    // Fill login form
    await page.getByLabel('Access Key').fill('minioadmin');
    await page.getByLabel('Secret Key').fill('minioadmin');

    // Click sign in button
    await page.getByRole('button', { name: /sign in/i }).click();

    // Wait for navigation to dashboard
    await page.waitForURL('/', { timeout: 10000 });

    // Check if we're on dashboard
    await expect(page.getByText('Dashboard')).toBeVisible();
    await expect(page.getByText('Total Buckets')).toBeVisible();
  });

  test('should show error for invalid credentials', async ({ page }) => {
    await page.goto('/');

    // Fill with invalid credentials
    await page.getByLabel('Access Key').fill('invalid');
    await page.getByLabel('Secret Key').fill('wrong');

    // Click sign in
    await page.getByRole('button', { name: /sign in/i }).click();

    // Wait for error message
    await expect(page.locator('[role="alert"]')).toBeVisible({ timeout: 5000 });
  });
});

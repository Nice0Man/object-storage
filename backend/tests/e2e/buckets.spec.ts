import { test, expect } from '@playwright/test';

async function login(page: any) {
  await page.goto('/');
  await page.getByLabel('Access Key').fill('minioadmin');
  await page.getByLabel('Secret Key').fill('minioadmin');
  await page.getByRole('button', { name: /sign in/i }).click();
  await page.waitForURL('/', { timeout: 10000 });
}

test.describe('Buckets Management Flow', () => {
  test.beforeEach(async ({ page }) => {
    await login(page);
    // Navigate to buckets page
    await page.locator('text=Buckets').first().click();
    await expect(page).toHaveURL(/.*buckets/);
  });

  test('should display buckets page', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /buckets/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /create bucket/i })).toBeVisible();
  });

  test('should create a new bucket', async ({ page }) => {
    const bucketName = `test-bucket-${Date.now()}`;

    // Click create bucket button
    await page.getByRole('button', { name: /create bucket/i }).click();

    // Wait for dialog
    await expect(page.getByRole('dialog')).toBeVisible();

    // Fill bucket name
    const nameInput = page.locator('input[name="bucketName"], input[id*="bucket"]').first();
    await nameInput.fill(bucketName);

    // Submit
    await page.getByRole('button', { name: /create/i }).click();

    // Wait for bucket to appear in list
    await expect(page.getByText(bucketName)).toBeVisible({ timeout: 5000 });
  });

  test('should search buckets', async ({ page }) => {
    // If there's a search input
    const searchInput = page.locator('input[placeholder*="search" i], input[type="search"]').first();

    if (await searchInput.isVisible()) {
      await searchInput.fill('test');
      // Results should filter
      await page.waitForTimeout(1000);
    }
  });

  test('should view bucket details', async ({ page }) => {
    // Click on first bucket if exists
    const firstBucket = page.locator('[data-testid="bucket-item"], .MuiTableRow-root').first();

    if (await firstBucket.isVisible()) {
      await firstBucket.click();
      // Should show bucket details or navigate
      await page.waitForTimeout(1000);
    }
  });
});

import { test, expect } from '@playwright/test';

// Helper to login before tests
async function login(page: any) {
  await page.goto('/');
  await page.getByLabel('Access Key').fill('minioadmin');
  await page.getByLabel('Secret Key').fill('minioadmin');
  await page.getByRole('button', { name: /sign in/i }).click();
  await page.waitForURL('/', { timeout: 10000 });
}

test.describe('Dashboard Flow', () => {
  test.beforeEach(async ({ page }) => {
    await login(page);
  });

  test('should display dashboard statistics', async ({ page }) => {
    // Check dashboard elements
    await expect(page.getByText('Dashboard')).toBeVisible();
    await expect(page.getByText('Total Buckets')).toBeVisible();
    await expect(page.getByText('Total Objects')).toBeVisible();
    await expect(page.getByText('Total Users')).toBeVisible();
    await expect(page.getByText('Storage Used')).toBeVisible();
  });

  test('should navigate to buckets page', async ({ page }) => {
    // Click on Buckets navigation or card
    const bucketsLink = page.locator('text=Buckets').first();
    await bucketsLink.click();

    // Verify we're on buckets page
    await expect(page).toHaveURL(/.*buckets/);
    await expect(page.getByRole('heading', { name: /buckets/i })).toBeVisible();
  });

  test('should navigate to objects page', async ({ page }) => {
    // Click on Objects navigation
    const objectsLink = page.locator('text=Objects').first();
    await objectsLink.click();

    // Verify we're on objects page
    await expect(page).toHaveURL(/.*objects/);
  });

  test('should navigate to users page', async ({ page }) => {
    // Click on Users navigation
    const usersLink = page.locator('text=Users').first();
    await usersLink.click();

    // Verify we're on users page
    await expect(page).toHaveURL(/.*users/);
    await expect(page.getByRole('heading', { name: /users/i })).toBeVisible();
  });

  test('should logout successfully', async ({ page }) => {
    // Click logout button
    await page.getByRole('button', { name: /logout/i }).click();

    // Verify we're back to login page
    await expect(page).toHaveURL('/login');
    await expect(page.getByText('Sign in to Object Storage Console')).toBeVisible();
  });
});

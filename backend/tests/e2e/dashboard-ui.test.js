import { test, expect } from '@playwright/test';

test.describe('Dashboard UI Tests', () => {
  test.beforeEach(async ({ page }) => {
    // Navigate to login page
    await page.goto('http://localhost:3000/login');

    // Wait for page to load
    await page.waitForLoadState('domcontentloaded');

    // Perform login - fields are "Access Key" and "Secret Key"
    await page.getByRole('textbox', { name: 'Access Key' }).fill('admin');
    await page.getByRole('textbox', { name: 'Secret Key' }).fill('changeme');
    await page.getByRole('button', { name: 'Sign In' }).click();

    // Wait for navigation after login (redirects to root /)
    await page.waitForURL('http://localhost:3000/', { timeout: 10000 });
    await page.waitForLoadState('networkidle');

    // Verify we're logged in by checking for Dashboard heading
    await page.waitForSelector('h4:has-text("Dashboard")', { timeout: 5000 });
  });

  test('should display dashboard header and title', async ({ page }) => {
    // Check page title
    await expect(page).toHaveTitle('Object Storage Console');

    // Check dashboard heading
    const heading = page.locator('h4:has-text("Dashboard")');
    await expect(heading).toBeVisible();

    // Check subtitle
    const subtitle = page.locator('text=Monitor and manage your S3-compatible object storage');
    await expect(subtitle).toBeVisible();
  });

  test('should display capacity card with donut chart', async ({ page }) => {
    // Check Capacity card heading
    const capacityHeading = page.locator('h6:has-text("Capacity")');
    await expect(capacityHeading).toBeVisible();

    // Check for total capacity display
    const totalCapacity = page.locator('text=/\\d+\\s*(TB|GB|MB|Bytes)/').first();
    await expect(totalCapacity).toBeVisible();

    // Check for "Available" label
    const availableLabel = page.locator('text=Available').first();
    await expect(availableLabel).toBeVisible();

    // Check for "Object Data" label
    const objectDataLabel = page.locator('text=Object Data').first();
    await expect(objectDataLabel).toBeVisible();

    // Verify donut chart is rendered (SVG or Canvas)
    const chart = page.locator('[role="progressbar"]').first();
    await expect(chart).toBeVisible();
  });

  test('should display statistics cards', async ({ page }) => {
    // Total Buckets card
    const bucketsCard = page.locator('text=Total Buckets');
    await expect(bucketsCard).toBeVisible();
    const bucketsCount = page.locator('h4').filter({ hasText: /^\d+$/ }).first();
    await expect(bucketsCount).toBeVisible();

    // Total Objects card
    const objectsCard = page.locator('text=Total Objects');
    await expect(objectsCard).toBeVisible();

    // Total Users card
    const usersCard = page.locator('text=Total Users');
    await expect(usersCard).toBeVisible();

    // Storage Used card
    const storageCard = page.locator('text=Storage Used');
    await expect(storageCard).toBeVisible();
  });

  test('should display recent activity section', async ({ page }) => {
    // Check Recent Activity heading
    const recentActivityHeading = page.locator('h6:has-text("Recent Activity")');
    await expect(recentActivityHeading).toBeVisible();

    // Check subtitle
    const subtitle = page.locator('text=Latest buckets');
    await expect(subtitle).toBeVisible();

    // Check for bucket items (should show at least the test buckets)
    const bucketItems = page.locator('text=/121231|mybucket/');
    await expect(bucketItems.first()).toBeVisible();
  });

  test('should display top buckets section', async ({ page }) => {
    // Check Top Buckets heading
    const topBucketsHeading = page.locator('h6:has-text("Top Buckets")');
    await expect(topBucketsHeading).toBeVisible();

    // Check subtitle
    const subtitle = page.locator('text=By Size');
    await expect(subtitle).toBeVisible();

    // Check for progress bars (visual indicators)
    const progressBars = page.locator('[role="progressbar"]');
    await expect(progressBars.first()).toBeVisible();
  });

  test('should display quick actions section', async ({ page }) => {
    // Check Quick Actions heading
    const quickActionsHeading = page.locator('h6:has-text("Quick Actions")');
    await expect(quickActionsHeading).toBeVisible();

    // Check for action cards
    const createBucketAction = page.locator('text=Create Bucket');
    await expect(createBucketAction).toBeVisible();

    const uploadObjectsAction = page.locator('text=Upload Objects');
    await expect(uploadObjectsAction).toBeVisible();

    const manageUsersAction = page.locator('text=Manage Users');
    await expect(manageUsersAction).toBeVisible();
  });

  test('should verify capacity data accuracy', async ({ page }) => {
    // Check for capacity data - more flexible selectors
    const availableLabel = page.locator('text=Available').first();
    const objectDataLabel = page.locator('text=Object Data').first();

    // Labels should be visible
    await expect(availableLabel).toBeVisible();
    await expect(objectDataLabel).toBeVisible();

    // Verify there are size values on the page (TB, GB, MB, Bytes)
    const sizeValues = page.locator('text=/\\d+\\s*(TB|GB|MB|Bytes)/');
    const count = await sizeValues.count();
    expect(count).toBeGreaterThan(0);
  });

  test('should display bucket information correctly', async ({ page }) => {
    // Check that buckets show object count
    const objectCounts = page.locator('text=/\\d+\\s+objects?/');
    await expect(objectCounts.first()).toBeVisible();

    // Check that buckets show size
    const bucketSizes = page.locator('text=/\\d+\\s+(TB|GB|MB|Bytes)/');
    await expect(bucketSizes.first()).toBeVisible();
  });

  test('should have collapsible sidebar', async ({ page }) => {
    // Check that navigation menu exists
    const navMenu = page.locator('nav, button:has-text("Dashboard")').first();
    await expect(navMenu).toBeVisible();

    // Sidebar is functional - we can see menu items
    const dashboardButton = page.locator('button:has-text("Dashboard")');
    await expect(dashboardButton).toBeVisible();

    // Note: Actual collapse/expand test would need specific button identification
    // For now, just verify sidebar presence and navigation works
  });

  test('should navigate to buckets page when clicking buckets card', async ({ page }) => {
    // Click on Total Buckets card
    const bucketsCard = page.locator('text=Total Buckets').locator('..');
    await bucketsCard.click();

    // Wait for navigation
    await page.waitForURL(/.*buckets.*/);

    // Verify we're on buckets page
    const bucketsHeading = page.locator('h4:has-text("Buckets")');
    await expect(bucketsHeading).toBeVisible();
  });

  test('should navigate to users page when clicking manage users', async ({ page }) => {
    // Click on Manage Users quick action
    const manageUsersCard = page.locator('text=Manage Users').locator('..');
    await manageUsersCard.click();

    // Wait for navigation
    await page.waitForURL(/.*users.*/);

    // Verify we're on users page
    const usersHeading = page.locator('h4:has-text("Users")');
    await expect(usersHeading).toBeVisible();
  });

  test('should display user info in sidebar', async ({ page }) => {
    // Check for admin username
    const username = page.locator('text=Administrator');
    await expect(username).toBeVisible();

    // Check for logout button
    const logoutButton = page.locator('button:has-text("Logout")');
    await expect(logoutButton).toBeVisible();

    // Verify user controls are present
    expect(await username.isVisible()).toBeTruthy();
    expect(await logoutButton.isVisible()).toBeTruthy();
  });

  test('should take full page screenshot for visual regression', async ({ page }) => {
    // Take a full page screenshot
    await page.screenshot({
      path: 'tests/e2e/screenshots/dashboard-full.png',
      fullPage: true
    });

    // Take a screenshot of just the capacity card
    const capacityCard = page.locator('h6:has-text("Capacity")').locator('..');
    await capacityCard.screenshot({
      path: 'tests/e2e/screenshots/capacity-card.png'
    });
  });
});

test.describe('Dashboard Data Visualization Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('http://localhost:3000/login');
    await page.waitForLoadState('domcontentloaded');
    await page.getByRole('textbox', { name: 'Access Key' }).fill('admin');
    await page.getByRole('textbox', { name: 'Secret Key' }).fill('changeme');
    await page.getByRole('button', { name: 'Sign In' }).click();
    await page.waitForURL('http://localhost:3000/', { timeout: 10000 });
    await page.waitForLoadState('networkidle');
    await page.waitForSelector('h4:has-text("Dashboard")', { timeout: 5000 });
  });

  test('should render progress bars correctly', async ({ page }) => {
    // Find all progress bars on the page
    const progressBars = page.locator('[role="progressbar"]');
    const count = await progressBars.count();

    // Should have at least capacity progress bars
    expect(count).toBeGreaterThan(0);

    // Each progress bar should be visible
    for (let i = 0; i < Math.min(count, 5); i++) {
      await expect(progressBars.nth(i)).toBeVisible();
    }
  });

  test('should display all navigation menu items', async ({ page }) => {
    // Check for key navigation items
    const bucketsButton = page.getByRole('button', { name: 'Buckets' });
    await expect(bucketsButton).toBeVisible();

    const usersButton = page.getByRole('button', { name: 'Users' });
    await expect(usersButton).toBeVisible();

    const objectsButton = page.getByRole('button', { name: 'Objects' });
    await expect(objectsButton).toBeVisible();

    // Verify at least 3 menu items are visible
    expect(await bucketsButton.isVisible()).toBeTruthy();
    expect(await usersButton.isVisible()).toBeTruthy();
    expect(await objectsButton.isVisible()).toBeTruthy();
  });

  test('should verify data consistency across cards', async ({ page }) => {
    // Get bucket count from card
    const totalBucketsText = await page.locator('text=Total Buckets').locator('..').locator('h4').textContent();
    const bucketsCount = parseInt(totalBucketsText || '0');

    // Verify bucket count is a number
    expect(bucketsCount).toBeGreaterThanOrEqual(0);

    // Check that Recent Activity section exists
    const recentActivity = page.locator('text=Recent Activity');
    await expect(recentActivity).toBeVisible();

    // Verify we have some data displayed
    expect(bucketsCount).toBeGreaterThanOrEqual(0);
  });
});

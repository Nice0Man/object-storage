/**
 * Login Workflow E2E Tests
 * Tests login functionality and identifies issues with fast refresh
 */

const axios = require('axios');

const API_BASE = 'http://localhost:9090';
const WEB_BASE = 'http://localhost:3000';

// Test credentials
const ADMIN_CREDS = {
  username: 'admin',
  password: 'changeme'
};

/**
 * Test 1: Basic Login Flow
 */
async function testBasicLogin() {
  console.log('\n🧪 Test 1: Basic Login Flow');
  try {
    const response = await axios.post(`${API_BASE}/api/v1/auth/login`, ADMIN_CREDS);

    if (response.status === 200 && response.data.token) {
      console.log('✅ Login successful');
      console.log(`   Token received: ${response.data.token.substring(0, 20)}...`);
      console.log(`   User: ${response.data.user.username}`);
      console.log(`   Admin: ${response.data.user.is_admin}`);
      return { success: true, token: response.data.token };
    } else {
      console.log('❌ Login failed: No token received');
      return { success: false };
    }
  } catch (error) {
    console.log(`❌ Login failed: ${error.message}`);
    if (error.response) {
      console.log(`   Status: ${error.response.status}`);
      console.log(`   Error: ${JSON.stringify(error.response.data)}`);
    }
    return { success: false };
  }
}

/**
 * Test 2: Token Validation
 */
async function testTokenValidation(token) {
  console.log('\n🧪 Test 2: Token Validation');
  try {
    const response = await axios.get(`${API_BASE}/api/v1/users`, {
      headers: {
        'Authorization': `Bearer ${token}`
      }
    });

    if (response.status === 200) {
      console.log('✅ Token is valid and admin access works');
      console.log(`   Users count: ${response.data.total || 0}`);
      return { success: true };
    } else {
      console.log('❌ Token validation failed');
      return { success: false };
    }
  } catch (error) {
    console.log(`❌ Token validation failed: ${error.message}`);
    if (error.response) {
      console.log(`   Status: ${error.response.status}`);
      console.log(`   Error: ${JSON.stringify(error.response.data)}`);
    }
    return { success: false };
  }
}

/**
 * Test 3: Rapid Sequential Logins (Fast Refresh Simulation)
 */
async function testRapidLogins() {
  console.log('\n🧪 Test 3: Rapid Sequential Logins (Fast Refresh Bug Test)');
  const results = [];

  for (let i = 0; i < 5; i++) {
    try {
      const startTime = Date.now();
      const response = await axios.post(`${API_BASE}/api/v1/auth/login`, ADMIN_CREDS);
      const duration = Date.now() - startTime;

      if (response.status === 200 && response.data.token) {
        console.log(`✅ Login ${i + 1}: Success (${duration}ms)`);
        results.push({ success: true, duration, attempt: i + 1 });
      } else {
        console.log(`❌ Login ${i + 1}: Failed - No token`);
        results.push({ success: false, duration, attempt: i + 1 });
      }
    } catch (error) {
      console.log(`❌ Login ${i + 1}: Error - ${error.message}`);
      results.push({ success: false, error: error.message, attempt: i + 1 });
    }

    // Small delay to simulate fast refresh
    await new Promise(resolve => setTimeout(resolve, 100));
  }

  const successCount = results.filter(r => r.success).length;
  console.log(`\n   Results: ${successCount}/5 successful logins`);

  return { success: successCount === 5, results };
}

/**
 * Test 4: Invalid Credentials
 */
async function testInvalidCredentials() {
  console.log('\n🧪 Test 4: Invalid Credentials');
  try {
    const response = await axios.post(`${API_BASE}/api/v1/auth/login`, {
      username: 'admin',
      password: 'wrongpassword'
    });

    console.log('❌ Should have failed with invalid credentials');
    return { success: false };
  } catch (error) {
    if (error.response && error.response.status === 401) {
      console.log('✅ Correctly rejected invalid credentials');
      return { success: true };
    } else {
      console.log(`❌ Unexpected error: ${error.message}`);
      return { success: false };
    }
  }
}

/**
 * Test 5: Token Expiration Check
 */
async function testTokenExpiration() {
  console.log('\n🧪 Test 5: Token Format Validation');
  try {
    const response = await axios.post(`${API_BASE}/api/v1/auth/login`, ADMIN_CREDS);
    const token = response.data.token;

    // Check JWT format
    const parts = token.split('.');
    if (parts.length === 3) {
      console.log('✅ Token has valid JWT format (3 parts)');

      // Decode payload (base64)
      const payload = JSON.parse(Buffer.from(parts[1], 'base64').toString());
      console.log('   Token payload:');
      console.log(`   - Issuer: ${payload.iss}`);
      console.log(`   - Subject: ${payload.sub}`);
      console.log(`   - Account: ${payload.account_name}`);
      console.log(`   - Is Admin: ${payload.is_admin}`);
      console.log(`   - Issued: ${new Date(payload.iat * 1000).toISOString()}`);
      console.log(`   - Expires: ${new Date(payload.exp * 1000).toISOString()}`);

      return { success: true, payload };
    } else {
      console.log('❌ Token has invalid format');
      return { success: false };
    }
  } catch (error) {
    console.log(`❌ Token format validation failed: ${error.message}`);
    return { success: false };
  }
}

/**
 * Test 6: Concurrent Login Requests
 */
async function testConcurrentLogins() {
  console.log('\n🧪 Test 6: Concurrent Login Requests');

  const promises = Array(5).fill(null).map((_, i) =>
    axios.post(`${API_BASE}/api/v1/auth/login`, ADMIN_CREDS)
      .then(response => ({ success: true, attempt: i + 1 }))
      .catch(error => ({ success: false, attempt: i + 1, error: error.message }))
  );

  const results = await Promise.all(promises);
  const successCount = results.filter(r => r.success).length;

  console.log(`   Results: ${successCount}/5 concurrent logins succeeded`);
  results.forEach(r => {
    if (r.success) {
      console.log(`✅ Concurrent login ${r.attempt}: Success`);
    } else {
      console.log(`❌ Concurrent login ${r.attempt}: Failed - ${r.error}`);
    }
  });

  return { success: successCount === 5, results };
}

/**
 * Main Test Suite
 */
async function runTests() {
  console.log('═══════════════════════════════════════════════════════');
  console.log('🚀 Login Workflow E2E Tests');
  console.log('═══════════════════════════════════════════════════════');

  const results = {
    basicLogin: { success: false },
    tokenValidation: { success: false },
    rapidLogins: { success: false },
    invalidCredentials: { success: false },
    tokenFormat: { success: false },
    concurrentLogins: { success: false }
  };

  // Test 1: Basic Login
  results.basicLogin = await testBasicLogin();

  if (results.basicLogin.success) {
    // Test 2: Token Validation
    results.tokenValidation = await testTokenValidation(results.basicLogin.token);
  }

  // Test 3: Rapid Sequential Logins
  results.rapidLogins = await testRapidLogins();

  // Test 4: Invalid Credentials
  results.invalidCredentials = await testInvalidCredentials();

  // Test 5: Token Format
  results.tokenFormat = await testTokenExpiration();

  // Test 6: Concurrent Logins
  results.concurrentLogins = await testConcurrentLogins();

  // Summary
  console.log('\n═══════════════════════════════════════════════════════');
  console.log('📊 Test Summary');
  console.log('═══════════════════════════════════════════════════════');

  const totalTests = Object.keys(results).length;
  const passedTests = Object.values(results).filter(r => r.success).length;

  Object.entries(results).forEach(([name, result]) => {
    const icon = result.success ? '✅' : '❌';
    console.log(`${icon} ${name}: ${result.success ? 'PASSED' : 'FAILED'}`);
  });

  console.log(`\n📈 Overall: ${passedTests}/${totalTests} tests passed`);
  console.log('═══════════════════════════════════════════════════════\n');

  return {
    passed: passedTests,
    total: totalTests,
    success: passedTests === totalTests,
    results
  };
}

// Run tests if called directly
if (require.main === module) {
  runTests()
    .then(summary => {
      process.exit(summary.success ? 0 : 1);
    })
    .catch(error => {
      console.error('Test suite failed:', error);
      process.exit(1);
    });
}

module.exports = { runTests };

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "${ROOT}/frontend"

case "${ROOT}" in
  /mnt/*)
    export TMPDIR="${TMPDIR:-${HOME}/.cache/object-storage-console-npm-tmp}"
    mkdir -p "${TMPDIR}"
    ;;
esac

# Fast path (default): skip npm install, Vite build only.
# INSTALL_NODE_DEPS=1 — force npm ci/install. STRICT_FRONTEND_BUILD=1 — run tsc before build.
STRICT="${STRICT_FRONTEND_BUILD:-0}"
SKIP_INSTALL="${SKIP_NPM_INSTALL:-1}"
INSTALL_DEPS="${INSTALL_NODE_DEPS:-0}"

if [[ ! -f package.json ]]; then
  echo "frontend/package.json not found"
  exit 1
fi

needs_install=0
stale_node_modules=0
if [[ -d node_modules/react-scripts ]]; then
  echo "Detected legacy react-scripts in node_modules (project uses Vite)."
  stale_node_modules=1
fi
if [[ -d node_modules ]] && ! node -e "require('vite/package.json')" 2>/dev/null; then
  echo "Vite is missing from node_modules."
  stale_node_modules=1
fi
if [[ "${stale_node_modules}" == "1" ]]; then
  chmod -R u+w node_modules 2>/dev/null || true
  rm -rf node_modules
  needs_install=1
fi

if [[ "${INSTALL_DEPS}" == "1" ]] || [[ "${SKIP_INSTALL}" != "1" ]]; then
  needs_install=1
elif [[ ! -d node_modules ]] || ! node -e "require('vite/package.json')" 2>/dev/null; then
  echo "node_modules missing or incomplete — installing dependencies"
  needs_install=1
elif ! node -e "
  const os = process.platform;
  const arch = process.arch === 'arm64' ? 'arm64' : process.arch === 'ia32' ? 'ia32' : 'x64';
  let pkg;
  if (os === 'linux') {
    let abi = 'gnu';
    try {
      const { execSync } = require('child_process');
      const out = execSync('ldd --version 2>&1', { encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] });
      if (/musl/i.test(out)) abi = 'musl';
    } catch {
      try {
        require('child_process').execSync('ldd /bin/sh', { stdio: 'pipe' });
      } catch {
        abi = 'musl';
      }
    }
    pkg = '@rollup/rollup-linux-' + arch + '-' + abi;
  } else if (os === 'darwin') {
    pkg = '@rollup/rollup-darwin-' + arch;
  } else if (os === 'win32') {
    pkg = '@rollup/rollup-win32-' + arch + '-msvc';
  } else {
    process.exit(1);
  }
  require.resolve(pkg);
" 2>/dev/null; then
  echo "Rollup native binary missing — installing optional dependencies"
  needs_install=1
fi

if [[ "${needs_install}" == "1" ]]; then
  if [[ "${CLEAN_FRONTEND_NODE_MODULES:-0}" == "1" ]]; then
    chmod -R u+w node_modules 2>/dev/null || true
    rm -rf node_modules
  fi
  if [[ -d node_modules ]] && ! node -e "require('vite/package.json')" 2>/dev/null; then
    echo "Removing broken frontend/node_modules (retry install)."
    chmod -R u+w node_modules 2>/dev/null || true
    rm -rf node_modules
  fi
  if [[ -f package-lock.json ]]; then
    if ! npm ci --no-audit --no-fund --include=optional; then
      echo "npm ci failed (lock out of sync?). Running npm install."
      npm install --no-audit --no-fund --include=optional
    fi
  else
    npm install --no-audit --no-fund --include=optional
  fi
  if ! node -e "
    const os = process.platform;
    const arch = process.arch === 'arm64' ? 'arm64' : process.arch === 'ia32' ? 'ia32' : 'x64';
    let pkg;
    if (os === 'linux') {
      let abi = 'gnu';
      try {
        const { execSync } = require('child_process');
        const out = execSync('ldd --version 2>&1', { encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] });
        if (/musl/i.test(out)) abi = 'musl';
      } catch {
        try {
          require('child_process').execSync('ldd /bin/sh', { stdio: 'pipe' });
        } catch {
          abi = 'musl';
        }
      }
      pkg = '@rollup/rollup-linux-' + arch + '-' + abi;
    } else if (os === 'darwin') {
      pkg = '@rollup/rollup-darwin-' + arch;
    } else if (os === 'win32') {
      pkg = '@rollup/rollup-win32-' + arch + '-msvc';
    } else {
      process.exit(1);
    }
    require.resolve(pkg);
  " 2>/dev/null; then
    echo "Retry: npm install --include=optional for Rollup native module"
    npm install --no-audit --no-fund --include=optional
  fi
fi

SECONDS=0

if [[ "${STRICT}" == "1" ]]; then
  echo "Strict build: type-check + vite build"
  npm run type-check
else
  echo "Fast build: vite only (STRICT_FRONTEND_BUILD=1 for tsc)"
fi

if ! node -e "require('vite/package.json')" 2>/dev/null; then
  echo "Error: Vite not installed after npm install. Run: CLEAN_FRONTEND_NODE_MODULES=1 INSTALL_NODE_DEPS=1 ./build-frontend.sh"
  exit 1
fi

if [[ -d node_modules/react-scripts ]]; then
  echo "Error: react-scripts still present; package.json expects Vite. Clean node_modules and retry."
  exit 1
fi

npm run build

echo "Frontend build ready in ${SECONDS}s: ${ROOT}/frontend/build"

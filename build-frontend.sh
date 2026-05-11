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

export GENERATE_SOURCEMAP="${GENERATE_SOURCEMAP:-false}"
export DISABLE_ESLINT_PLUGIN="${DISABLE_ESLINT_PLUGIN:-true}"
export TSC_COMPILE_ON_ERROR="${TSC_COMPILE_ON_ERROR:-true}"

if [[ ! -f package.json ]]; then
  echo "frontend/package.json not found"
  exit 1
fi

if [[ "${INSTALL_NODE_DEPS:-1}" == "1" ]]; then
  if [[ "${CLEAN_FRONTEND_NODE_MODULES:-0}" == "1" ]]; then
    chmod -R u+w node_modules 2>/dev/null || true
    rm -rf node_modules
  fi
  if [[ -d node_modules ]] && ! node -e "require('react-dev-utils/crossSpawn')" 2>/dev/null; then
    echo "Removing broken frontend/node_modules (retry install with TMPDIR=${TMPDIR:-default})."
    chmod -R u+w node_modules 2>/dev/null || true
    rm -rf node_modules
  fi
  if [[ -d node_modules ]] && [[ -f package-lock.json ]]; then
    if ! npm ci; then
      echo "npm ci failed (lock out of sync?). Running npm install to refresh deps."
      npm install
    fi
  else
    npm install
  fi
fi

npm run build

echo "Frontend build ready: ${ROOT}/frontend/build"

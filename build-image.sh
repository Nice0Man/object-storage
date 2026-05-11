#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "${ROOT}"

IMAGE_NAME="${IMAGE_NAME:-object-storage-console}"
IMAGE_TAG="${IMAGE_TAG:-latest}"
DOCKERFILE_PATH="${DOCKERFILE_PATH:-Dockerfile}"
BUILD_BACKEND_FIRST="${BUILD_BACKEND_FIRST:-1}"
BUILD_FRONTEND_FIRST="${BUILD_FRONTEND_FIRST:-1}"
BACKEND_BUILD_JOBS="${BACKEND_BUILD_JOBS:-$(nproc)}"
SKIP_BACKEND_IF_READY="${SKIP_BACKEND_IF_READY:-1}"
SKIP_FRONTEND_IF_READY="${SKIP_FRONTEND_IF_READY:-1}"
BACKEND_BIN="${BACKEND_BIN:-${ROOT}/backend/build/bin/console}"
FRONTEND_READY_MARKER="${FRONTEND_READY_MARKER:-${ROOT}/frontend/build/index.html}"

if [[ "${BUILD_BACKEND_FIRST}" == "1" ]]; then
  if [[ "${SKIP_BACKEND_IF_READY}" == "1" ]] && [[ -x "${BACKEND_BIN}" ]]; then
    echo "Backend binary already present, skipping build-backend.sh (${BACKEND_BIN})"
  else
    echo "Building backend binary first (jobs=${BACKEND_BUILD_JOBS})"
    BUILD_JOBS="${BACKEND_BUILD_JOBS}" ./build-backend.sh
  fi
fi

if [[ "${BUILD_FRONTEND_FIRST}" == "1" ]]; then
  if [[ "${SKIP_FRONTEND_IF_READY}" == "1" ]] && [[ -f "${FRONTEND_READY_MARKER}" ]]; then
    echo "Frontend build already present, skipping build-frontend.sh (${FRONTEND_READY_MARKER})"
  else
    echo "Building frontend (npm) before docker build"
    if [[ "${SKIP_FRONTEND_IF_READY}" == "0" ]]; then
      SKIP_IF_READY=0 ./build-frontend.sh
    else
      ./build-frontend.sh
    fi
  fi
fi

echo "Building ${IMAGE_NAME}:${IMAGE_TAG} using ${DOCKERFILE_PATH}"
docker build -f "${DOCKERFILE_PATH}" -t "${IMAGE_NAME}:${IMAGE_TAG}" .
echo "Done: ${IMAGE_NAME}:${IMAGE_TAG}"

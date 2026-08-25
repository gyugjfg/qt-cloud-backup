#!/usr/bin/env bash
set -euo pipefail

SERVER_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SERVER_DIR}/.." && pwd)"
ROOT_PATH="${1:-${PWD}/cloud-backup-root}"
BUILD_DIR="${BACKUP_SERVER_BUILD_DIR:-${TMPDIR:-/tmp}/qt-cloud-backup-server-build}"
BINARY_PATH="${BACKUP_SERVER_BINARY:-${BUILD_DIR}/backup_server}"

mkdir -p "${ROOT_PATH}" "${BUILD_DIR}"
g++ -std=c++17 -O2 -Wall -Wextra -pthread \
    "${SERVER_DIR}/server.cpp" \
    -o "${BINARY_PATH}"

echo "Server root: ${ROOT_PATH}"
echo "Starting backup server; client virtual '/' maps to this directory."
exec "${BINARY_PATH}" "${ROOT_PATH}"

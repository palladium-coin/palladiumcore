#!/usr/bin/env bash
# Build the same commit twice, with a different number of jobs, and compare the checksums.
#
#   ./check-reproducible.sh <linux-x86_64|linux-aarch64|linux-armv7l|windows>
#
# Exit code 0 only if every binary is identical. On a difference, diffoscope shows where.
export LC_ALL=C
set -euo pipefail

TARGET="${1:?usage: $0 <target>}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BASE="${REPO_DIR}/build/reproducible-check/${TARGET}"
rm -rf "${BASE}"

OUT_DIR="${BASE}/a" JOBS="$(nproc)" "${SCRIPT_DIR}/build.sh" "${TARGET}"
OUT_DIR="${BASE}/b" JOBS=2 "${SCRIPT_DIR}/build.sh" "${TARGET}"

if diff -u "${BASE}/a/SHA256SUMS" "${BASE}/b/SHA256SUMS"; then
    echo "[OK] ${TARGET}: both builds are identical"
else
    echo "[FAIL] ${TARGET}: the builds differ (compare ${BASE}/a and ${BASE}/b, e.g. with diffoscope)" >&2
    exit 1
fi

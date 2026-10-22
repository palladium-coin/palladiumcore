#!/usr/bin/env bash
# Reproducible build of the release binaries inside Docker (Ubuntu amd64 host).
#
#   ./build.sh <linux-x86_64|linux-aarch64|linux-armv7l|windows> [--installer]
#
# Environment: OUT_DIR (default ../build/<target>), JOBS (default: all cores),
#              REF (default HEAD; a tag or commit to build instead).
#
# Only what is committed is built: the sources are exported with "git archive", so local changes and
# untracked files never end up in a binary.
export LC_ALL=C
set -euo pipefail

usage() {
    echo "Usage: $0 <linux-x86_64|linux-aarch64|linux-armv7l|windows> [--installer]" >&2
}

TARGET="${1:-}"
INSTALLER=false
[[ "${2:-}" == "--installer" ]] && INSTALLER=true

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

case "${TARGET}" in
linux-x86_64)
    HOST_TRIPLE=x86_64-pc-linux-gnu
    EXTRA_PACKAGES=""
    POST_INSTALL=true
    ;;
linux-aarch64)
    HOST_TRIPLE=aarch64-linux-gnu
    EXTRA_PACKAGES="g++-aarch64-linux-gnu binutils-aarch64-linux-gnu"
    POST_INSTALL=true
    ;;
linux-armv7l)
    HOST_TRIPLE=arm-linux-gnueabihf
    EXTRA_PACKAGES="g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf"
    POST_INSTALL=true
    ;;
windows)
    HOST_TRIPLE=x86_64-w64-mingw32
    EXTRA_PACKAGES="g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 nsis"
    POST_INSTALL="update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix && update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix"
    ;;
*)
    usage
    exit 1
    ;;
esac
if [[ "${INSTALLER}" == true && "${TARGET}" != windows ]]; then
    echo "--installer only applies to the windows target" >&2
    exit 1
fi

REF="${REF:-HEAD}"
COMMIT="$(git -C "${REPO_DIR}" rev-parse --verify "${REF}^{commit}")"
SOURCE_DATE_EPOCH="$(git -C "${REPO_DIR}" log -1 --format=%ct "${COMMIT}")"
OUT_DIR="${OUT_DIR:-${REPO_DIR}/build/${TARGET}}"
CACHE_DIR="${CACHE_DIR:-${REPO_DIR}/build/cache}"
IMAGE_NAME="palladium-builder:${TARGET}"

if [[ "${REF}" == HEAD && -n "$(git -C "${REPO_DIR}" status --porcelain --untracked-files=no)" ]]; then
    echo "[!] there are uncommitted changes: they are NOT part of the build (only ${COMMIT:0:12} is)"
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
echo "[*] exporting ${COMMIT}"
git -C "${REPO_DIR}" archive --format=tar --prefix= -o "${WORK_DIR}/source.tar" "${COMMIT}"

echo "[*] build image ${IMAGE_NAME}"
docker build --platform=linux/amd64 \
    --build-arg EXTRA_PACKAGES="${EXTRA_PACKAGES}" \
    --build-arg POST_INSTALL="${POST_INSTALL}" \
    -t "${IMAGE_NAME}" - < "${SCRIPT_DIR}/Dockerfile"
IMAGE_ID="$(docker image inspect --format '{{.Id}}' "${IMAGE_NAME}")"

mkdir -p "${OUT_DIR}" "${CACHE_DIR}/sources"

echo "[*] build in the container (only the output and the download cache are writable)"
docker run --rm --platform=linux/amd64 \
    -e TARGET="${TARGET}" -e HOST_TRIPLE="${HOST_TRIPLE}" -e COMMIT="${COMMIT}" \
    -e SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH}" -e BUILD_INSTALLER="${INSTALLER}" \
    -e JOBS="${JOBS:-$(nproc)}" -e IMAGE_ID="${IMAGE_ID}" \
    -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
    -v "${WORK_DIR}/source.tar":/source.tar:ro \
    -v "${SCRIPT_DIR}/in-container-build.sh":/in-container-build.sh:ro \
    -v "${CACHE_DIR}":/cache \
    -v "${OUT_DIR}":/out \
    "${IMAGE_NAME}" bash /in-container-build.sh

echo "[*] binaries and SHA256SUMS in: ${OUT_DIR}"

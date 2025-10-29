#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="palladium-builder:linux-aarch64-ubuntu20.04"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${REPO_DIR}/build/aarch64"
HOST_TRIPLE="aarch64-linux-gnu"

HOST_UID="$(id -u)"
HOST_GID="$(id -g)"

echo "[*] Build image including ALL repository files (COPY . /src)..."

docker build --platform=linux/amd64 \
  -t "$IMAGE_NAME" \
  -f "${SCRIPT_DIR}/Dockerfile.linux-aarch64" \
  "${REPO_DIR}"

mkdir -p "${OUT_DIR}"

echo "[*] Start container: build COMPLETED in container; mount ONLY output..."
docker run --rm --platform=linux/amd64 \
  -e HOST_UID="${HOST_UID}" -e HOST_GID="${HOST_GID}" \
  -v "${OUT_DIR}":/out \
  "$IMAGE_NAME" \
  bash -c "
    set -euo pipefail
    cd /src

    echo '[*] depends (HOST=${HOST_TRIPLE})...'
    cd depends && make HOST=${HOST_TRIPLE} -j\$(nproc) && cd ..

    echo '[*] autogen/configure...'
    [[ -x ./autogen.sh ]] && ./autogen.sh
    [[ -f ./configure ]] || { echo 'configure not found: autogen failed'; exit 1; }

    ./configure --prefix=\$PWD/depends/${HOST_TRIPLE} \
                --host=${HOST_TRIPLE} \
                --enable-glibc-back-compat \
                --enable-reduce-exports \
                CC=${HOST_TRIPLE}-gcc CXX=${HOST_TRIPLE}-g++ \
                AR=${HOST_TRIPLE}-ar RANLIB=${HOST_TRIPLE}-ranlib STRIP=${HOST_TRIPLE}-strip \
                LDFLAGS='-static-libstdc++'

    echo '[*] make...'
    make -j\$(nproc)

    echo '[*] copy binaries to /out...'
    mkdir -p /out
    for b in src/palladiumd src/palladium-cli src/palladium-tx src/palladium-wallet src/qt/palladium-qt; do
      [[ -f \"\$b\" ]] && install -m 0755 \"\$b\" /out/
    done

    # Add permissions to host user
    chown -R \${HOST_UID:-0}:\${HOST_GID:-0} /out

    echo '[*] build COMPLETED (all binaries in container) → /out'
  "

echo "[*] Binaries aarch64 available in: ${OUT_DIR}"

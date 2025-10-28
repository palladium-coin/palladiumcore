#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="palladium-builder:ubuntu20.04"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${SCRIPT_DIR}/out"
HOST="x86_64-pc-linux-gnu"

if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
  docker build -t "$IMAGE_NAME" -f "${SCRIPT_DIR}/Dockerfile" "${SCRIPT_DIR}"
fi

mkdir -p "$OUT_DIR"

docker run --rm \
  -v "${REPO_DIR}":/src \
  -v "${OUT_DIR}":/out \
  "$IMAGE_NAME" \
  bash -c "
    set -euo pipefail
    cd /src
    cd depends && make HOST=${HOST} -j\$(nproc) && cd ..
    [[ -x ./autogen.sh ]] && ./autogen.sh
    ./configure --prefix=\$PWD/depends/${HOST} \
                --enable-glibc-back-compat \
                --enable-reduce-exports \
                LDFLAGS='-static-libstdc++'
    make -j\$(nproc)
    for bin in src/palladiumd src/palladium-cli src/palladium-tx src/palladium-wallet src/qt/palladium-qt; do
      [[ -f \"\$bin\" ]] && install -m 0755 \"\$bin\" /out/
    done
  "

echo "[*] x86_64 binaries copied to ${OUT_DIR}"

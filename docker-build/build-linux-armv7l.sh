#!/usr/bin/env bash
export LC_ALL=C
set -euo pipefail

IMAGE_NAME="palladium-builder:linux-armv7l-ubuntu20.04"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${REPO_DIR}/build/linux-armv7l"
HOST_TRIPLE="arm-linux-gnueabihf"

HOST_UID="$(id -u)"
HOST_GID="$(id -g)"

echo "[*] Building Docker image including the ENTIRE repository (COPY . /src)..."
docker build --platform=linux/amd64 \
  -t "$IMAGE_NAME" \
  -f "${SCRIPT_DIR}/Dockerfile.linux-armv7l" \
  "${REPO_DIR}"

mkdir -p "${OUT_DIR}"

echo "[*] Starting container: build COMPLETED in container; mounting ONLY the output..."
docker run --rm --platform=linux/amd64 \
  -e HOST_UID="${HOST_UID}" -e HOST_GID="${HOST_GID}" \
  -v "${OUT_DIR}":/out \
  "$IMAGE_NAME" \
  bash -c "
    set -euo pipefail
    cd /src

    echo '[*] cleaning tree (avoid host-built artifacts)...'
    [[ -f Makefile ]] && make distclean || true
    find . -name \"*.moc\" -delete
    find . -name \"moc_*.cpp\" -delete
    rm -rf univalue/.libs
    rm -rf depends/${HOST_TRIPLE}
    rm -f config.cache

    echo '[*] depends (HOST=${HOST_TRIPLE}, forcing Qt packages)...'
    cd depends && make HOST=${HOST_TRIPLE} NO_QT= -j\$(nproc) && cd ..

    echo '[*] autogen/configure...'
    [[ -x ./autogen.sh ]] && ./autogen.sh
    [[ -f ./configure ]] || { echo 'configure not found: autogen failed'; exit 1; }

    DEPENDS_PREFIX=\$PWD/depends/${HOST_TRIPLE}

    # Qt host tools (moc/uic/rcc) are staged in depends/<host>/native/bin.
    export PATH=\${DEPENDS_PREFIX}/native/bin:\${DEPENDS_PREFIX}/bin:\$PATH

    CONFIG_SITE=\${DEPENDS_PREFIX}/share/config.site \
      ./configure --prefix=\${DEPENDS_PREFIX} \
                --enable-gui=qt5 \
                --with-gui=qt5 \
                --with-qrencode \
                --with-qt-bindir=\${DEPENDS_PREFIX}/native/bin \
                --host=${HOST_TRIPLE} \
                --enable-glibc-back-compat \
                --enable-reduce-exports \
                CC=${HOST_TRIPLE}-gcc CXX=${HOST_TRIPLE}-g++ \
                AR=${HOST_TRIPLE}-ar RANLIB=${HOST_TRIPLE}-ranlib STRIP=${HOST_TRIPLE}-strip \
                LDFLAGS='-static-libstdc++'

    echo '[*] make...'
    make -j\$(nproc)

    if [[ ! -x src/qt/palladium-qt ]]; then
      echo '[!] ERROR: src/qt/palladium-qt not produced. Check configure output for \"with gui / qt = yes\".'
      exit 1
    fi

    echo '[*] copy binaries to /out...'
    mkdir -p /out
    for b in src/palladiumd src/palladium-cli src/palladium-tx src/palladium-wallet; do
      [[ -f \"\$b\" ]] && install -m 0755 \"\$b\" /out/
    done
    install -m 0755 src/qt/palladium-qt /out/

    # Add permissions to host user
    chown -R \${HOST_UID:-0}:\${HOST_GID:-0} /out

    echo '[*] build COMPLETED (all binaries in container) → /out'
  "

echo "[*] ARMv7l binaries available in: ${OUT_DIR}"

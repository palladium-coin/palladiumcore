#!/usr/bin/env bash
export LC_ALL=C
set -euo pipefail

# Default to not building the installer
BUILD_INSTALLER=false

# Check for --installer flag
if [[ "${1:-}" == "--installer" ]]; then
    BUILD_INSTALLER=true
    echo "[*] Installer build requested."
fi

IMAGE_NAME="palladium-builder:windows-ubuntu20.04"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${REPO_DIR}/build/windows"
HOST_TRIPLE="x86_64-w64-mingw32"

HOST_UID="$(id -u)"
HOST_GID="$(id -g)"

echo "[*] Building Docker image including the ENTIRE repository (COPY . /src)..."

docker build --platform=linux/amd64 \
  -t "$IMAGE_NAME" \
  -f "${SCRIPT_DIR}/Dockerfile.windows" \
  "${REPO_DIR}"

mkdir -p "${OUT_DIR}"

echo "[*] Starting container: build COMPLETELY in container; mount ONLY the output..."
docker run --rm --platform=linux/amd64 \
  -e HOST_UID="${HOST_UID}" -e HOST_GID="${HOST_GID}" \
  -v "${OUT_DIR}":/out \
  -e BUILD_INSTALLER="${BUILD_INSTALLER}" \
  "$IMAGE_NAME" \
  bash -c "
    set -euo pipefail
    cd /src

    echo '[*] cleaning tree (avoid host-built artifacts)...'
    [[ -f Makefile ]] && make distclean || true
    (cd univalue && make distclean) || true
    rm -rf univalue/.libs univalue/lib/.libs
    rm -f univalue/config.cache univalue/config.status
    rm -rf depends/${HOST_TRIPLE}
    rm -f config.cache

    echo '[*] depends (Windows cross, HOST=${HOST_TRIPLE})...'
    cd depends && make HOST=${HOST_TRIPLE} -j\$(nproc) && cd ..

    echo '[*] autogen/configure...'
    [[ -x ./autogen.sh ]] && ./autogen.sh
    [[ -f ./configure ]] || { echo 'configure not found: autogen failed'; exit 1; }

    # For Windows with depends: use CONFIG_SITE and prefix=/
    CONFIG_SITE=\$PWD/depends/${HOST_TRIPLE}/share/config.site \
      ./configure --prefix=/

    echo '[*] make...'
    make -j\$(nproc)

    echo '[*] copying .exe files to /out...'
    mkdir -p /out
    # Typical executables (update the names if your project produces different ones)
    for b in \
      src/palladiumd.exe \
      src/palladium-cli.exe \
      src/palladium-tx.exe \
      src/palladium-wallet.exe \
      src/qt/palladium-qt.exe; do
      [[ -f \"\$b\" ]] && install -m 0755 \"\$b\" /out/
    done

    if [[ "\$BUILD_INSTALLER" == "true" ]]; then
        echo '[*] make deploy (creating installer)...'
        make deploy
        echo '[*] copying installer to /out...'
        find . -iname 'palladium-*-win64-setup.exe' -print -exec install -m 0644 {} /out/ \;
    fi

    # Align permissions to host user
    chown -R \${HOST_UID:-0}:\${HOST_GID:-0} /out

    echo '[*] build COMPLETED (everything in container) → /out'
  "

echo "[*] Windows executables available in: ${OUT_DIR}"

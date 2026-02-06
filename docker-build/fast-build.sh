#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="palladium-fast-builder:latest"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUT_DIR="${REPO_DIR}/build/docker-out"

# 1. Build the base builder image (cached)
docker build -t "$IMAGE_NAME" -f "${SCRIPT_DIR}/Dockerfile.fast-builder" "$SCRIPT_DIR"

mkdir -p "$OUT_DIR"

# 2. Run the build inside the container using the REPO as a volume
docker run --rm \
    -v "${REPO_DIR}:/src" \
    -v "${OUT_DIR}:/out" \
    "$IMAGE_NAME" \
    bash -c "
        set -e
        cd /src
        
        # Build BDB if needed
        if [ ! -d '/src/db4' ]; then
            ./contrib/install_db4.sh . --with-mutex=POSIX/pthreads CPPFLAGS='-P -D_GNU_SOURCE' LDFLAGS='-lpthread'
        fi
        
        export BDB_PREFIX='/src/db4'
        
        ./autogen.sh
        ./configure \
            --with-gui=qt5 \
            BDB_LIBS='-L${BDB_PREFIX}/lib -ldb_cxx-4.8' \
            BDB_CFLAGS='-I${BDB_PREFIX}/include' \
            LDFLAGS='-lpthread -lrt' \
            --disable-tests \
            --disable-bench
            
        make -j$(nproc)
        
        echo 'Copying binaries...'
        cp src/palladiumd src/palladium-cli src/qt/palladium-qt /out/
        echo 'Build complete.'
    "

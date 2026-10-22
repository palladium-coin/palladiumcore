#!/usr/bin/env bash
# Runs inside the build container (started by build.sh). Do not run it on the host.
export LC_ALL=C
export TZ=UTC
set -euo pipefail

: "${TARGET:?}" "${HOST_TRIPLE:?}" "${SOURCE_DATE_EPOCH:?}" "${COMMIT:?}"
export SOURCE_DATE_EPOCH
JOBS="${JOBS:-$(nproc)}"
BUILD_INSTALLER="${BUILD_INSTALLER:-false}"

# The path is part of the binaries (assert messages, debug info): always the same one
BUILD_DIR=/build/src
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
echo "[*] unpacking commit ${COMMIT}"
tar -xf /source.tar -C "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[*] depends for ${HOST_TRIPLE}"
make -C depends HOST="${HOST_TRIPLE}" SOURCES_PATH=/cache/sources -j"${JOBS}"

echo "[*] autogen"
./autogen.sh

DEPENDS_PREFIX="${BUILD_DIR}/depends/${HOST_TRIPLE}"
# Qt host tools (moc/uic/rcc) come from depends, never from the system
export PATH="${DEPENDS_PREFIX}/native/bin:${DEPENDS_PREFIX}/bin:${PATH}"
export CONFIG_SITE="${DEPENDS_PREFIX}/share/config.site"

# rcc stores the modification time of every resource: use a fixed one (as depends does for Qt itself)
export QT_RCC_TEST=1 QT_RCC_SOURCE_DATE_OVERRIDE=1

# Same compiler flags for every build; the build path is replaced in what the compiler writes
MAP_FLAGS="-ffile-prefix-map=${BUILD_DIR}=. -fdebug-prefix-map=${BUILD_DIR}=."

case "${TARGET}" in
linux-x86_64)
    CONFIGURE_ARGS=(--prefix="${DEPENDS_PREFIX}" --enable-glibc-back-compat --enable-reduce-exports
        LDFLAGS=-static-libstdc++)
    STRIP=strip
    SUFFIX=""
    ;;
linux-aarch64 | linux-armv7l)
    CONFIGURE_ARGS=(--prefix="${DEPENDS_PREFIX}" --host="${HOST_TRIPLE}" --enable-glibc-back-compat --enable-reduce-exports
        CC="${HOST_TRIPLE}-gcc" CXX="${HOST_TRIPLE}-g++" AR="${HOST_TRIPLE}-ar"
        RANLIB="${HOST_TRIPLE}-ranlib" STRIP="${HOST_TRIPLE}-strip" LDFLAGS=-static-libstdc++)
    STRIP="${HOST_TRIPLE}-strip"
    SUFFIX=""
    ;;
windows)
    CONFIGURE_ARGS=(--prefix=/ --host="${HOST_TRIPLE}")
    STRIP="${HOST_TRIPLE}-strip"
    SUFFIX=".exe"
    ;;
*)
    echo "unknown target ${TARGET}" >&2
    exit 1
    ;;
esac

echo "[*] configure"
./configure --enable-determinism --enable-gui=qt5 --with-gui=qt5 --with-qrencode \
    --with-qt-bindir="${DEPENDS_PREFIX}/native/bin" \
    "${CONFIGURE_ARGS[@]}" \
    CFLAGS="${MAP_FLAGS}" CXXFLAGS="${MAP_FLAGS}"

echo "[*] make"
make -j"${JOBS}"

if [[ ! -x "src/qt/palladium-qt${SUFFIX}" ]]; then
    echo "[!] src/qt/palladium-qt${SUFFIX} was not produced: check 'with gui / qt' in the configure output" >&2
    exit 1
fi

echo "[*] copying the stripped binaries to /out"
rm -f /out/SHA256SUMS /out/build-info.txt
mkdir -p /out
for b in src/palladiumd src/palladium-cli src/palladium-tx src/palladium-wallet src/qt/palladium-qt; do
    install -m 0755 "${b}${SUFFIX}" /out/
    "${STRIP}" "/out/$(basename "${b}")${SUFFIX}"
done

if [[ "${TARGET}" == windows && "${BUILD_INSTALLER}" == true ]]; then
    echo "[*] make deploy (installer; not part of the reproducibility check)"
    make deploy
    find . -iname 'palladium-*-win64-setup.exe' -exec install -m 0644 {} /out/ \;
fi

echo "[*] checksums"
(cd /out && sha256sum palladium*"${SUFFIX}" > SHA256SUMS)
cat > /out/build-info.txt <<INFO
commit: ${COMMIT}
source date epoch: ${SOURCE_DATE_EPOCH}
target: ${TARGET}
image: ${IMAGE_ID:-unknown}
INFO
cat /out/SHA256SUMS

# Align the ownership with the host user
chown -R "${HOST_UID:-0}:${HOST_GID:-0}" /out
echo "[*] done"

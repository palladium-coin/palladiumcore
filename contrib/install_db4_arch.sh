#!/bin/bash
# Fixed BerkeleyDB 4.8 installer for Arch Linux (modern GCC/glibc)
set -e

BDB_VERSION='db-4.8.30.NC'
BDB_PREFIX="$(pwd)/db4"

# Clean up
rm -rf db4 "${BDB_VERSION}" "${BDB_VERSION}.tar.gz"

# Download
curl -L -O "https://download.oracle.com/berkeley-db/${BDB_VERSION}.tar.gz"
tar -xzvf "${BDB_VERSION}.tar.gz"

cd "${BDB_VERSION}"

# Apply the required C++11/Clang patch
curl -L -o clang.patch "https://gist.githubusercontent.com/LnL7/5153b251fd525fe15de69b67e63a6075/raw/7778e9364679093a32dec2908656738e16b6bdcb/clang.patch"
patch -p2 < clang.patch

# Fix the config.guess/sub for modern architectures
curl -L -o dist/config.guess "https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD"
curl -L -o dist/config.sub "https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD"

cd build_unix/

# THE CRITICAL FIX: Explicitly set the mutex type to POSIX/pthreads
# and use -D_GNU_SOURCE to expose required headers
../dist/configure   --enable-cxx   --disable-shared   --disable-replication   --with-pic   --prefix="${BDB_PREFIX}"   --with-mutex=POSIX/pthreads   CPPFLAGS="-P -D_GNU_SOURCE"   LDFLAGS="-lpthread"

make install
echo "DONE: BDB 4.8 built at ${BDB_PREFIX}"

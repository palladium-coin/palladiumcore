#!/bin/bash
# High-compatibility build script for Arch Linux
set -e
export BDB_PREFIX="$(pwd)/db4"
rm -rf db4 db-4.8.30.NC
./contrib/install_db4.sh . --with-mutex=POSIX/pthreads CPPFLAGS="-P -D_GNU_SOURCE" LDFLAGS="-lpthread"
./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" --with-boost=/usr --with-boost-libdir=/usr/lib --with-gui=qt5 --disable-tests --disable-bench --disable-reduce-exports LDFLAGS="-lpthread -lrt" LIBS="-lboost_system -lboost_filesystem -lboost_thread -lboost_chrono"
make -j$(nproc)

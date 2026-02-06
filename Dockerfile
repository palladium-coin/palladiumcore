# --- Stage 1: Build Environment ---
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y     build-essential libtool autotools-dev automake pkg-config bsdmainutils python3     libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev     libboost-chrono-dev libboost-test-dev libboost-thread-dev     libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a libqt5dbus5     qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler     curl ca-certificates

WORKDIR /app
COPY . .

# Build BerkeleyDB 4.8
RUN ./contrib/install_db4.sh .

# Build Palladium Core
RUN ./autogen.sh &&     export BDB_PREFIX="/app/db4" &&     ./configure         --with-gui=qt5         BDB_LIBS="-L/lib -ldb_cxx-4.8"         BDB_CFLAGS="-I/include"         --disable-tests         --disable-bench &&     make -j4

# --- Stage 2: Production Image ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y     libevent-2.1-7 libboost-system1.74.0 libboost-filesystem1.74.0     libboost-thread1.74.0 libzmq5 libqt5gui5 libqt5core5a libqt5dbus5     libqt5widgets5 libprotobuf23     && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/bin
COPY --from=builder /app/src/palladiumd .
COPY --from=builder /app/src/palladium-cli .
COPY --from=builder /app/src/qt/palladium-qt .

VOLUME [/root/.palladium]

EXPOSE 2332 2333

CMD [palladiumd, -printtoconsole]

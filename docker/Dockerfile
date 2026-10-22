# Runs palladiumd from the official release binaries (not built from source: see ../docker-build for that).
#
#   docker build -t palladiumd docker
#   docker run -d --name palladium -v ~/.palladium:/home/palladium/.palladium -p 2333:2333 palladiumd
#   docker exec palladium palladium-cli getblockchaininfo
#
# Builds for linux/amd64, linux/arm64 and linux/arm/v7 (docker buildx build --platform ... ).
#
# The archive is checked against the clearsigned SHA256SUMS.asc, which is checked against the release key (pinned fingerprint below).

FROM ubuntu:22.04 AS fetch

ARG VERSION=2.1.0
# Fingerprint of palladium-release-key.asc, published in the release notes
ARG RELEASE_FPR=70D5C2A2F6C5A128CD9223F227AB4AEDBF5D8BC1
# Set automatically by BuildKit: amd64, arm64 or arm
ARG TARGETARCH

RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl gnupg \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /dl
RUN set -eux; \
    case "${TARGETARCH}" in \
      amd64) T=x86_64-linux-gnu ;; \
      arm64) T=aarch64-linux-gnu ;; \
      arm)   T=arm-linux-gnueabihf ;; \
      *) echo "unsupported architecture: ${TARGETARCH}" >&2; exit 1 ;; \
    esac; \
    URL=https://github.com/palladium-coin/palladiumcore/releases/download/v${VERSION}; \
    F=palladium-${VERSION}-${T}.tar.gz; \
    for f in "$F" SHA256SUMS.asc palladium-release-key.asc; do curl -fsSLO "$URL/$f"; done; \
    export GNUPGHOME="$(mktemp -d)"; \
    gpg --import palladium-release-key.asc; \
    gpg --with-colons --fingerprint | grep -q "^fpr:::::::::${RELEASE_FPR}:"; \
    gpg --status-fd 1 --output SHA256SUMS.signed --decrypt SHA256SUMS.asc | grep -q "^\[GNUPG:\] VALIDSIG ${RELEASE_FPR} "; \
    sha256sum -c --ignore-missing SHA256SUMS.signed; \
    tar xzf "$F" --strip-components=1 -C /usr/local palladium-${VERSION}/bin/palladiumd palladium-${VERSION}/bin/palladium-cli

FROM ubuntu:22.04

COPY --from=fetch /usr/local/bin/palladiumd /usr/local/bin/palladium-cli /usr/local/bin/

# The data directory is the default one (~/.palladium), so palladium-cli works inside the container as is.
# Created here so that an empty volume is owned by the palladium user; docker-compose.yml mounts the host's ~/.palladium.
RUN useradd --create-home --uid 1000 palladium \
 && mkdir /home/palladium/.palladium && chown palladium:palladium /home/palladium/.palladium
USER palladium
VOLUME /home/palladium/.palladium

# What the node listens on: P2P (mainnet), RPC, and the ZMQ streams (hashblock, rawblock, rawtx).
# EXPOSE only documents it and publishes nothing: containers on the same network reach all of them anyway, and the
# host only gets what -p / ports: publishes (2333 only, see docker-compose.yml and README.md).
EXPOSE 2333 2332 28332 28334 28335

# -daemon=0: the palladium.conf that palladiumd writes on first start has daemon=1, which would end the container
ENTRYPOINT ["palladiumd", "-printtoconsole", "-daemon=0"]

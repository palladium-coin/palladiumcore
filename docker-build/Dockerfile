# Build environment for the reproducible release builds (see README.md).
#
# Everything that could change between two builds is pinned:
# - the base image, by digest
# - the packages, by the date of the Ubuntu archive snapshot they come from
# The source code is NOT copied into the image: build.sh passes the exported commit at run time.

# Ubuntu 20.04 (focal), linux/amd64. Update the digest on purpose, never by accident:
#   docker pull --platform=linux/amd64 ubuntu:20.04 && docker inspect --format '{{index .RepoDigests 0}}' ubuntu:20.04
ARG UBUNTU_IMAGE=ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214
FROM ${UBUNTU_IMAGE}

# https://snapshot.ubuntu.com/ubuntu/<date>/ must have this date. Change it on purpose only.
ARG APT_SNAPSHOT=20261015T000000Z
# Packages that depend on the target (cross compilers, NSIS, ...)
ARG EXTRA_PACKAGES=""
# Shell command run after the packages are installed
ARG POST_INSTALL="true"

ENV DEBIAN_FRONTEND=noninteractive TZ=UTC LC_ALL=C

# The snapshot service sometimes skips an index without failing, so apt-get update runs again with every
# attempt of the install below.

# The base image has no CA certificates and the snapshot service only speaks https: fetch them first
# from the default mirror (they are only used to verify TLS, not part of what is built).
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates \
 && printf '%s\n' \
      "deb https://snapshot.ubuntu.com/ubuntu/${APT_SNAPSHOT}/ focal main restricted universe multiverse" \
      "deb https://snapshot.ubuntu.com/ubuntu/${APT_SNAPSHOT}/ focal-updates main restricted universe multiverse" \
      "deb https://snapshot.ubuntu.com/ubuntu/${APT_SNAPSHOT}/ focal-security main restricted universe multiverse" \
      > /etc/apt/sources.list \
 && printf '%s\n' 'Acquire::Check-Valid-Until "false";' 'Acquire::Retries "10";' > /etc/apt/apt.conf.d/10snapshot \
 && for attempt in 1 2 3 4 5 6; do \
      apt-get update && apt-get install -y --no-install-recommends --fix-missing \
      build-essential libtool autotools-dev automake autoconf pkg-config \
      bsdmainutils python3 curl git unzip zip file rsync make cmake patch bzip2 xz-utils \
      ${EXTRA_PACKAGES} \
      && break || { echo "apt-get install failed (attempt ${attempt}), retrying"; sleep 20; }; done \
 && dpkg -s build-essential automake > /dev/null \
 && rm -rf /var/lib/apt/lists/* \
 && sh -c "${POST_INSTALL}"

WORKDIR /build
CMD ["/bin/bash"]

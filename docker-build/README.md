# Palladium Core Docker Build System

Reproducible release builds of the Palladium Core binaries, run entirely inside Docker on an `amd64` Ubuntu host:

| Target | Host triple | Binaries |
| --- | --- | --- |
| `linux-x86_64` | `x86_64-pc-linux-gnu` | `palladiumd`, `palladium-cli`, `palladium-tx`, `palladium-wallet`, `palladium-qt` |
| `linux-aarch64` | `aarch64-linux-gnu` | same |
| `linux-armv7l` | `arm-linux-gnueabihf` | same |
| `windows` | `x86_64-w64-mingw32` | same, as `.exe` (optional NSIS installer) |

The same commit gives the same binaries, byte for byte, whoever builds it.

## Quick start

```bash
cd docker-build
./build.sh linux-x86_64              # or linux-aarch64, linux-armv7l, windows
./build.sh windows --installer       # binaries and installer
./build-menu.sh                      # the same, from a menu
```

Output, in `build/<target>/` (at the root of the repository, ignored by git):
- the stripped binaries
- `SHA256SUMS`: the checksums to publish and to compare
- `build-info.txt`: commit, source date, target and image of the build
- the installer, if requested

Environment variables of `build.sh`:

| Variable | Default | Meaning |
| --- | --- | --- |
| `REF` | `HEAD` | commit or tag to build |
| `OUT_DIR` | `build/<target>` | where the output goes |
| `JOBS` | all cores | parallel jobs |
| `CACHE_DIR` | `build/cache` | download cache of the `depends` sources |

## Prerequisites

- Docker, usable by your user
- an `amd64` host (the scripts use `--platform=linux/amd64`; on other hosts it needs `qemu`/`binfmt` and is slow)
- free disk space: about 15 GB, plus the cache
- network access: the image, the Ubuntu snapshot and the `depends` sources are downloaded; the sources are checked against the hashes in `depends/packages/`

The first build of a target takes 30 to 60 minutes (Qt is compiled from source); later ones reuse the download cache.

## What makes the build reproducible

- **Build image**: [Dockerfile](Dockerfile) starts from `ubuntu:20.04` pinned by digest and installs the packages from a dated snapshot of the Ubuntu archive (`APT_SNAPSHOT`). The image is not built from the source tree; the source never enters the image.
- **Source**: only committed files are built. `build.sh` exports the commit with `git archive`, so local changes and untracked files never end up in a binary (a warning says so when there are uncommitted changes).
- **Dependencies**: Qt, Boost, libevent, ... all come from `depends/`, for every target.
- **Time and path**: `SOURCE_DATE_EPOCH` is the commit time; the build path is always `/build/src` and is also replaced in what the compiler writes (`-ffile-prefix-map`); `--enable-determinism`; the dates of Qt resources are fixed (`QT_RCC_SOURCE_DATE_OVERRIDE`); `TZ=UTC`, `LC_ALL=C`.
- **Output**: stripped binaries and `SHA256SUMS`.

### Check it

```bash
./check-reproducible.sh linux-x86_64
```

It builds the same commit twice with a different number of jobs and compares the two `SHA256SUMS`: exit code 0 only if every binary is identical. If they differ, compare `build/reproducible-check/<target>/a` and `/b` (for example with `diffoscope`).

Result of the check on Ubuntu amd64, with the commit of this repository:

| Target | Two builds identical |
| --- | --- |
| `linux-x86_64` | yes |
| `windows` (binaries) | yes |
| `linux-aarch64` | yes |
| `linux-armv7l` | yes |

The Windows installer (NSIS) is not part of the check: its timestamps are not controlled.

### Verify a release

1. `git checkout <tag>`, then `cd docker-build && ./build.sh <target>`.
2. Compare `build/<target>/SHA256SUMS` with the published one: they must be identical.

Two builders on different machines with the same result is the strongest check available here; the check above only compares two builds on the same machine.

### Changing the pins

The digest of the base image and `APT_SNAPSHOT` change what is built, so change them on purpose, in a commit of their own, and write it in `CHANGELOG.md`:

```bash
docker pull --platform=linux/amd64 ubuntu:20.04
docker inspect --format '{{index .RepoDigests 0}}' ubuntu:20.04     # new digest
# APT_SNAPSHOT: a date for which https://snapshot.ubuntu.com/ubuntu/<date>/ exists
```

## How it works

`build.sh <target>`:
1. exports the commit with `git archive` into a temporary tar;
2. builds the image from the [Dockerfile](Dockerfile), with the packages of the target as build arguments;
3. runs [in-container-build.sh](in-container-build.sh) in a container; only the tar and the script (read-only), the download cache and the output directory are mounted;
4. inside: `depends` for the host triple, `autogen.sh`, `configure`, `make`, strip, `SHA256SUMS`.

There is no cleanup step: every build starts from a fresh export in a new container, so nothing from an earlier build or from the host can be reused.

The Qt tools (`moc`, `uic`, `rcc`) are taken from `depends` (`PATH` and `--with-qt-bindir`), never from the system. The Qt version is in `depends/packages/qt.mk`.

## Troubleshooting

- **`Unable to locate package` or `Failed to fetch` while building the image**: the snapshot service sometimes skips a file or answers with 5xx. The Dockerfile retries `apt-get update` and `install` up to 6 times; if it still fails, run the build again.
- **No space left**: `docker system prune` removes unused images and build cache; the download cache is in `build/cache`.
- **Different hashes on two machines**: compare the `build-info.txt` of both (same commit, same image?), then use `diffoscope` on the differing binary.
- **Stale or wrong output**: `rm -rf build/<target>` and build again.

## Files

- [build.sh](build.sh): builds one target
- [in-container-build.sh](in-container-build.sh): what runs inside the container
- [check-reproducible.sh](check-reproducible.sh): builds twice and compares
- [build-menu.sh](build-menu.sh): interactive menu
- [Dockerfile](Dockerfile): the build image

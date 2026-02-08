# Palladium Core Docker Build System

This directory contains Docker-based build scripts for:
- Linux `x86_64`
- Linux `aarch64`
- Linux `armv7l`
- Windows `x86_64` (`.exe` binaries, optional NSIS installer)

Builds are executed entirely inside containers. Only the output directory is mounted back to the host (`build/...`).

## Prerequisites

- Docker installed and running
- Access to Docker daemon (`docker build` / `docker run`)
- Recommended free disk space: 15+ GB
- Linux host recommended (Ubuntu 20.04+)

Platform note:
- Scripts use `--platform=linux/amd64`.
- On `amd64` hosts this runs natively.
- On non-`amd64` hosts, compatible emulation (`qemu`/`binfmt`) is required and builds are slower.

## Quick Start (Interactive Menu)

```bash
cd docker-build
./build-menu.sh
```

Menu options:
- `1`: Linux `x86_64`
- `2`: Linux `aarch64` (ARM64)
- `3`: Linux `armv7l` (ARM 32-bit)
- `4`: Windows `x86_64`
- `4a`: Windows `x86_64` (Installer)
- `5`: Build all targets (binaries + Windows installer)
- `6`: Build all installers
- `0`: Exit

You can select multiple entries, for example `1 4a`.

## Build Targets

### Linux x86_64

```bash
cd docker-build
./build-linux-x86_64.sh
```

Output: `build/linux-x86_64/`
- `palladiumd`
- `palladium-cli`
- `palladium-tx`
- `palladium-wallet`
- `palladium-qt`

### Linux aarch64

```bash
cd docker-build
./build-linux-aarch64.sh
```

Output: `build/linux-aarch64/`
- `palladiumd`
- `palladium-cli`
- `palladium-tx`
- `palladium-wallet`
- `palladium-qt`

### Linux armv7l

```bash
cd docker-build
./build-linux-armv7l.sh
```

Output: `build/linux-armv7l/`
- `palladiumd`
- `palladium-cli`
- `palladium-tx`
- `palladium-wallet`
- `palladium-qt`

### Windows x86_64

Binaries:

```bash
cd docker-build
./build-windows.sh
```

Binaries + installer:

```bash
cd docker-build
./build-windows.sh --installer
```

Output: `build/windows/`
- `palladiumd.exe`
- `palladium-cli.exe`
- `palladium-tx.exe`
- `palladium-wallet.exe`
- `palladium-qt.exe`
- Installer (if requested): `palladium-*-win64-setup.exe`

## Technical Build Flow

Each build script does the following:
1. Builds a Docker image from the current repository (`COPY . /src`).
2. Starts a container and runs the build in `/src`.
3. Performs an aggressive cleanup to avoid stale host artifacts.
4. Builds `depends` for the target host triple.
5. Runs `autogen.sh`, `configure`, `make -j$(nproc)`.
6. Copies final binaries into `/out` (host-mounted output directory).
7. Applies host UID/GID ownership to output files.

Key details:
- Linux targets use host triples:
  - `x86_64-pc-linux-gnu`
  - `aarch64-linux-gnu`
  - `arm-linux-gnueabihf`
- Windows target uses:
  - `x86_64-w64-mingw32`
- Windows installer generation uses `make deploy` when `--installer` is passed.

## Qt Toolchain Notes

To avoid Qt tool mismatch issues, Linux scripts force Qt tools from `depends`:
- prepend `depends/<host>/bin` to `PATH`
- pass `--with-qt-bindir=$PWD/depends/<host>/bin` to `configure`

Qt version used by `depends` is defined in `depends/packages/qt.mk`:
- currently `5.9.8`

## Cleanup Strategy (Stale Artifact Protection)

Scripts run cleanup inside the container before building:
- `make distclean` (if available)
- remove `depends/<host>`
- remove `config.cache`
- Windows script also cleans `univalue` build/caches explicitly:
  - `univalue/.libs`
  - `univalue/lib/.libs`
  - `univalue/config.cache`
  - `univalue/config.status`

This prevents ABI/toolchain mixing between host-compiled and container-compiled objects.

## Troubleshooting

### `QT_INIT_METAOBJECT` errors

Symptom:
- compile errors in generated `.moc` files

Typical cause:
- `moc` from system Qt mixed with headers/libs from `depends` Qt

Mitigation already in scripts:
- force `depends` Qt tools (`PATH` + `--with-qt-bindir`)

### Linker errors such as `__isoc23_strtol` or missing UniValue symbols

Examples:
- `__isoc23_strtol`
- `UniValue::setBool(bool)`
- `UniValue::getValues() const`

Typical cause:
- stale host-built objects reused in container link stage

Mitigation already in scripts:
- pre-build cleanup, including dedicated `univalue` cleanup for Windows builds

### Executable bit issues

```bash
cd docker-build
chmod +x *.sh
```

### Force a clean host-side output

Usually not required, but useful after interrupted builds:

```bash
rm -rf build/linux-* build/windows
```

## Main Files

- `docker-build/build-menu.sh`
- `docker-build/build-linux-x86_64.sh`
- `docker-build/build-linux-aarch64.sh`
- `docker-build/build-linux-armv7l.sh`
- `docker-build/build-windows.sh`
- `docker-build/Dockerfile.linux-x86_64`
- `docker-build/Dockerfile.linux-aarch64`
- `docker-build/Dockerfile.linux-armv7l`
- `docker-build/Dockerfile.windows`

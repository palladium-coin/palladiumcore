# Docker Build System for Palladium Core

Docker-based build system for creating Palladium Core binaries using Ubuntu 20.04+ containers on x86_64 architecture.

## Build Environment

All builds use:
- **Base OS**: Ubuntu 20.04 or newer
- **Container Platform**: Docker
- **Host Architecture**: x86_64 (AMD64)
- **Build Method**: Cross-compilation for target platforms

## Linux x86_64 Build

### Overview

This build creates native Linux x86_64 binaries using a Docker container based on Ubuntu 20.04. The build process:

1. **Builds a Docker image** (`palladium-builder:linux-x86_64-ubuntu20.04`) containing all build dependencies
2. **Copies the entire repository** into the container during image build
3. **Runs the build process** entirely inside the container using the `depends` system
4. **Outputs binaries** to a mounted volume for host access

### Quick Start

```bash
cd docker-build
chmod +x build-linux-x86_64.sh
./build-linux-x86_64.sh
```

Binaries will be available in `../build/linux-x86_64/` directory.

### Prerequisites

- Docker installed and running ([installation guide](https://docs.docker.com/get-docker/))
- Sufficient disk space for the build process (at least 10 GB free)
- Internet connection for downloading dependencies

### Produced Binaries

The following binaries are built and copied to the output directory:

- `palladiumd`: Main daemon
- `palladium-cli`: Command-line client  
- `palladium-tx`: Transaction utility
- `palladium-wallet`: Wallet utility
- `palladium-qt`: GUI application (if Qt dependencies are available)

### Output Directory

Binaries are placed in: `../build/linux-x86_64/` (relative to the docker-build directory)

### Troubleshooting

**Permission errors:** The build script automatically handles file permissions using host UID/GID

**Build failed:** Run interactive container for debugging:
```bash
docker run --rm -it -v "$(pwd)/../build/linux-x86_64":/out palladium-builder:linux-x86_64-ubuntu20.04 bash
```

**Docker build issues:** Ensure Docker has sufficient resources and the daemon is running

**Missing dependencies:** The Dockerfile installs all required build dependencies automatically

## Linux aarch64 Build

### Overview

This build creates Linux aarch64 (ARM64) binaries specifically designed for single board computers like the popular Raspberry Pi and other ARM-based devices. The build process uses cross-compilation in a Docker container:

1. **Builds a Docker image** (`palladium-builder:linux-aarch64-ubuntu20.04`) containing ARM64 cross-compilation toolchain
2. **Copies the entire repository** into the container during image build
3. **Cross-compiles for ARM64** using the `depends` system with `aarch64-linux-gnu` target
4. **Outputs binaries** to a mounted volume for host access

### Quick Start

```bash
cd docker-build
chmod +x build-linux-aarch64.sh
./build-linux-aarch64.sh
```

ARM64 binaries will be available in `../build/linux-aarch64/` directory.

### Prerequisites

- Docker installed and running ([installation guide](https://docs.docker.com/get-docker/))
- Sufficient disk space for the build process (at least 12 GB free)
- Internet connection for downloading dependencies and ARM64 cross-compilation toolchain

### Target Devices

This build is optimized for:
- **Raspberry Pi 4** and newer models
- **Raspberry Pi 3** (64-bit mode)
- Other ARM64-based single board computers
- ARM64 Linux servers and workstations

### Produced Binaries

The following ARM64 binaries are built and copied to the output directory:

- `palladiumd`: Main daemon
- `palladium-cli`: Command-line client
- `palladium-tx`: Transaction utility
- `palladium-wallet`: Wallet utility
- `palladium-qt`: GUI application (if Qt dependencies are available)

### Output Directory

Binaries are placed in: `../build/linux-aarch64/` (relative to the docker-build directory)

### Cross-Compilation Details

- **Target Triple**: `aarch64-linux-gnu`
- **Toolchain**: GCC ARM64 cross-compiler
- **Dependencies**: Built using the `depends` system for ARM64 target
- **Configuration**: Uses `CONFIG_SITE` for proper cross-compilation setup

### Troubleshooting

**Permission errors:** The build script automatically handles file permissions using host UID/GID

**Build failed:** Run interactive container for debugging:
```bash
docker run --rm -it -v "$(pwd)/../build/linux-aarch64":/out palladium-builder:linux-aarch64-ubuntu20.04 bash
```

**Cross-compilation issues:** Ensure the ARM64 cross-compilation toolchain is properly installed in the container

**Missing binaries:** Check that the configure script detected the ARM64 target correctly

**Docker build issues:** Ensure Docker has sufficient resources and the daemon is running

## Windows x86_64 Build

### Overview

This build creates Windows x86_64 executables using cross-compilation in a Docker container based on Ubuntu 20.04. The build process:

1. **Builds a Docker image** (`palladium-builder:windows-ubuntu20.04`) containing MinGW-w64 cross-compilation toolchain
2. **Copies the entire repository** into the container during image build
3. **Cross-compiles for Windows** using the `depends` system with `x86_64-w64-mingw32` target
4. **Outputs .exe files** to a mounted volume for host access

### Quick Start

```bash
cd docker-build
chmod +x build-windows.sh
./build-windows.sh
```

Windows executables will be available in `../build/windows/` directory.

### Prerequisites

- Docker installed and running ([installation guide](https://docs.docker.com/get-docker/))
- Sufficient disk space for the build process (at least 12 GB free)
- Internet connection for downloading dependencies and MinGW-w64 toolchain

### Produced Executables

The following Windows executables are built and copied to the output directory:

- `palladiumd.exe`: Main daemon
- `palladium-cli.exe`: Command-line client
- `palladium-tx.exe`: Transaction utility
- `palladium-wallet.exe`: Wallet utility
- `palladium-qt.exe`: GUI application (if Qt dependencies are available)

### Output Directory

Executables are placed in: `../build/windows/` (relative to the docker-build directory)

### Cross-Compilation Details

- **Target Triple**: `x86_64-w64-mingw32`
- **Toolchain**: MinGW-w64 cross-compiler
- **Dependencies**: Built using the `depends` system for Windows target
- **Configuration**: Uses `CONFIG_SITE` for proper cross-compilation setup

### Troubleshooting

**Permission errors:** The build script automatically handles file permissions using host UID/GID

**Build failed:** Run interactive container for debugging:
```bash
docker run --rm -it -v "$(pwd)/../build/windows":/out palladium-builder:windows-ubuntu20.04 bash
```

**Cross-compilation issues:** Ensure the MinGW-w64 toolchain is properly installed in the container

**Missing .exe files:** Check that the configure script detected the Windows target correctly

**Docker build issues:** Ensure Docker has sufficient resources and the daemon is running
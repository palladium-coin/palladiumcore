# Docker Build System for Palladium Core

## System Requirements

- **Host System**: Ubuntu 20.04+ on x86_64 architecture
- **Docker**: Installed and running ([installation guide](https://docs.docker.com/get-docker/))
- **Disk Space**: At least 15 GB free

## Unified Build Menu (Recommended)

The easiest way to build binaries for one or multiple platforms is using the interactive menu script.

### Usage

```bash
cd docker-build
./build-menu.sh
```

You will be prompted to select the target platforms:
- `1`: Linux x86_64
- `2`: Linux aarch64
- `3`: Linux ARMv7l
- `4`: Windows x86_64
- `5`: Build ALL targets

You can also select multiple targets by separating them with spaces (e.g., `1 4`).

## Linux x86_64

Creates native Linux 64-bit binaries using a Docker container based on Ubuntu 20.04.

### Commands

```bash
cd docker-build                    # Navigate to the docker-build directory
./build-linux-x86_64.sh            # Run the build script
```

### Output

Binaries will be available in `../build/linux-x86_64/`:
- `palladiumd` - Main daemon
- `palladium-cli` - Command-line client
- `palladium-tx` - Transaction utility
- `palladium-wallet` - Wallet utility
- `palladium-qt` - GUI application

## Linux aarch64 (ARM64)

Creates ARM64 binaries for devices like Raspberry Pi 4+ through cross-compilation.

### Commands

```bash
cd docker-build                     # Navigate to the docker-build directory
./build-linux-aarch64.sh            # Run the cross-compilation build script
```

### Output

Binaries will be available in `../build/linux-aarch64/`:
- `palladiumd` - Main daemon
- `palladium-cli` - Command-line client
- `palladium-tx` - Transaction utility
- `palladium-wallet` - Wallet utility
- `palladium-qt` - GUI application

## Linux ARMv7l (ARM 32-bit)

Creates ARM 32-bit binaries for older devices like Raspberry Pi 2/3 and Pi Zero.

### Commands

```bash
cd docker-build                     # Navigate to the docker-build directory
./build-linux-armv7l.sh             # Run the ARMv7l cross-compilation build script
```

### Output

Binaries will be available in `../build/armv7l/`:
- `palladiumd` - Main daemon
- `palladium-cli` - Command-line client
- `palladium-tx` - Transaction utility
- `palladium-wallet` - Wallet utility
- `palladium-qt` - GUI application

## Windows x86_64

Creates Windows executables through cross-compilation with MinGW-w64.

### Commands

```bash
cd docker-build                     # Navigate to the docker-build directory
./build-windows.sh                  # Run the Windows cross-compilation build script
```

### Output

Executables will be available in `../build/windows/`:
- `palladiumd.exe` - Main daemon
- `palladium-cli.exe` - Command-line client
- `palladium-tx.exe` - Transaction utility
- `palladium-wallet.exe` - Wallet utility
- `palladium-qt.exe` - GUI application

### Troubleshooting

```bash
cd docker-build
chmod +x *.sh
```

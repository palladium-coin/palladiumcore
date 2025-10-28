# Docker Build System for Palladium Core

Docker-based build system for creating Palladium Core binaries.

## Quick Start

```bash
cd docker-build
chmod +x build-linux-x86_64.sh
./build-linux-x86_64.sh
```

Binaries will be available in the `out/` directory.

## Prerequisites

- Docker installed and running

## Produced Binaries

- `palladiumd`: Main daemon
- `palladium-cli`: Command-line client
- `palladium-tx`: Transaction utility
- `palladium-wallet`: Wallet utility
- `palladium-qt`: GUI (if available)

## Troubleshooting

**Permission errors:** Use `sudo ./build-linux-x86_64.sh`

**Build failed:** Run interactive container for debugging:
```bash
docker run --rm -it -v "$(pwd)":/src palladium-builder:ubuntu20.04 bash
```
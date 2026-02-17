# Palladium Core

[![Release](https://img.shields.io/github/v/release/palladium-coin/palladiumcore)](https://github.com/palladium-coin/palladiumcore/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](COPYING)

**Official Websites:** [palladiumblockchain.net](https://palladiumblockchain.net) | [palladium-coin.com](https://palladium-coin.com)

## Table of Contents

- [What is Palladium Core?](#what-is-palladium-core)
- [Quick Start](#quick-start)
- [Building from Source](#building-from-source)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Version History](#version-history)
- [Support](#support)
- [License](#license)

## What is Palladium Core?

Palladium Core is a decentralized cryptocurrency built on Bitcoin's proven codebase, enhanced with modern features including **Taproot support** (BIP340/341/342/350). Designed for the palladium market ecosystem, it delivers enterprise-grade security, efficiency, and privacy for digital transactions.

### Key Features

- **Taproot Support**: Enhanced privacy with Schnorr signatures and bech32m addresses
- **Security**: Advanced cryptographic techniques ensure transaction security and fund protection
- **Efficiency**: Optimized blockchain parameters provide fast and reliable transaction processing
- **Transparency**: Open-source architecture enables community inspection and contribution
- **Market-Focused**: Tailored features specifically designed for palladium industry requirements
- **Decentralized**: Peer-to-peer network with no central authority

### What's New in v2.0.0

Palladium Core 2.0.0 introduces **full Taproot support**, bringing cutting-edge privacy and efficiency improvements:

- **Schnorr Signatures**: More efficient and secure than traditional ECDSA signatures
- **Bech32m Addresses**: New address format (`plm1p...`) for Taproot outputs
- **Enhanced Privacy**: Complex smart contracts appear as simple transactions on-chain
- **Smaller Transactions**: Lower fees through more efficient signature schemes

**Learn More**: [Taproot User Guide](doc/taproot-guide.md) | [Technical Specification](doc/TAPROOT.md) | [Release Notes](doc/release-notes/release-notes-2.0.0.md)

## Quick Start

### Pre-built Binaries

Download the latest Palladium Core release:

**[Download Latest Release](https://github.com/palladium-coin/palladiumcore/releases/latest)**

**Installation Steps:**

1. **Download**: Get the appropriate binary for your operating system
2. **Install**: Extract and run the installer
3. **Configure**: Create `palladium.conf` (see [Configuration](#configuration) below)
4. **Launch**: Start Palladium Core and sync with the network

**First-time users?** Check out the [Taproot User Guide](doc/taproot-guide.md) to learn about creating modern Taproot addresses with enhanced privacy.

### Configuration

For enhanced connectivity and performance, you can create a configuration file. Palladium Core supports comprehensive configuration options for mainnet, testnet, and regtest networks.

**Configuration File Location:**
- **Windows**: `%appdata%/Palladium/`  
- **Linux**: `/home/[username]/.palladium/`  
- **macOS**: `~/Library/Application Support/Palladium/`

Create a file named `palladium.conf` in the appropriate directory for your operating system.

**Complete Configuration Guide:**
For detailed configuration instructions, network-specific settings, security best practices, and complete configuration examples, please refer to our comprehensive configuration guide: **[Palladium Configuration File Documentation](doc/configuration-file.md)**

## Building from Source

### Docker Build (Recommended)

For a simpler and more reproducible build process, you can use our Docker-based build system. This method provides a consistent build environment and eliminates dependency management issues.

**Requirements:**
- Linux AMD x86_64 system with Ubuntu 20.04 or newer
- Docker installed and running

For detailed instructions and configuration options, see the [docker-build](docker-build/) directory.

### Manual Build Instructions

## Manual Build Instructions

### Quick Build Script (Ubuntu/WSL Only)

**For rapid development and debugging on Ubuntu 20.04 or WSL with Ubuntu 20.04**

We provide a convenient automated build script designed specifically for quick development iterations and GUI testing:

```bash
./quick-build.sh [OPTIONS]
```

**⚠️ Important Note**: This script is optimized for **Ubuntu 20.04** (including WSL with Ubuntu 20.04). It may not work correctly on newer Ubuntu versions due to dependency differences.

#### Available Options:

- `--full` or no arguments: Complete build process (dependencies + BerkeleyDB + compilation) [DEFAULT]
- `--build`: Full build with reconfigure (runs distclean, configure, and make)
- `--rebuild`: Quick incremental rebuild (clean + make, no reconfigure) - **fastest for testing**
- `--install-deps`: Install system dependencies only
- `--install-db`: Install BerkeleyDB 4.8 only
- `--clean`: Clean build artifacts
- `--help`: Show help message

#### Usage Examples:

```bash
# First time setup (installs everything)
./quick-build.sh

# Full rebuild after configuration changes
./quick-build.sh --build

# Quick rebuild after code changes (fastest)
./quick-build.sh --rebuild

# Clean build artifacts
./quick-build.sh --clean
```

#### What It Does:

- **Automated dependency installation**: Installs all required build tools, libraries, and Qt5 for GUI
- **BerkeleyDB 4.8 setup**: Automatically installs BerkeleyDB 4.8 (required for wallet functionality)
- **Optimized compilation**: Uses all available CPU cores with `make -j$(nproc)`
- **ccache support**: Automatically enables ccache for faster recompilation
- **GUI support**: Builds `palladium-qt` for testing the graphical interface

#### Output Binaries:

After successful build, binaries are located at:
- `src/palladiumd` - Daemon
- `src/palladium-cli` - Command-line interface
- `src/qt/palladium-qt` - Qt GUI application

#### Build Workflow Recommendation:

1. **First time**: `./quick-build.sh` (installs everything)
2. **After code changes**: `./quick-build.sh --rebuild` (fastest, keeps configuration)
3. **After configure changes**: `./quick-build.sh --build` (reconfigures everything)

---

For detailed manual build instructions specific to your operating system, please refer to the comprehensive documentation available in the `/doc` folder:

### Platform-Specific Build Guides

- **Unix/Linux Systems**: [`doc/build-unix.md`](doc/build-unix.md)
- **Windows**: [`doc/build-windows.md`](doc/build-windows.md)
- **macOS**: [`doc/build-osx.md`](doc/build-osx.md)
- **FreeBSD**: [`doc/build-freebsd.md`](doc/build-freebsd.md)
- **NetBSD**: [`doc/build-netbsd.md`](doc/build-netbsd.md)
- **OpenBSD**: [`doc/build-openbsd.md`](doc/build-openbsd.md)

### Additional Resources

- **Dependencies Overview**: [`doc/dependencies.md`](doc/dependencies.md) - Complete list of build dependencies
- **Developer Notes**: [`doc/developer-notes.md`](doc/developer-notes.md) - Advanced build configurations and development setup
- **Gitian Building**: [`doc/gitian-building.md`](doc/gitian-building.md) - Deterministic builds for release binaries

Each platform-specific guide includes:
- Required dependencies and installation commands
- Step-by-step build process
- Configuration options and optimizations
- Troubleshooting common build issues
- Platform-specific considerations and best practices

Choose the appropriate guide for your operating system to ensure a successful build process.

## Contributing

We welcome contributions from the community! Please read our [Contributing Guidelines](CONTRIBUTING.md) before submitting pull requests.

### Development Process

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Security

Security is paramount in cryptocurrency development. Please report security vulnerabilities privately to our security team. See [SECURITY.md](SECURITY.md) for details.

## License

Palladium Core is released under the terms of the MIT license. See [COPYING](COPYING) for more information.

## Documentation

### Core Documentation
- **[CHANGELOG](CHANGELOG.md)** - Complete version history and release notes
- **[Taproot User Guide](doc/taproot-guide.md)** - How to use Taproot addresses (v2.0.0)
- **[Taproot Technical Specification](doc/TAPROOT.md)** - Deep dive into Taproot implementation
- **[Release Notes](doc/release-notes/)** - Detailed release notes for each version
- **[Configuration Guide](doc/configuration-file.md)** - Complete configuration reference
- **[BIP Implementation Status](doc/bips.md)** - Bitcoin Improvement Proposals implemented

### Build Documentation
- **[Build Instructions](doc/)** - Platform-specific build guides
- **[Dependencies](doc/dependencies.md)** - Complete dependency list
- **[Developer Notes](doc/developer-notes.md)** - Advanced development setup

### API Documentation
- **[JSON-RPC API](doc/JSON-RPC-interface.md)** - Complete RPC command reference
- **[REST Interface](doc/REST-interface.md)** - RESTful API documentation
- **[ZMQ Interface](doc/zmq.md)** - ZeroMQ notification interface

## Support

- **Documentation**: [Wiki](https://github.com/palladium-coin/palladium/wiki)
- **Issues**: [GitHub Issues](https://github.com/palladium-coin/palladium/issues)
- **Community**: [Discord](https://discord.gg/palladium) | [Telegram](https://t.me/palladiumcoin)
- **Website**: [palladiumblockchain.net](https://palladiumblockchain.net/)

## Version History

For a complete history of changes, improvements, and bug fixes across all versions, see the **[CHANGELOG](CHANGELOG.md)**.

[View all releases](https://github.com/palladium-coin/palladiumcore/releases)

## Acknowledgments

Palladium Core is built upon the Bitcoin Core codebase. We thank the Bitcoin Core developers and the broader cryptocurrency community for their foundational work.

**Taproot Implementation**: The Taproot implementation (v2.0.0) is based on Bitcoin Core v24.2, adapted for Palladium's chain parameters and consensus rules.

---

**Disclaimer**: Cryptocurrency investments carry risk. Please do your own research and invest responsibly.

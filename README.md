# Palladium Core

**Official Websites:** [palladiumblockchain.net](https://palladiumblockchain.net) and [palladium-coin.com](https://palladium-coin.com)
## Overview

Palladium Core is a decentralized digital currency forked from Bitcoin, specifically designed to serve the palladium market ecosystem. Built upon the proven Bitcoin protocol foundation, Palladium Core delivers enhanced security, efficiency, and transparency for palladium-related transactions.

### Key Features

- **Security**: Advanced cryptographic techniques ensure transaction security and fund protection
- **Efficiency**: Optimized blockchain parameters provide fast and reliable transaction processing
- **Transparency**: Open-source architecture enables community inspection and contribution
- **Market-Focused**: Tailored features specifically designed for palladium industry requirements
- **Decentralized**: Peer-to-peer network with no central authority

## Quick Start

### Installation

1. **Download and Install**: Get the latest Palladium Core wallet from our [releases page](https://github.com/palladium-coin/palladiumcore/releases)
2. **Configure**: Create the `palladium.conf` configuration file (see [Configuration](#configuration) section below)
3. **Launch the Core**: Start the Palladium Core application (includes automatic network synchronization)

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

## Support

- **Documentation**: [Wiki](https://github.com/palladium-coin/palladium/wiki)
- **Issues**: [GitHub Issues](https://github.com/palladium-coin/palladium/issues)
- **Community**: [Discord](https://discord.gg/palladium) | [Telegram](https://t.me/palladiumcoin)
- **Website**: [palladiumblockchain.net](https://palladiumblockchain.net/)

## Acknowledgments

Palladium Core is built upon the Bitcoin Core codebase. We thank the Bitcoin Core developers and the broader cryptocurrency community for their foundational work.

---

**Disclaimer**: Cryptocurrency investments carry risk. Please do your own research and invest responsibly.

# Palladium Core

**Official Website:** [palladiumblockchain.net](https://palladiumblockchain.net)
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
2. **Configure**: Create the `palladium.conf` configuration file (see [Configuration](#advanced-configuration) section below)
3. **Launch the Core**: Start the Palladium Core application (includes automatic network synchronization)

### Configuration

For enhanced connectivity and performance, you can create a configuration file:

**Windows**: Navigate to `%appdata%/Palladium/`  
**Linux**: Navigate to `/home/[username]/.palladium/`  
**macOS**: Navigate to `~/Library/Application Support/Palladium/`

Create a file named `palladium.conf`. For users running a full node or requiring advanced functionality, we recommend the following comprehensive configuration:

```conf
# Core Settings
txindex=1
server=1
listen=1
daemon=1
discover=1

# RPC Configuration
rpcuser=your_username_here
rpcpassword=your_secure_password_here

# Network Ports
port=2333
rpcport=2332

# Connection Settings
maxconnections=50
fallbackfee=0.0001
rpcallowip=192.168.0.0/16
rpcbind=192.168.0.0/16

# Trusted Nodes
addnode=89.117.149.130:2333
addnode=66.94.115.80:2333
addnode=173.212.224.67:2333

# ZeroMQ Configuration
zmqpubrawblock=tcp://0.0.0.0:28334
zmqpubrawtx=tcp://0.0.0.0:28335
zmqpubhashblock=tcp://0.0.0.0:28332
```

**Security Warning**: Replace `your_username_here` and `your_secure_password_here` with strong, unique credentials.

### Configuration Parameters Explained

| Parameter | Description | Recommended Value |
|-----------|-------------|-------------------|
| `txindex` | Maintains full transaction index | `1` (enabled) |
| `server` | Enables RPC server | `1` (enabled) |
| `listen` | Accept incoming connections | `1` (enabled) |
| `daemon` | Run in background mode | `1` (enabled) |
| `discover` | Enable peer discovery | `1` (enabled) |
| `maxconnections` | Maximum peer connections | `50` |
| `fallbackfee` | Default transaction fee | `0.0001` PLM/kB |
| `port` | P2P network port | `2333` |
| `rpcport` | RPC server port | `2332` |

## Building from Source

### Docker Build (Recommended)

For a simpler and more reproducible build process, you can use our Docker-based build system. This method provides a consistent build environment and eliminates dependency management issues.

**Requirements:**
- Linux AMD x86_64 system with Ubuntu 20.04 or newer
- Docker installed and running

For detailed instructions and configuration options, see the [docker-build](docker-build/) directory.

### Manual Build Instructions

```bash
# Step 1: Install build dependencies
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3 curl

# Step 2: Build with depends (manages everything automatically)
cd depends
make HOST=x86_64-pc-linux-gnu -j$(nproc)
cd ..

# Step 3: Configure and build
./autogen.sh
./configure --prefix=$PWD/depends/x86_64-pc-linux-gnu \
            --enable-glibc-back-compat \
            --enable-reduce-exports \
            LDFLAGS=-static-libstdc++
make -j$(nproc)

# Step 4: Verify binaries are built
ls -la src/palladiumd src/palladium-cli src/palladium-tx src/palladium-wallet src/qt/palladium-qt

# Step 5: Test that binaries work
./src/palladiumd --version
./src/palladium-cli --version
./src/palladium-tx --version
./src/palladium-wallet --version
./src/qt/palladium-qt --version
```

**Runtime Dependencies (for GUI):**
```bash
sudo apt-get update
sudo apt-get install libqt5gui5 libqt5widgets5 libqt5core5a
```

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

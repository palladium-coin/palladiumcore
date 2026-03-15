# Palladium Configuration File Guide

## Overview

The `palladium.conf` file is the main configuration file for Palladium Core, controlling how your node operates, connects to the network, and provides services. This file allows you to customize various aspects of your Palladium Core installation without needing to use command-line arguments every time you start the application.

## What is palladium.conf?

`palladium.conf` is a plain text configuration file that contains key-value pairs to configure your Palladium Core node. It determines:

- Network connectivity settings
- RPC server configuration
- Security parameters
- Performance optimizations
- Logging and debugging options
- Network-specific settings (mainnet, testnet, regtest)

## Location by Operating System

The configuration file must be placed in the Palladium data directory, which varies by operating system:

### Windows
- **Path**: `%appdata%\Palladium\palladium.conf`
- **Example**: `C:\Users\[Username]\AppData\Roaming\Palladium\palladium.conf`

### Linux
- **Path**: `/home/[username]/.palladium/palladium.conf`
- **Example**: `/home/user/.palladium/palladium.conf`

### macOS
- **Path**: `~/Library/Application Support/Palladium/palladium.conf`
- **Example**: `/Users/[Username]/Library/Application Support/Palladium/palladium.conf`

## Network Configurations

Palladium Core supports three different networks, each with its own configuration requirements:

### 1. Mainnet (Production Network)
Mainnet is the live Palladium blockchain where real transactions occur and real PLM coins are used.

**Key Characteristics:**
- Uses default ports (2333 for P2P, 2332 for RPC)
- Connects to production network
- Real economic value
- Should be used for production deployments only

### 2. Testnet (Testing Network)
Testnet is a separate blockchain designed for testing purposes with no real economic value.

**Key Characteristics:**
- Uses different ports (12333 for P2P, 12332 for RPC)
- Separate blockchain from mainnet
- Test PLM coins have no value
- Safe for development and experimentation
- Faster block times for testing

### 3. Regtest (Regression Testing)
Regtest is a local testing environment where you can create blocks instantly.

**Key Characteristics:**
- Local blockchain only
- Instant block generation
- Complete control over network
- Ideal for development and automated testing
- No external network connections required

## Core Configuration Parameters

### Basic Node Settings
```conf
txindex=1                                # Enable transaction index (required for getrawtransaction RPC)

server=1                                 # Enable RPC server (required for RPC commands)

listen=1                                 # Accept incoming connections (required for peer discovery)

daemon=1                                 # Run as daemon (background process)

discover=1                               # Enable peer discovery (required for peer connections)

maxconnections=50                        # Maximum number of connections
```

### RPC Configuration
```conf
rpcuser=your_secure_username             # RPC username (required for RPC authentication)

rpcpassword=your_secure_password         # RPC password (required for RPC authentication)

rpcallowip=127.0.0.1                     # Allow RPC connections from specific IP ranges (localhost)
rpcallowip=192.168.1.0/24                # Allow RPC connections from specific IP ranges (local network)

rpcbind=127.0.0.1                        # Bind RPC to specific interface
```

### Network Settings
```conf
port=2333                                # P2P network port

rpcport=2332                             # RPC server port

fallbackfee=0.0001                       # Default transaction fee (PLM per kB)

minrelaytxfee=0.00001                    # Minimum relay fee
```

### Security Settings
```conf
# Wallet encryption (set at runtime, not in config file)
# Use encryptwallet RPC command instead

disablewallet=1                          # Disable wallet functionality (for full nodes without wallet)

# Node Connection Methods (choose one approach):
# addnode - Adds node to connection list, still allows other connections
addnode=trusted.node.ip:2333             # Add trusted node to connection pool

# connect - ONLY connects to specified nodes, blocks all other connections
connect=trusted.node.ip:2333             # Connect ONLY to this node (maximum security)
```

### Performance Tuning
```conf
# UTXO database cache size in MB. Higher values significantly speed up initial
# block download and block validation. Default: 450.
# Recommended: 2048 on machines with ≥4GB RAM, 512 on machines with 1-2GB RAM.
dbcache=2048

# Number of threads for parallel script verification in transactions.
# 0 = automatic (uses all available logical cores).
# Set an explicit value to limit CPU usage (e.g. 4 on a quad-core machine).
par=4

maxorphantx=100                          # Maximum number of orphan transactions

maxmempool=300                           # Maximum size of mempool in MB

mempoolexpiry=72                         # Mempool memory expiration time in hours
```

### Privacy Settings
```conf

dnsseed=0                                # Disable DNS seeding for privacy


bind=127.0.0.1                           # Bind to specific interface only (localhost)
```

### Mining and Block Template Configuration
```conf
# Block template creation limits
blockmaxweight=3996000                   # Maximum BIP141 block weight
blockmintxfee=0.00001                    # Minimum fee rate for transactions included in block creation (PLM/kB)

# Relay and mining policy options
bytespersigop=20                         # Equivalent bytes per sigop in transactions for relay and mining
datacarrier=1                            # Relay and mine data carrier (OP_RETURN) transactions
datacarriersize=83                       # Maximum data size for OP_RETURN transactions we relay and mine
minrelaytxfee=0.00001                    # Fee rate threshold for relaying and mining policy (PLM/kB)
```

> **Note:** `mine`, `miningaddress`, `blockmaxsize`, and `blockprioritysize` are not valid configuration options in Palladium Core.
> Mining software should use the `getblocktemplate` RPC with an external miner/pool stack.


## Complete Configuration Examples

### Mainnet Configuration
```conf
# Core Settings
txindex=1
server=1
listen=1
daemon=1
discover=1

# RPC Configuration
rpcuser=your_secure_username
rpcpassword=your_very_secure_password
rpcallowip=127.0.0.1
rpcallowip=192.168.1.0/24
rpcbind=127.0.0.1

# Network Settings (Mainnet defaults)
port=2333
rpcport=2332
maxconnections=50
fallbackfee=0.0001
minrelaytxfee=0.00001

# Trusted Nodes
addnode=89.117.149.130:2333
addnode=66.94.115.80:2333
addnode=173.212.224.67:2333

# Advanced Settings
zmqpubrawblock=tcp://0.0.0.0:28334
zmqpubrawtx=tcp://0.0.0.0:28335
zmqpubhashblock=tcp://0.0.0.0:28332

# Performance (adjust to available hardware)
dbcache=2048
par=4

# Logging
debug=net
debug=rpc
debug=coindb
```

### Testnet Configuration
```conf
# Network Selection
testnet=1

[test]
# Core Settings
txindex=1
server=1
listen=1
daemon=1
discover=1

# RPC Configuration
rpcuser=testnet_user
rpcpassword=testnet_password
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Network Settings (Testnet)
port=12333
rpcport=12332
maxconnections=25
fallbackfee=0.0001
minrelaytxfee=0.00001

# Faster block time for testing
blockmintxfee=0.00001
```

### Regtest Configuration
```conf
# Network Selection
regtest=1

[regtest]
# Core Settings
txindex=1
server=1
listen=1
daemon=1

# RPC Configuration
rpcuser=regtest_user
rpcpassword=regtest_password
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Network Settings (Regtest)
port=23444
rpcport=23443
maxconnections=1  # No external connections needed

# Development Settings
fallbackfee=0.00001
minrelaytxfee=0.000001
blockmintxfee=0.000001

# Enable all debugging for development
debug=1
```

## Unified Configuration File with Network Sections

You can also create a single configuration file that works for all networks using section headers. This approach allows you to switch between networks without changing the entire configuration file.

```conf
# Palladium Core Unified Configuration

# By default, if neither testnet nor regtest is specified, the node runs on mainnet.
# Uncomment ONLY ONE of the following lines to switch network:
# testnet=1   # Enable testnet mode (disables mainnet and regtest)
# regtest=1   # Enable regtest mode (disables mainnet and testnet)

# Mainnet settings (default - no section header needed)
# These settings apply to mainnet by default

txindex=1
server=1
listen=1
daemon=1
discover=1

rpcuser=your_secure_username
rpcpassword=your_very_secure_password
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

port=2333
rpcport=2332
maxconnections=50
fallbackfee=0.0001
minrelaytxfee=0.00001

# Mainnet trusted nodes
addnode=89.117.149.130:2333
addnode=66.94.115.80:2333
addnode=173.212.224.67:2333

[test]
# Testnet specific settings
# These override mainnet settings when using testnet

# Testnet ports
port=12333
rpcport=12332
maxconnections=25

# Testnet RPC (different credentials recommended)
rpcuser=testnet_user
rpcpassword=testnet_password

# Testnet nodes
addnode=<ip-address1>:12333
addnode=<ip-address2>:12333

[regtest]
# Regression test network settings
# These override mainnet settings when using regtest

# Regtest ports
port=28444
rpcport=28443
maxconnections=1

# Regtest RPC (development credentials)
rpcuser=regtest_user
rpcpassword=regtest_password

# Run in foreground for development
daemon=0

# Lower fees for testing
fallbackfee=0.00001
minrelaytxfee=0.000001
blockmintxfee=0.000001

# Enable all debugging for development
debug=1
```

### How Network Selection Works

- **Mainnet**: Used by default when no network is specified (no section header needed)
- **Testnet**: Activated when starting with `-testnet` flag or `testnet=1` in main section
- **Regtest**: Activated when starting with `-regtest` flag or `regtest=1` in main section

When using section headers:
- Settings in `[test]` section only apply when using testnet
- Settings in `[regtest]` section only apply when using regtest
- Mainnet uses settings outside any section or in main section
- Network-specific sections override main settings for that network

## Security Best Practices

1. **Strong RPC Credentials**: Always use strong, unique usernames and passwords for RPC access
2. **Network Restrictions**: Limit `rpcallowip` to only necessary IP ranges
3. **Firewall Configuration**: Configure your firewall to only allow necessary ports
4. **Regular Updates**: Keep your Palladium Core software updated
5. **Backup Configuration**: Keep backups of your configuration file
6. **Testnet First**: Always test configuration changes on testnet before mainnet

## Troubleshooting

### Common Issues

1. **Port Already in Use**: Ensure no other services are using ports 2333/2332
2. **RPC Connection Failed**: Check firewall settings and RPC credentials
3. **Sync Issues**: Verify network connectivity and add trusted nodes
4. **Permission Errors**: Ensure proper file permissions on configuration file

### Debug Options
```conf
# Enable specific debugging categories
debug=net      # Network activity
debug=rpc      # RPC requests
debug=coindb   # Database operations
debug=qt       # GUI operations
debug=mempool  # Memory pool operations
```

## Additional Resources

- [Palladium Core RPC Documentation](JSON-RPC-interface.md)
- [Network Configuration Guide](reduce-traffic.md)
- [Performance Optimization](reduce-memory.md)
- [Developer Documentation](developer-notes.md)

For the most up-to-date information, always refer to the official Palladium Core documentation and release notes.

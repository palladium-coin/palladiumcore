Palladium Core
==============

[![Release](https://img.shields.io/github/v/release/palladium-coin/palladiumcore)](https://github.com/palladium-coin/palladiumcore/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](COPYING)

https://palladiumblockchain.net | https://palladium-coin.com

What is Palladium?
------------------

Palladium (PLM) is a decentralized digital currency with a peer-to-peer network and no central authority.
Palladium Core is the reference client: it connects to the network, downloads and fully validates every block
and transaction, and includes a wallet and a graphical interface (`palladium-qt`).

Palladium Core is a fork of [Bitcoin Core](https://github.com/bitcoin/bitcoin) with these differences:

- **Taproot** (BIP 340/341/342/350): Schnorr signatures, bech32m addresses (`plm1p...`). Active on mainnet from
  block 405000. See the [Taproot guide](doc/taproot-guide.md) and the [technical specification](doc/TAPROOT.md).
- **Difficulty and block time**: from the *Resilience* fork (block 340000) blocks are targeted every 2 minutes
  and the difficulty is adjusted every 60 blocks. Testnet uses LWMA from the genesis block.
  See [doc/palladium-consensus.md](doc/palladium-consensus.md).
- **Network**: mainnet P2P port 2333 and RPC port 2332, testnet 12333 and 12332; bech32 prefixes `plm` and `tplm`.
- **Names**: `palladiumd`, `palladium-cli`, `palladium-qt`, `palladium-tx`, `palladium-wallet`, `palladium.conf`,
  data directory `~/.palladium`.

The history of every release is in the [CHANGELOG](CHANGELOG.md).

Getting started
---------------

Download the binaries from the [releases page](https://github.com/palladium-coin/palladiumcore/releases),
unpack them and run `palladium-qt` (GUI) or `palladiumd` (headless). The first synchronization downloads the
whole chain and can take from a few hours to a day, depending on your machine and connection.

Verify what you download: each release has a `SHA256SUMS` file. The binaries are built reproducibly (see
below), so you can build the same commit yourself and compare the hashes.

Configuration is optional. Create `palladium.conf` in the data directory (`~/.palladium/` on Linux,
`%APPDATA%\Palladium\` on Windows, `~/Library/Application Support/Palladium/` on macOS). All the options, with
examples for mainnet, testnet and regtest, are in the [configuration guide](doc/configuration-file.md).

Building
--------

### Reproducible release builds

The binaries for Linux (x86_64, aarch64, armv7l) and Windows are built inside Docker, from a pinned image and
a pinned Ubuntu package snapshot, so that the same commit gives the same binaries byte for byte:

```bash
cd docker-build
./build.sh linux-x86_64        # or linux-aarch64, linux-armv7l, windows
```

Needs Docker on an amd64 Ubuntu host. See [docker-build/README.md](docker-build/README.md) for the options, how
to check the reproducibility and how to verify a release.

### From source

The build is the usual autotools one:

```bash
./autogen.sh
./configure
make -j"$(nproc)"
```

Instructions for each platform, with the dependencies:

- [Unix / Linux](doc/build-unix.md)
- [Windows](doc/build-windows.md)
- [macOS](doc/build-osx.md)
- [FreeBSD](doc/build-freebsd.md), [NetBSD](doc/build-netbsd.md), [OpenBSD](doc/build-openbsd.md)
- [Dependencies](doc/dependencies.md)

`./quick-build.sh` is a helper for development on Ubuntu 20.04 (or WSL): it installs the dependencies and
BerkeleyDB 4.8 and builds with ccache (`./quick-build.sh --help` lists the options).

Testing
-------

Testing and code review are the bottleneck of the development: please test your changes and help to review
the ones of others.

```bash
make check                          # unit tests
test/functional/test_runner.py      # functional tests
test/lint/lint-all.sh               # lint
```

- [Unit tests and functional tests](test/README.md)
- [Fuzzing](doc/fuzzing.md): libFuzzer harnesses, including the stateful ones for blocks, mempool, RPC, HTTP and
  P2P messages
- [Benchmarks](doc/benchmarking.md)

Every push and pull request runs lint, the unit and functional tests and the fuzz targets on GitHub Actions
(see [.github/workflows/ci.yml](.github/workflows/ci.yml)).

Contributing
------------

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) and the
[developer notes](doc/developer-notes.md) first. In short: fork the repository, work on a topic branch, add tests
for what you change, and open a pull request. Commits use the prefixes of Bitcoin Core (`rpc:`, `net:`,
`wallet:`, `qt:`, `test:`, `doc:`, `build:`).

Changes to the consensus rules (`src/consensus/`, `src/pow.cpp`, `src/validation.cpp`) need particular care and
are reviewed more strictly.

Security
--------

Please report vulnerabilities privately, as described in [SECURITY.md](SECURITY.md). Do not open a public issue.

Documentation
-------------

The index of all the documents is [doc/README.md](doc/README.md). Among them:

- [Configuration file](doc/configuration-file.md)
- [JSON-RPC interface](doc/JSON-RPC-interface.md), [REST interface](doc/REST-interface.md), [ZMQ](doc/zmq.md)
- [Taproot guide](doc/taproot-guide.md), [BIPs implemented](doc/bips.md)
- [Release process](doc/release-process.md)

Support
-------

- Issues: [GitHub Issues](https://github.com/palladium-coin/palladiumcore/issues)
- Community: [Discord](https://discord.gg/palladium) | [Telegram](https://t.me/palladiumcoin)
- Websites: [palladiumblockchain.net](https://palladiumblockchain.net) | [palladium-coin.com](https://palladium-coin.com)

License
-------

Palladium Core is released under the terms of the MIT license. See [COPYING](COPYING).

Palladium Core is built on the work of the Bitcoin Core developers and of the wider community. The Taproot
implementation is based on Bitcoin Core v24.2, adapted to the chain parameters and consensus rules of Palladium.

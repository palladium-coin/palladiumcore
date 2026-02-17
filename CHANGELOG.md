# Changelog

All notable changes to Palladium Core will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.1] - 2026-02-17

### Fixed
- **Critical Taproot consensus fix**: `ConnectBlock` now provides `spent_outputs` to `PrecomputedTransactionData` so BIP341 sighash verification works during block validation
- Fixed Taproot annex hashing to use the BIP341 tagged hash `TapAnnex`
- Fixed wallet script learning to include `BECH32M` in `LearnAllRelatedScripts`
- Fixed Taproot internal key persistence across wallet restarts by storing and loading output-key to internal-pubkey mappings in wallet DB
- Added Taproot mapping/script backfill on wallet load to restore ownership/signing behavior for existing wallets created before the persistence fix

### Changed
- Updated version to `2.0.1` in build/version metadata (`configure.ac`, `build_msvc/palladium_config.h`, man pages)

### Documentation
- Corrected Taproot activation threshold documentation from 90% to 75%

### Tests
- Strengthened `feature_taproot.py` with address ownership checks (`ismine`), restart persistence coverage, and explicit mempool-to-block confirmation checks
- Registered `feature_taproot.py` in `test/functional/test_runner.py`

## [2.0.0] - 2026-02-08

### Added
- Full Taproot support implementing BIP340 (Schnorr Signatures), BIP341 (Taproot), BIP342 (Tapscript), and BIP350 (Bech32m)
- Schnorr signature support via secp256k1 library upgrade to Bitcoin Core v24.2
- Bech32m address format for Taproot outputs (addresses starting with `plm1p...`)
- Address type dropdown in GUI Receive tab for selecting legacy/P2SH-SegWit/bech32/bech32m
- `getnewaddress` RPC support for "bech32m" address type parameter
- XOnlyPubKey class for 32-byte x-only public key representation
- Taproot deployment via BIP9 soft fork (bit 2, mainnet activation February 2026-March 2027)
- Functional test `feature_taproot.py` for end-to-end Taproot lifecycle validation
- Comprehensive technical documentation in doc/TAPROOT.md
- User-friendly Taproot guide in doc/taproot-guide.md

### Changed
- Upgraded secp256k1 library to Bitcoin Core v24.2 with extrakeys and schnorrsig modules enabled
- Updated script interpreter with full taproot/tapscript validation rules (+469 lines)
- Default address type remains "bech32" for backward compatibility

### Documentation
- Added doc/TAPROOT.md: comprehensive technical specification
- Added doc/taproot-guide.md: user-friendly guide for wallet users
- Updated doc/bips.md with BIP340/341/342/350 implementation details
- Restructured release notes: doc/release-notes/release-notes-2.0.0.md
- Updated man pages for all binaries to version 2.0.0

[Detailed release notes](doc/release-notes/release-notes-2.0.0.md)

## [1.5.1] - 2026-01-26

### Added
- New Palladium logo and improved branding
- Rebuild option in quick-build.sh script

### Changed
- Improved splash screen text visibility with shadow effect and larger font
- Updated copyright year to 2026
- Widened transaction amount column header for better readability
- Enhanced modal overlay UI with border and semi-transparent background

### Fixed
- Dark mode input field heights now match light mode
- Dark mode spinbox button visibility improved
- Text clipping in modal overlay resolved
- Window minimum size properly set
- UI inconsistencies across light and dark themes
- Compilation issues with modern GCC (Ubuntu 24.04+)
- Missing argument handling in build-windows.sh
- ccache handling in quick-build.sh

### Documentation
- Added .gitignore for backup files

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.5.1)

## [1.5.0] - 2025-12-10

### Added
- **Hard Fork Logic activated at block 340,000**: Chat and Difficulty Adjustment Algorithm (DAA)
- Better difficulty adjustment at block 350,000
- Update checker functionality
- Combine script utility
- Additional parameters to chainparams.cpp

### Changed
- Improved difficulty adjustment algorithm for better network stability
- Updated modal overlay and GUI files
- Replaced deprecated `state.DoS` call with `state.Invalid`

### Fixed
- Linker errors resolved
- Various stability improvements and bug fixes
- GUI component fixes

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.5.0)

## [1.4.4] - 2025-11-19

### Changed
- Version bump to 1.4.4 with updates to configure.ac, MSVC config, clientversion.cpp, and man pages

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.4)

## [1.4.3] - 2025-11-18

### Added
- **Dark Mode support** with toggle switch in Settings menu
- Dark theme stylesheet (dark.qss)
- Theme preference persistence using QSettings
- Theme toggle functionality in GUI

### Changed
- Settings menu enhanced with Dark Mode checkbox option

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.3)

## [1.4.2] - 2025-11-18

### Added
- Testnet seed nodes for peer discovery
- Allow minimum difficulty blocks after testnet inactivity

### Changed
- Updated testnet consensus parameters
- Simplified testnet difficulty adjustment logic
- Updated BIP34 height and hash for testnet
- Allow `getblocktemplate` during initial block download for testnet

### Fixed
- Testnet chain parameters updated
- Testnet difficulty calculation rules adjusted
- Restored `getblocktemplate` security check
- Wallet confirmation time corrected

### Documentation
- Added troubleshooting section to README
- Moved detailed configuration to separate document
- Updated repository URLs throughout documentation
- Removed outdated configuration details

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.2)

## [1.4.1] - 2025-11-11

### Added
- **Docker build system** for cross-platform compilation
  - Linux x86_64 Docker build infrastructure
  - Windows cross-compilation support via Docker
  - ARM64 (aarch64) support for Raspberry Pi and similar devices
  - ARMv7l architecture support
- Testnet and regtest support enabled with magic byte changes

### Changed
- Simplified version string formatting using semantic versioning
- Replaced CLIENT_BUILD with FormatVersion function
- Testnet3 renamed to testnet across all references
- Restructured Docker build system for better organization

### Fixed
- **Critical: Node synchronization** - Fresh nodes can now fully synchronize from genesis
- Chainwork parameter corrected in chainparams.cpp
- Consensus parameters updated to enable initial blockchain sync

### Documentation
- Build instructions reorganized into platform-specific documents
- README restructured with Docker build emphasis
- Simplified build instructions by linking to platform-specific docs
- Updated repository URLs throughout documentation

### Removed
- Inactive mainnet seed node removed from node list

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.1)

## [1.4.0] - 2025-10-23

### Added
- **AuxPow (Auxiliary Proof of Work)** support
- New seed nodes and DNS seeders
- Comprehensive project documentation in README
- LaTeX format whitepaper (converted from PDF)
- New Palladium logo assets (128px and 250px)
- Mining pool configuration support

### Changed
- Major update to mining and work parameters
- Easier compilation process
- Updated palladium.conf with new defaults
- Stability improvements across the codebase
- Updated website information
- Enhanced splash screen initialization messages

### Fixed
- Build failure by including `<limits>` header in corelib
- Various stability and performance fixes

### Documentation
- Updated README with comprehensive installation instructions
- Whitepaper converted to LaTeX format for better maintainability
- Updated build instructions for macOS
- Improved issue templates and bug report forms
- Updated release notes structure

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.0)

## [1.3.0] - 2024-04-07

### Added
- **LWMA (Lightweight Moving Average) difficulty adjustment algorithm**
- Updated chain parameter seeds (chainparamsseeds.h)

### Changed
- Mining logic modifications (mining.cpp)
- Proof-of-work implementation updates (pow.cpp)
- Chain parameters updated (chainparams.cpp)
- Client version information updated
- Configuration header changes (palladium_config.h)

### Fixed
- macOS compilation issues resolved
- Various bug fixes and stability improvements

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.3.0)

## [1.2.0] - 2024-04-04

### Added
- DNS seeds and fixed seeds for network bootstrap
- Built-in DNS seeder support
- Checkpoints for blockchain validation

### Changed
- Chain parameters updated across multiple commits (chainparams.cpp)
- Soft fork block time and halving schedule adjustments
- Validation logic updates (validation.cpp)
- Wallet functionality improvements (wallet.cpp)
- Database handling updates (db.cpp)
- Message handling improvements (message.cpp)

### Fixed
- SegWit functionality fixes

### Documentation
- Updated README with DNS seeder information
- Updated copyright notices
- Man pages updated (palladium-cli.1, palladium-tx.1, palladium-wallet.1)

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.2.0)

## [1.1.0] - 2024-03-15

Initial public release of Palladium Core.

### Features
- Bitcoin Core codebase foundation
- SegWit support
- Basic wallet functionality
- P2P networking
- Mining support
- RPC interface

[Release on GitHub](https://github.com/palladium-coin/palladiumcore/releases/tag/v1.1.0)

---

[Unreleased]: https://github.com/palladium-coin/palladiumcore/compare/v2.0.1...HEAD
[2.0.1]: https://github.com/palladium-coin/palladiumcore/releases/tag/v2.0.1
[2.0.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v2.0.0
[1.5.1]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.5.1
[1.5.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.5.0
[1.4.4]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.4
[1.4.3]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.3
[1.4.2]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.2
[1.4.1]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.1
[1.4.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.4.0
[1.3.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.3.0
[1.2.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.2.0
[1.1.0]: https://github.com/palladium-coin/palladiumcore/releases/tag/v1.1.0

# Changelog

All notable changes to Palladium Core will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.1.0] - 2026-10-22

Mainnet consensus rules are unchanged. One testnet validation rule is stricter (see Changed).

### Added
- Reproducible Docker builds (`docker-build/`; `build-linux-*.sh` and `build-windows.sh` are replaced by `build.sh <target>`, and the four Dockerfiles by one): one `Dockerfile` pinned by image digest and Ubuntu archive snapshot, one `build.sh` for all targets (Linux x86_64, aarch64, armv7l and Windows) that builds the exported commit with fixed time, path and Qt resource dates, strips the binaries and writes `SHA256SUMS`; `check-reproducible.sh` builds twice and compares. Verified on Ubuntu amd64 for all four targets (the Windows installer excluded): two builds give identical binaries.
- `-taprootheight` option to set the Taproot activation height (regtest only)
- Taproot support in the wallet: `dumpprivkey`, watch-only keys, `importaddress` and `importmulti` now recognise and spend Taproot outputs of imported keys
- `doc/palladium-consensus.md` (consensus differences from Bitcoin Core) and `CLAUDE.md`

### Changed
- **Testnet**: the difficulty (`nBits`) of every header is now compared with the expected value. Before, `ContextualCheckBlockHeader` skipped the comparison for blocks more than 4 minutes after their parent, so such a block could claim any difficulty. The 20 minute minimum difficulty rule is already part of `GetNextWorkRequired`. Testnet nodes should upgrade together; mainnet is not affected.
- Taproot spends are not relayed before the activation height (policy only)
- The pre-fork OP_RETURN size limit is checked against the next block (`Height() + 1`), so a transaction accepted just before the fork height no longer fails in the first block of the fork (relay policy, not consensus)
- Tip staleness and sync estimates use the post-fork block spacing (2 minutes)
- Mainnet `chainTxData`, `nMinimumChainWork` and `defaultAssumeValid` (now block 541892) are updated; the signatures of the whole chain were verified with `-assumevalid=0` up to that block
- `getblockchaininfo` reports Taproot as a buried softfork
- `estimatesmartfee` returns the `-fallbackfee` rate when there is not enough data
- Regtest: RPC port is now 28443 (P2P port minus 1 pattern), `nPowTargetSpacingV2` is set, and the difficulty is fixed (never retargets)
- `pow.cpp` uses named constants and consensus parameters instead of hardcoded fork heights and timespan
- `-walletdir` with a trailing path separator is canonicalised
- Man pages and examples use the Palladium default ports
- `update_version.sh` takes the version as an argument (`--rc N`, `--dev`), also sets `CLIENT_VERSION_RC` and `CLIENT_VERSION_IS_RELEASE`, updates the date of the man pages and checks the result

### Removed
- Gitian (`contrib/gitian-*`, `doc/gitian-building.md`) and Guix (`contrib/guix/`): the release binaries are built with `docker-build/`
- The inherited Travis, Cirrus and AppVeyor configuration and the `ci/` scripts, replaced by the GitHub Actions workflow
- `INSTALL.md` and `.python-version`

### Fixed
- Signed overflow (undefined behaviour) when a peer sends an extreme timestamp in its `version` message; found by the new `process_messages` fuzz target. The value is clamped before the clock offset is computed.
- A `getheaders` request dropped while another was in flight is now sent when the headers reply arrives; before, a burst of block announcements could leave the node a few blocks behind until the next announcement
- Regtest difficulty rose on every block (LWMA branch taken), so around height 935 `generate` could silently return no block
- `SCRIPT_ERR_SCHNORR_SIG_HASHTYPE` is reported when the sighash of a Schnorr signature fails
- A Taproot address no longer resolves to its untweaked internal key in `AddrToPubKey` (used by `createmultisig` and `addmultisigaddress`)
- `make check` passes several Boost suites per test file

### Tests
- GitHub Actions workflow (`.github/workflows/ci.yml`): lint, build with unit and functional tests, and the fuzz targets built with libFuzzer, ASan and UBSan and run on the seed corpus
- Unit tests for the difficulty rules in `pow_tests` (difficulty reset window, LWMA, Resilience fork retarget, testnet minimum difficulty, regtest) and a new `chainparams_tests` suite
- Functional tests `feature_taproot_activation.py` (Taproot activation height) and `feature_opreturn_prefork.py` (pre-fork OP_RETURN policy); `-walletdir` trailing separator case in `wallet_multiwallet.py`
- Fuzz harnesses in `src/test/fuzz/` (`pow`, `taproot_verify`, `taproot_tweak`, `key_io`, script, transaction, block and PSBT harnesses, `process_message`, network, parsing, fee rate, bloom filter, key, script, address manager, check queue and merkle block harnesses, node-level `process_block`, `mempool_accept`, `rpc`, `http_request` and `process_messages` harnesses, and deserialization of 31 types) with a small seed corpus in `test/fuzz/corpus/`; the fuzz targets listed in `src/Makefile.test.include` had no sources before

### Documentation
- Documented the known limit of the LWMA (zero target if every target of the window is below about 2^30, unreachable on a real chain) in `doc/palladium-consensus.md` and `src/pow.cpp`; no behaviour change. Fixed the order of the difficulty rules in that page (regtest never retargets)
- Added `doc/testing.md` (what the tests cover) and rewrote `doc/fuzzing.md` for the harnesses that exist
- Restructured `README.md` and rewrote `doc/release-process.md` for the Docker build, the signed `SHA256SUMS` and the GitHub release; `docker-build/README.md` describes the reproducibility check and its limits

## [2.0.3] - 2026-03-12

### Fixed
- **Critical consensus fix**: replaced BIP9-based Taproot activation with a hardcoded height (`TaprootHeight = 405000`), resolving a chain split where miners signaling BIP9 bit 2 caused Taproot to activate at height 385200 before the implementation was deployed. New nodes now accept all pre-405000 blocks without applying Taproot script rules, unblocking nodes stuck at height 385246.
- On startup, BLOCK_FAILED flags are automatically cleared for blocks below `TaprootHeight`, so nodes upgrading from old software resync without requiring a manual `reconsiderblock` command.
- Updated `defaultAssumeValid` and `nMinimumChainWork` to height 385246; added checkpoints at 350000, 370000 and 385246.
- Updated `chainTxData` to reflect current network state.

### Documentation
- Added `dbcache` and `par` performance options with recommended values to `share/examples/palladium.conf` and `doc/configuration-file.md`.

## [2.0.2] - 2026-03-08

### Added
- Async Qt `InitExecutor` for startup/shutdown and sync-state notification plumbing from validation to UI
- Exposed block/header tip snapshots via `appInitMain` for sync-aware UI initialization
- Added a sync progress bar to modal overlay with separate header/block progress tracking

### Changed
- Qt Receive dialog now shows the `bech32m` address option only when wallet Taproot capability is available
- Header sync now rate-limits duplicate `getheaders` requests
- Made `update_version.sh` deterministic and auto-detect current version before bumping
- Updated version metadata and man pages to `2.0.2`

### Fixed
- Aligned Taproot sighash epoch and annex hashing semantics with Bitcoin/BIP341 behavior
- Aligned Taproot witness standardness policy with Bitcoin
- Added tapscript-specific empty pubkey error handling to improve validation diagnostics
- `getblocktemplate` on regtest now skips peer/IBD checks for local mining and test workflows
- Fixed Qt modal overlay regressions (text overlap and mixed header/block progress behavior)
- Restored `InitExecutor` compatibility with older Qt `invokeMethod` APIs

### Removed
- Removed dark mode support from `palladium-qt`
- Removed whitepaper assets from this repository (moved to a dedicated repository)

### Tests
- Extended Taproot functional coverage with valid script-path scenarios
- Realigned functional tests with Palladium consensus behavior and stabilized Taproot validation flows
- Added script/taproot test coverage for tapscript-specific empty pubkey handling

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
- Updated man pages for all binaries to version 2.0.0

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

[Unreleased]: https://github.com/palladium-coin/palladiumcore/compare/v2.0.2...HEAD
[2.0.2]: https://github.com/palladium-coin/palladiumcore/releases/tag/v2.0.2
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

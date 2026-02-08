2.0.0 Release Notes
====================

Palladium Core version 2.0.0 is now available from:

  <https://github.com/palladium-coin/palladiumcore/releases/tag/v2.0.0>

This is a major release introducing Taproot support (BIP340/341/342/350), representing
the most significant protocol upgrade since Segregated Witness.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/palladium-coin/palladiumcore/issues>

To receive security and update notifications, please visit:

  <https://palladiumblockchain.net/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Palladium-Qt` (on Mac)
or `palladiumd`/`palladium-qt` (on Linux).

Upgrading directly from a version of Palladium Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Palladium Core are generally supported.

**Important**: First startup after upgrading may take several minutes as the wallet
rescans for Taproot-related transactions (if applicable).

Compatibility
==============

Palladium Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer. Palladium
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Palladium Core on
unsupported systems.

From Palladium Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Palladium Core does not yet change appearance
when macOS "dark mode" is activated.

Notable Changes
===============

Taproot Support (BIP340/341/342/350)
------------------------------------

Palladium Core 2.0.0 introduces full Taproot support, implementing Bitcoin Improvement
Proposals 340, 341, 342, and 350. This represents the most significant protocol upgrade
since Segregated Witness and brings substantial improvements to privacy, efficiency, and
smart contract capabilities.

### What is Taproot?

Taproot improves privacy, efficiency, and smart contract flexibility on the Palladium
blockchain through:

- **Schnorr Signatures (BIP340)**: More efficient signature scheme replacing ECDSA for
  witness v1 outputs. Schnorr signatures are 64 bytes (vs 71-72 for ECDSA) and enable
  future signature aggregation improvements.

- **Taproot Spending Rules (BIP341)**: New output type (witness version 1) enabling more
  private and flexible scripts. Taproot outputs can be spent via key-path (single signature,
  most efficient) or script-path (reveal only the executed script from a Merkle tree).

- **Tapscript (BIP342)**: Updated script validation rules with new opcodes including
  OP_CHECKSIGADD for efficient threshold signatures. Tapscript introduces validation weight
  limits and reserves OP_SUCCESS opcodes for future soft-fork upgrades.

- **Bech32m Addresses (BIP350)**: New address format for Taproot (starting with `plm1p...`).
  Bech32m uses a different checksum constant (0x2bc830a3) specifically designed for witness
  version 1 and higher addresses.

### Deployment Timeline

Taproot will activate via BIP9 miner signaling:

- **Mainnet**: BIP9 soft fork activation using bit 2
  - Signaling starts: March 1, 2026 00:00:00 UTC (block time 1772323200)
  - Signaling timeout: March 1, 2027 00:00:00 UTC (block time 1803859200)
  - Activation threshold: 1815 of 2016 blocks (90%) must signal readiness
  - Status: Once locked in, Taproot activates in the next period

- **Testnet**: Same parameters as mainnet for realistic testing

- **Regtest**: Always active for immediate developer testing

**Checking activation status**:
```bash
palladium-cli getblockchaininfo
```

Look for the "softforks" section and find the "taproot" deployment. Possible states:
- `defined`: Deployment defined but signaling hasn't started
- `started`: Signaling period active, miners can signal readiness
- `locked_in`: Activation threshold reached, grace period before activation
- `active`: Taproot consensus rules enforced
- `failed`: Timeout reached without sufficient signaling

### Wallet Changes

New "bech32m" address type is now available for Taproot outputs:

**GUI (palladium-qt)**:
- Receive tab now has a dropdown to select address type:
  - "Legacy" - Traditional P2PKH addresses (1...)
  - "P2SH-SegWit" - SegWit wrapped in P2SH (3...)
  - "Bech32" - Native SegWit v0 addresses (plm1q...)
  - "Bech32m" - Taproot addresses (plm1p...) ← NEW

- Default address type remains "bech32" for backward compatibility
- Users can switch to "bech32m" for enhanced privacy and efficiency

**RPC Interface**:
- `getnewaddress "label" "bech32m"` generates Taproot addresses
- `getaddressinfo` reports witness version 1 for Taproot addresses
- All existing RPCs work seamlessly with Taproot addresses

**Example**:
```bash
# Generate a Taproot address
$ palladium-cli getnewaddress "savings" "bech32m"
plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr

# Get information about the address
$ palladium-cli getaddressinfo "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr"
{
  "address": "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr",
  "iswitness": true,
  "witness_version": 1,
  "witness_program": "...",
  ...
}

# Send to a Taproot address (works like any other address)
$ palladium-cli sendtoaddress "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr" 1.5
```

**Configuration**:
To make bech32m the default address type, add to `palladium.conf`:
```conf
addresstype=bech32m
changetype=bech32m
```

Both receiving to and spending from Taproot addresses are fully supported once the
soft fork activates on mainnet.

### Transaction Behavior

**Sending**:
- Taproot UTXOs are automatically included in coin selection
- Spending from Taproot uses key-path by default (most efficient)
- Schnorr signatures are generated automatically for Taproot inputs
- Slightly lower transaction fees due to smaller signature size

**Receiving**:
- Any wallet can send to your Taproot address (if they support bech32m)
- Taproot outputs are tracked and displayed in transaction history
- Balance calculations include Taproot UTXOs

**Privacy**:
- Key-path spends look identical to single-signature payments
- Complex scripts remain hidden unless script-path is used
- No way to distinguish multisig from single-sig on-chain (key-path)

### Technical Implementation

This release backports Taproot implementation from Bitcoin Core v24.2 with appropriate
adaptations for Palladium's chain parameters and consensus rules.

**Key components**:

1. **Cryptographic Layer**:
   - secp256k1 library upgraded to Bitcoin Core v24.2
   - Enabled `extrakeys` module for x-only public keys
   - Enabled `schnorrsig` module for BIP340 Schnorr signatures
   - XOnlyPubKey class for 32-byte public key representation

2. **Consensus Layer**:
   - Full taproot/tapscript validation in script interpreter (469 lines added)
   - Taproot signature hash computation with tagged SHA256
   - Control block parsing and Merkle proof verification
   - Validation weight tracking for tapscript execution

3. **Encoding Layer**:
   - Bech32m encoding/decoding with dual-constant checksum
   - Automatic format selection based on witness version
   - Palladium-specific HRP: "plm" for mainnet

4. **Wallet Layer**:
   - BECH32M output type for address generation
   - Schnorr signature generation for Taproot inputs
   - Internal key tracking for Taproot outputs
   - Output descriptors extended for Taproot

5. **Policy Layer**:
   - Script verification flags: SCRIPT_VERIFY_TAPROOT and related
   - Standardness limits for tapscript (stack size, script size, validation weight)
   - Control block size validation (33-4129 bytes)

**Testing**:
- Functional test: `test/functional/feature_taproot.py` (end-to-end P2TR lifecycle)
- Unit tests: Schnorr signature generation/verification in `test/key_tests.cpp`
- Bech32m encoding tests with Palladium-specific test vectors
- Integration tests for RPC interface and wallet operations

For detailed technical information, see [TAPROOT.md](../TAPROOT.md).

For user-friendly guide, see [doc/taproot-guide.md](../taproot-guide.md).

### Upgrading to Taproot

**Do I need to upgrade my addresses?**
No. Using Taproot is completely optional. All existing address types (legacy, P2SH, bech32)
continue to work indefinitely.

**When should I use Taproot?**
- For enhanced privacy (key-path spending hides script complexity)
- For slightly lower fees (Schnorr signatures are smaller)
- For future-proof wallet infrastructure

**Compatibility concerns?**
- Ensure recipient wallets support bech32m before sending to Taproot addresses
- Most modern wallets will add support after mainnet activation
- Provide a bech32 fallback address for maximum compatibility

**Migration path**:
1. Upgrade to Palladium Core 2.0.0
2. Wait for Taproot activation on mainnet (March 2026 expected)
3. Start generating bech32m addresses for new payments
4. Existing funds in old addresses can be spent to Taproot addresses over time

Build System Changes
--------------------

### secp256k1 Library Update

The secp256k1 library has been upgraded to Bitcoin Core v24.2 tag, enabling:
- BIP340 Schnorr signature support
- X-only public key operations
- Optimized signature verification
- Precomputed tables for ecmult operations

This upgrade is essential for Taproot functionality and brings performance improvements
to all signature operations.

GUI Improvements
----------------

### Address Type Selection

The Receive tab now features a dropdown menu for address type selection:
- Visual indication of address type (Legacy / P2SH-SegWit / Bech32 / Bech32m)
- Tooltip explanations for each address type
- Remembers last selected type for user convenience

This makes it easy for users to generate Taproot addresses without using the command line.

RPC Changes
-----------

### New Features

- `getnewaddress` now accepts "bech32m" as address type parameter
- `getaddressinfo` correctly identifies Taproot addresses with `witness_version: 1`
- `createmultisig` supports "bech32m" type (with limitations)

### Modified Behavior

- `getblockchaininfo` includes "taproot" deployment status in "softforks" section
- `validateaddress` validates bech32m address format
- `decodescript` recognizes witness v1 outputs

### Deprecated Features

None in this release.

For full RPC documentation, run `palladium-cli help` or see [JSON-RPC-interface.md](JSON-RPC-interface.md).

Testing Improvements
--------------------

### Functional Tests

New functional test `feature_taproot.py` validates:
- Taproot address generation
- Sending to Taproot addresses
- Spending from Taproot addresses (key-path)
- Transaction confirmation and UTXO tracking

### Test Framework Updates

- Python test framework updated with bech32m encoding support
- Segwit_addr module aligned to Palladium chain parameters
- Test vectors adapted from Bitcoin to Palladium HRP and consensus rules

Documentation
-------------

### New Documentation

- `doc/TAPROOT.md`: Comprehensive technical specification
- `doc/taproot-guide.md`: User-friendly guide for wallet users
- `doc/release-notes/release-notes-2.0.0.md`: Detailed release notes for v2.0.0
- `CHANGELOG.md`: Changelog with history of all releases

### Updated Documentation

- `doc/bips.md`: Updated with BIP340/341/342/350 implementation details
- `README.md`: References to Taproot support

Known Issues
============

### Taproot Before Activation

- Taproot addresses can be generated before mainnet activation
- Funds sent to Taproot addresses before activation will be unspendable until activation
- **Recommendation**: Wait for activation on mainnet before using Taproot addresses for real funds

### Wallet Compatibility

- Very old wallet software may not recognize bech32m addresses
- Some exchanges may not support deposits to bech32m addresses initially
- **Recommendation**: Verify recipient support before sending to Taproot addresses

### Script-Path Spending

- Advanced script-path spending features require manual descriptor/PSBT construction
- GUI does not yet support creating complex taproot scripts
- **Workaround**: Use RPC/CLI for advanced Taproot features or wait for future GUI improvements

2.0.0 Change Log
================

### Consensus

- Add BIP340 Schnorr signature validation
- Add BIP341 Taproot spending rules
- Add BIP342 Tapscript validation
- Implement BIP9 deployment for Taproot (bit 2)
- Add script verification flags for Taproot

### Crypto

- Upgrade secp256k1 to Bitcoin Core v24.2
- Add XOnlyPubKey class for 32-byte public keys
- Add CKey::SignSchnorr() for BIP340 signatures
- Add CKey::SignSchnorrTaproot() for BIP341 key-path spending
- Add XOnlyPubKey::VerifySchnorr() for signature verification

### Encoding

- Implement BIP350 bech32m format
- Add dual-mode bech32/bech32m encoding
- Update key_io for automatic address format selection

### Script

- Add SigVersion::TAPROOT and SigVersion::TAPSCRIPT
- Implement Taproot signature hash computation
- Add control block validation
- Add validation weight tracking for tapscript
- Add OP_CHECKSIGADD support
- Add new script error codes for Taproot

### Wallet

- Add OutputType::BECH32M for Taproot addresses
- Add bech32m address generation in GUI and RPC
- Add Schnorr signature creation for Taproot spending
- Add internal key tracking for Taproot outputs
- Update output descriptors for Taproot

### RPC

- Add "bech32m" address type to getnewaddress
- Add Taproot deployment status to getblockchaininfo
- Update getaddressinfo for witness v1 addresses
- Extend createmultisig with bech32m support

### GUI

- Add address type dropdown to Receive tab
- Add "bech32m" option for Taproot addresses
- Update tooltips and UI labels for clarity

### Tests

- Add feature_taproot.py functional test
- Add Schnorr signature unit tests
- Add bech32m encoding tests
- Adapt test framework to Palladium parameters

### Build

- Update secp256k1 to Bitcoin Core v24.2
- Enable extrakeys and schnorrsig modules
- Update build scripts for new dependencies

### Documentation

- Add doc/TAPROOT.md technical specification
- Add doc/taproot-guide.md user guide
- Update doc/bips.md with BIP340/341/342/350
- Add doc/release-notes/release-notes-2.0.0.md (this file)
- Add CHANGELOG.md with complete release history

Credits
=======

Thanks to the Bitcoin Core developers for the original Taproot implementation in
Bitcoin Core v24.2, which formed the basis for this backport to Palladium Core.

Special thanks to everyone who directly contributed to this release:

- davide3011 (Taproot backport and implementation)

As well as to the broader Bitcoin development community whose work on Taproot
made this upgrade possible.

And to everyone that helped with translations, testing, and community support.

---

For user guide, see [doc/taproot-guide.md](../taproot-guide.md).
For technical details, see [TAPROOT.md](../TAPROOT.md).
For BIP specifications, see [doc/bips.md](../bips.md).

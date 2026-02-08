# Taproot Implementation in Palladium Core

## Introduction

Taproot is a major protocol upgrade to Palladium Core that enhances privacy, improves efficiency, and enables more flexible smart contract development. Introduced in version 2.0.0, this implementation brings the benefits of Bitcoin's Taproot upgrade to the Palladium blockchain through a complete backport from Bitcoin Core v24.2.

Taproot combines three key innovations:
- **Schnorr signatures** (BIP340) for more efficient and private transactions
- **Taproot outputs** (BIP341) enabling key-path and script-path spending
- **Tapscript** (BIP342) with updated validation rules and new opcodes
- **Bech32m addresses** (BIP350) for encoding Taproot outputs

## BIP Implementation Reference

### BIP340: Schnorr Signatures for secp256k1

**Status**: Fully implemented in v2.0.0
**Specification**: https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki

BIP340 defines the Schnorr signature scheme for secp256k1, replacing ECDSA for witness version 1 outputs. Key features:

- **64-byte signatures**: More compact than DER-encoded ECDSA (71-72 bytes)
- **X-only public keys**: 32-byte representation (only x-coordinate)
- **Linearity**: Enables key and signature aggregation (foundation for future multisig improvements)
- **Provable security**: Based on discrete logarithm assumption
- **Batch verification**: Multiple signatures can be verified more efficiently together

**Implementation components**:
- `XOnlyPubKey` class: 32-byte x-only public key representation
- `CKey::SignSchnorr()`: BIP340-compliant signature generation
- `XOnlyPubKey::VerifySchnorr()`: Signature verification
- `CPubKey::GetXOnlyPubKey()`: Extraction of x-only key from compressed pubkey
- Integration with libsecp256k1's `extrakeys` and `schnorrsig` modules

### BIP341: Taproot: SegWit version 1 spending rules

**Status**: Fully implemented in v2.0.0
**Specification**: https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki

BIP341 introduces Taproot outputs (witness version 1), enabling two spending paths:

1. **Key-path spending**: Direct signature verification (default, most efficient)
2. **Script-path spending**: Reveal and execute a script from a Merkle tree

**Key innovations**:
- **Unified spending**: Key-path and script-path spends are indistinguishable until spent
- **MAST (Merkelized Alternative Script Trees)**: Only reveal executed script branch
- **TapTweak**: Output key is internal key tweaked with script tree commitment
- **Parity bit**: Even/odd y-coordinate handled implicitly

**Implementation components**:
- `SigVersion::TAPROOT`: New signature version for key-path spending
- `CKey::SignSchnorrTaproot()`: Key-path signature with BIP341 tweaking
- `XOnlyPubKey::ComputeTapTweak()`: Taproot tweak calculation
- `XOnlyPubKey::CreatePayToTaprootPubKey()`: Output key derivation with parity handling
- Control block validation: 33-4129 bytes, 32-byte node increments
- Taproot signature hash with SHA256 tagged hashing

### BIP342: Validation of Taproot Scripts

**Status**: Fully implemented in v2.0.0
**Specification**: https://github.com/bitcoin/bips/blob/master/bip-0342.mediawiki

BIP342 defines validation rules for scripts revealed via script-path spending:

**New features**:
- **OP_CHECKSIGADD**: Replaces OP_CHECKMULTISIG with better semantics
- **Validation weight limits**: Prevent DoS attacks in script validation
- **OP_SUCCESS opcodes**: Reserved for future soft-fork upgrades (80, 98, 126-129, 131-134, 137-138, 141-142, 149-153)
- **Signature opcodes**: Use Schnorr instead of ECDSA
- **Taproot leaf version**: 0xc0 (extensible to 0xc1-0xfe)

**Implementation components**:
- `SigVersion::TAPSCRIPT`: New signature version for script-path
- `ScriptExecutionData`: Tracks tapleaf hash, codeseparator position, annex, validation weight
- Validation weight constraints:
  - `VALIDATION_WEIGHT_PER_SIGOP_PASSED` = 50 units
  - `VALIDATION_WEIGHT_OFFSET` = 50 units
- Annex support with tag 0x50
- Minimal-if requirement for tapscript conditionals
- CHECKMULTISIG disabled in tapscript (use OP_CHECKSIGADD instead)

### BIP350: Bech32m format for v1+ witness addresses

**Status**: Fully implemented in v2.0.0
**Specification**: https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki

BIP350 introduces Bech32m, a modified version of Bech32 for witness version 1+ addresses:

**Key differences from Bech32**:
- **Different constant**: Polymod check uses 0x2bc830a3 (vs 1 for Bech32)
- **Witness version**: v0 uses Bech32, v1+ uses Bech32m
- **Length mutation protection**: Better resistance to certain error patterns

**Address format**:
- Taproot addresses: `plm1p...` (witness v1)
- SegWit v0 addresses: `plm1q...` (witness v0, uses Bech32)

**Implementation components**:
- `bech32::Encoding` enum: `BECH32` and `BECH32M`
- Dual-constant checksum creation/verification
- Automatic encoding selection based on witness version
- Palladium-specific HRP (Human Readable Part): "plm"

## Source Attribution

This implementation is a backport of Taproot functionality from **Bitcoin Core v24.2** with appropriate adaptations for Palladium's chain parameters, consensus rules, and testing infrastructure.

**Bitcoin Core components**:
- secp256k1 library upgraded to Bitcoin Core v24.2 tag
- Schnorr signature implementation from Bitcoin Core
- Taproot consensus rules from Bitcoin Core
- Bech32m encoding logic from Bitcoin Core

**Palladium-specific adaptations**:
- Chain parameters (deployment timing, genesis blocks, maturity rules)
- Address prefixes (HRP: "plm" for mainnet)
- Test vector alignment to Palladium's consensus
- Wallet infrastructure integration
- Deployment schedule for Palladium mainnet/testnet

## Architecture Overview

### Crypto Layer

**Files**: `src/key.cpp`, `src/key.h`, `src/pubkey.cpp`, `src/pubkey.h`

**Classes and methods**:
- `XOnlyPubKey`: 32-byte x-only public key
  - `VerifySchnorr(msg, sig)`: BIP340 signature verification
  - `ComputeTapTweak(merkle_root)`: Calculate BIP341 taproot tweak
  - `CreatePayToTaprootPubKey(merkle_root, &output_key)`: Derive output key
  - `CheckTapTweak(internal_key, merkle_root, parity)`: Verify taproot tweak

- `CKey`: Private key operations
  - `SignSchnorr(msg, &sig)`: Basic Schnorr signature (BIP340)
  - `SignSchnorrTaproot(msg, merkle_root, &sig)`: Taproot key-path signature (BIP341)

- `CPubKey`: Compressed public key (33 bytes)
  - `GetXOnlyPubKey()`: Extract x-only representation

**Statistics**:
- `key.cpp`: +26 lines
- `key.h`: +11 lines
- `pubkey.cpp`: +49 lines
- `pubkey.h`: +66 lines
- `test/key_tests.cpp`: +60 lines (Schnorr test coverage)

### Encoding Layer

**Files**: `src/bech32.cpp`, `src/bech32.h`, `src/key_io.cpp`

**Key changes**:
- `bech32::Encoding` enum for dual-mode operation
- `Encode(encoding, hrp, values)`: Supports both BECH32 and BECH32M
- `Decode(str)`: Auto-detects encoding based on witness version
- Address encoding/decoding logic updated for automatic selection

**Statistics**:
- `bech32.cpp`: +67/-48 lines
- `bech32.h`: +15/-1 lines
- `key_io.cpp`: +22/-9 lines

### Script Layer

**Files**: `src/script/interpreter.cpp`, `src/script/interpreter.h`, `src/script/script.h`, `src/script/script_error.cpp`, `src/script/script_error.h`, `src/script/standard.cpp`, `src/script/standard.h`

**Major additions**:
- `SigVersion::TAPROOT`: Key-path spending
- `SigVersion::TAPSCRIPT`: Script-path spending
- `EvalScript()`: Taproot/tapscript execution paths
- `BaseSignatureChecker::CheckSchnorrSignature()`: Schnorr verification hook
- Taproot signature hash computation with precomputed data
- Control block parsing and validation
- OP_CHECKSIGADD implementation
- Validation weight tracking

**New error codes**:
- `SCRIPT_ERR_SCHNORR_SIG_SIZE`
- `SCRIPT_ERR_SCHNORR_SIG_HASHTYPE`
- `SCRIPT_ERR_SCHNORR_SIG`
- `SCRIPT_ERR_TAPROOT_WRONG_CONTROL_SIZE`
- `SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT`
- `SCRIPT_ERR_TAPSCRIPT_CHECKMULTISIG`
- `SCRIPT_ERR_TAPSCRIPT_MINIMALIF`

**Statistics**:
- `interpreter.cpp`: +469/-36 lines
- `interpreter.h`: +69/-1 lines
- `script.h`: +22/-1 lines
- `script_error.cpp`: +20 lines
- `script_error.h`: +12 lines
- `standard.cpp`: +12 lines
- `standard.h`: +1 line

### Consensus Layer

**Files**: `src/chainparams.cpp`, `src/consensus/params.h`, `src/versionbitsinfo.cpp`

**Deployment parameters**:

**Mainnet**:
- Deployment: BIP9 soft fork
- Bit: 2
- Start time: 1771027200 (February 14, 2026 00:00:00 UTC)
- Timeout: 1803859200 (March 1, 2027 00:00:00 UTC)
- Min activation height: Not specified (activation determined by signaling threshold)

**Testnet**:
- Same parameters as mainnet

**Regtest**:
- Status: ALWAYS_ACTIVE (immediate activation for testing)

**Statistics**:
- `chainparams.cpp`: +9 lines
- `consensus/params.h`: +3/-1 lines
- `versionbitsinfo.cpp`: +4 lines

### Wallet Layer

**Files**: `src/outputtype.cpp`, `src/outputtype.h`, `src/script/descriptor.cpp`, `src/script/sign.cpp`, `src/script/signingprovider.cpp`, `src/wallet/scriptpubkeyman.cpp`, `src/wallet/rpcwallet.cpp`

**Key additions**:
- `OutputType::BECH32M`: New output type for Taproot
- `GetDestinationForTaprootKey()`: Create witness v1 destination
- `CreateSig()`: Updated for Schnorr signature generation
  - Key-path: Uses `SignSchnorrTaproot()` with empty scriptCode
  - Script-path: Uses `SignSchnorr()` for tapscript validation
- Taproot internal key storage: `m_taproot_internal_keys` map
- `GetTaprootInternalKey()`: Lookup internal key from output key

**RPC changes**:
- `getnewaddress` supports "bech32m" address type
- `createmultisig` extended for bech32m (with taproot limitations)
- `getaddressinfo` reports address type correctly for taproot

**Statistics**:
- `outputtype.cpp`: +35/-1 lines
- `outputtype.h`: +3/-1 lines
- `descriptor.cpp`: +72/-2 lines
- `sign.cpp`: +90/-27 lines
- `signingprovider.cpp`: +17 lines
- `scriptpubkeyman.cpp`: +36 lines
- `rpcwallet.cpp`: +12/-3 lines

### Policy Layer

**Files**: `src/policy/policy.cpp`, `src/policy/policy.h`, `src/validation.cpp`

**Script verification flags**:
- `SCRIPT_VERIFY_TAPROOT` (bit 17): Enable taproot/tapscript consensus
- `SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION` (bit 18): Reject unknown taproot versions
- `SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS` (bit 19): Reject OP_SUCCESS opcodes
- `SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE` (bit 20): Reject unknown pubkey types

**Standardness limits**:
- `MAX_STANDARD_TAPSCRIPT_STACK_ITEMS` = 100
- `MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE` = 80 bytes
- `MAX_STANDARD_TAPSCRIPT_SCRIPT_SIZE` = 3600 bytes
- `TAPROOT_CONTROL_BASE_SIZE` = 33 bytes (control block minimum)
- `TAPROOT_CONTROL_NODE_SIZE` = 32 bytes (each merkle node)
- `TAPROOT_CONTROL_MAX_SIZE` = 4129 bytes (max control block)

**Statistics**:
- `policy/policy.cpp`: +46/-1 lines
- `policy/policy.h`: +12/-1 lines
- `validation.cpp`: +10 lines

## Consensus Rules

### Deployment Activation

Taproot activates via BIP9 signaling:
1. Miners signal readiness by setting bit 2 in block version
2. When 1815 of 2016 blocks in a retarget period signal (90% threshold), deployment is LOCKED_IN
3. After LOCKED_IN period ends, deployment becomes ACTIVE for all subsequent blocks
4. Timeout on March 1, 2027 if insufficient signaling

### Script Verification Flags

**Standard transaction validation** (mempool/relay):
```
SCRIPT_VERIFY_TAPROOT |
SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION |
SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS |
SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE
```

**Consensus validation** (blocks):
```
SCRIPT_VERIFY_TAPROOT
```

### Taproot Spending Rules

**Key-path spending** (witness v1 with single witness element):
1. Witness must contain exactly 1 element (signature, or 65 bytes after annex removal)
2. Signature must be 64 or 65 bytes (65 = 64-byte sig + SIGHASH byte)
3. Extract internal key from output key using BIP341 taproot tweak
4. Verify Schnorr signature against tweaked pubkey
5. Empty scriptCode per BIP341

**Script-path spending** (witness v1 with multiple elements):
1. Last witness element is control block (33-4129 bytes, 33 + 32n)
2. Second-to-last element is tapscript (max 3600 bytes for standard)
3. Control block format: `[leaf_version || internal_key_parity || internal_key[1:] || merkle_proof]`
4. Verify control block reconstructs output key
5. Execute tapscript with TAPSCRIPT signature version
6. Track validation weight (max 1650 units)

**Annex handling**:
- If first witness stack element starts with 0x50, it's the annex
- Annex is removed before script validation
- Annex hash is committed in signature hash

### Validation Weight

Tapscript execution tracks validation weight to prevent DoS:
- Each signature operation: 50 weight units (if signature validates)
- Initial offset: 50 weight units
- Maximum: 1650 units per script-path spend
- Formula: `weight = 50 + 50 * num_successful_checksigs`
- Limit: `num_successful_checksigs <= 32` (1650 / 50 - 1)

### Standardness Policy

**Tapscript stack limits**:
- Max stack items: 100
- Max stack item size: 80 bytes
- Max script size: 3600 bytes

**Control block validation**:
- Minimum size: 33 bytes (version + internal key)
- Maximum size: 4129 bytes (33 + 32 * 128 merkle nodes)
- Size must be 33 + 32n for integer n

**Leaf version**:
- Standard: 0xc0 (tapscript)
- Non-standard: 0xc1-0xfe (reserved for future soft forks)
- Invalid: 0x00-0xbf, 0xff

## Key Files Modified

### Summary by Layer

| Layer | Files Modified | Lines Added | Lines Removed |
|-------|----------------|-------------|---------------|
| Crypto | 5 | 212 | 6 |
| Encoding | 3 | 104 | 58 |
| Script | 7 | 604 | 38 |
| Consensus | 3 | 16 | 1 |
| Wallet | 9 | 265 | 37 |
| Policy | 3 | 68 | 2 |
| Testing | Multiple | Extensive | - |
| **Total** | **30+** | **~1269** | **~142** |

### Critical Implementation Files

**Cryptographic primitives**:
- `src/key.cpp`, `src/key.h`: Schnorr signing with BIP340/BIP341
- `src/pubkey.cpp`, `src/pubkey.h`: XOnlyPubKey, signature verification, taproot tweaking
- `src/secp256k1/`: Upgraded to Bitcoin Core v24.2 with extrakeys and schnorrsig modules

**Consensus rules**:
- `src/script/interpreter.cpp`: Taproot/tapscript validation logic (469 lines added)
- `src/script/script_error.h`: New error codes for taproot
- `src/chainparams.cpp`: BIP9 deployment parameters

**Address encoding**:
- `src/bech32.cpp`: Bech32m implementation
- `src/key_io.cpp`: Address encoding/decoding with automatic format selection

**Wallet integration**:
- `src/outputtype.cpp`: BECH32M output type
- `src/script/sign.cpp`: Schnorr signature generation for transactions
- `src/wallet/scriptpubkeyman.cpp`: Internal key tracking for taproot

**Testing**:
- `test/functional/feature_taproot.py`: End-to-end taproot functional test
- `src/test/key_tests.cpp`: Schnorr signature unit tests
- `src/test/data/`: Bech32m test vectors aligned to Palladium

## Testing

### Functional Tests

**Feature test**: `test/functional/feature_taproot.py`

Test coverage:
1. Wallet creation with default bech32m support
2. Taproot address generation: `getnewaddress("", "bech32m")`
3. Funding test: Send to P2TR address
4. Balance verification
5. Spending test: Send from P2TR address
6. Transaction confirmation
7. End-to-end P2TR lifecycle validation

**Test framework**:
- Python segwit_addr module updated for Bech32m
- Address validation for Palladium HRP ("plm")
- Consensus rule alignment to Palladium parameters

### Unit Tests

**Schnorr signatures**: `src/test/key_tests.cpp`

Test coverage:
- XOnlyPubKey construction and validation
- Schnorr signature round-trip (sign + verify)
- Signature size validation (64 bytes)
- XOnlyPubKey invariants (negation property)
- ECDSA interoperability (same key, both signatures valid)
- BIP340 test vectors

**Bech32m encoding**: `src/test/bech32_tests.cpp`

Test coverage:
- Bech32m encoding/decoding
- Checksum validation
- Witness version detection
- Error detection (insertions, deletions, substitutions)
- Palladium-specific test vectors

### Integration Tests

**RPC interface**:
- `getnewaddress` with "bech32m" type
- `getaddressinfo` for taproot addresses
- `sendtoaddress` to/from taproot outputs
- `createmultisig` taproot limitations
- `getblockchaininfo` taproot deployment status

**Wallet operations**:
- Create taproot receive address
- Send to taproot address
- Spend from taproot address (key-path)
- Fee estimation for taproot transactions
- UTXO selection with taproot inputs

## References

### BIP Specifications

- **BIP340**: Schnorr Signatures for secp256k1
  https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki

- **BIP341**: Taproot: SegWit version 1 spending rules
  https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki

- **BIP342**: Validation of Taproot Scripts
  https://github.com/bitcoin/bips/blob/master/bip-0342.mediawiki

- **BIP350**: Bech32m format for v1+ witness addresses
  https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki

### Additional Resources

- **Bitcoin Core v24.2 release**: Source for Taproot backport
  https://github.com/bitcoin/bitcoin/releases/tag/v24.2

- **secp256k1 library**: Cryptographic primitives
  https://github.com/bitcoin-core/secp256k1

- **Taproot Workshop**: Technical deep-dive (Bitcoin-focused but applicable)
  https://github.com/bitcoinops/taproot-workshop

### Palladium-Specific Documentation

- **User guide**: `doc/taproot-guide.md`
- **Release notes**: `doc/release-notes.md` (v2.0.0)
- **BIP implementation list**: `doc/bips.md`

---

**Version**: Palladium Core 2.0.0
**Last Updated**: February 2026
**Status**: Production-ready, mainnet deployment March 2026

# Tests in Palladium Core

How to run the tests is in [test/README.md](../test/README.md) and [src/test/README.md](../src/test/README.md).
This page lists what the tests check for the parts that differ from Bitcoin Core
(see also [palladium-consensus.md](palladium-consensus.md)).

## Unit tests (`make check`, `src/test/test_palladium`)

| Area | Suite | Covers |
|------|-------|--------|
| Chain parameters | `chainparams_tests` | Ports (P2P and RPC), bech32 HRP (`plm`, `tplm`, `rplm`), message start, fork and Taproot heights, legacy mainnet values (`nPowTargetSpacing` 10 min, `nPowTargetTimespan` 14 days), `-taprootheight` on regtest. |
| Difficulty | `pow_tests` | Legacy retarget; difficulty reset window 28930-28999; legacy interval; LWMA (stable on target, slow and fast blocks, `6*T` solvetime clamp, non-monotonic timestamps, pow limit); LWMA before the fork; retarget every 60 blocks after the Resilience fork and its factor 4 limits; testnet 20 minute rule; fixed regtest difficulty. `CheckProofOfWork` and `GetBlockProofEquivalentTime` are the Bitcoin Core tests. |
| Taproot | `taproot_tests` | TapTweak and BIP86 vectors, SIGHASH_DEFAULT and the other hash types, key path and script path spends, annex, control block errors, leaf versions, unknown pubkey types, several inputs. |
| Taproot, shared | `script_tests`, `key_tests`, `script_p2sh_tests` | Schnorr signatures and the `taproot_active` flag of `AreInputsStandard`. |

## Functional tests (`test/functional`)

| Test | Covers |
|------|--------|
| `feature_taproot.py` | Wallet: bech32m addresses, funding and spending, 64 byte signatures, invalid control block, several inputs, restart, script path, imported keys, `dumpprivkey`, `importpubkey`, `importaddress`, `importmulti`, address decoding. |
| `feature_taproot_consensus.py` | Bitcoin Core's Taproot consensus test, with one node where Taproot is not active (`-taprootheight=-1`) and one where it is. |
| `feature_taproot_activation.py` | The activation height: the spend of a Taproot output is not relayed before it, is accepted when the next block is the activation block, and is mined there; `getblockchaininfo` reports the same height. |
| `feature_opreturn_prefork.py` | OP_RETURN outputs over 83 bytes are rejected while the next block is before the Resilience fork (`bad-txns-oversize-opreturn-prefork`), and accepted up to `-datacarriersize` from the fork block on. |
| `feature_block.py` | Block validation over p2p with Palladium rules: block version, BIP34 coinbase height, non-final transactions, and the other scenarios in `_run_palladium_block_scenarios`. |
| `rpc_blockchain.py` | `getblockchaininfo` reports `taproot` as a buried softfork. |
| `p2p_getheaders_deferred.py` | A `getheaders` suppressed while another is in flight is sent when the first is answered. |
| `wallet_multiwallet.py` | `-walletdir` with and without a trailing separator. |

Regtest differs from Bitcoin Core: coinbase maturity is 120 blocks, the Resilience fork is at height 200
and Taproot is active from genesis (change it with `-taprootheight`).

## Fuzz tests

See [fuzzing.md](fuzzing.md).

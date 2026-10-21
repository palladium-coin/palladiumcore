# Palladium consensus and policy differences

Where Palladium Core departs from Bitcoin Core. Values are from `src/chainparams.cpp`, `src/pow.cpp` and `src/validation.cpp`; changes to the consensus parts need a coordinated network upgrade.

## Activation heights

| Rule | Mainnet | Testnet | Regtest |
|------|---------|---------|---------|
| BIP34/65/66, CSV, Segwit | 29000 | 0 (BIP34: 1700) | 0 |
| Resilience fork (`nResilienceForkHeight`) | 340000 | 2000 | 200 |
| Taproot (`TaprootHeight`, see [TAPROOT.md](TAPROOT.md)) | 405000 | 2500 | 0 |

## Block spacing

- `nPowTargetSpacing`: 10 minutes on mainnet, kept for validating the pre-fork chain (it also defines `DifficultyAdjustmentInterval()` and some network timeouts); 2 minutes on testnet and regtest.
- `nPowTargetSpacingV2`: 2 minutes on every network. Used by the LWMA and by the time-based heuristics that reflect today's block rate.

## Difficulty (`GetNextWorkRequired`)

Checked in this order:

1. Heights 28930-28999: proof-of-work limit (difficulty reset for the new algorithm and for Segwit/CSV activation).
2. Regtest (`fPowNoRetargeting`): the difficulty never changes.
3. Testnet (`fPowAllowMinDifficultyBlocks`): a block more than 20 minutes after its parent may use the minimum difficulty; otherwise LWMA from genesis.
4. From `nResilienceForkHeight`: retarget every 60 blocks with a 7200 s target timespan (60 blocks x 120 s), each step limited to a factor of 4 in either direction.
5. Heights 29000 to the fork: LWMA (`LwmaCalculateNextWorkRequired`, window of 240 blocks, solve times capped at 6 x target spacing).
6. Below 29000: the legacy Bitcoin retarget every `DifficultyAdjustmentInterval()` blocks (14 days / 10 minutes).

### Known limit of the LWMA

`LwmaCalculateNextWorkRequired` adds up `target / (k * N)` for the blocks of the window with integer
division, where `k * N` is about 8.3 x 10^8 (N = 240, T = 120 s). If every target in the window is
below that value the sum is 0 and the next target is 0, which no hash can satisfy.

This cannot happen on a real chain: the proof-of-work limit is about 2^224, so it would take a
difficulty about 2^194 times the minimum. The code is therefore left as it is on purpose. Adding a
guard would change consensus and need a fork, for a case that cannot occur. The fuzz target `pow` keeps
its targets above `powLimit >> 100` for this reason. If the algorithm is ever changed (for example by
a new fork), the window sum should be computed with a division that rounds up or a minimum target.

## Policy

While the next block is below `nResilienceForkHeight`, transactions with an OP_RETURN output whose script is larger than 83 bytes are rejected by mempool acceptance (`bad-txns-oversize-opreturn-prefork`). This is a relay policy check, not a block validity rule.

## Network parameters

| | Mainnet | Testnet | Regtest |
|---|---------|---------|---------|
| P2P port | 2333 | 12333 | 28444 |
| RPC port | 2332 | 12332 | 28443 |
| Bech32 HRP | `plm` | `tplm` | `rplm` |

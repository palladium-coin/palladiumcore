# Taproot User Guide

## What is Taproot?

Taproot is a major upgrade to the Palladium protocol that makes transactions more private, efficient, and flexible. Introduced in Palladium Core 2.0.0, Taproot brings several important improvements:

### Key Benefits

**Enhanced Privacy**
- Complex smart contracts look identical to simple payments on the blockchain
- Only the spending conditions actually used are revealed
- Multi-signature setups can appear as single-signature transactions

**Improved Efficiency**
- Smaller transaction sizes using Schnorr signatures
- Lower fees for complex transactions
- Batch signature verification speeds up validation

**Greater Flexibility**
- More powerful smart contract capabilities
- Support for complex spending conditions without revealing them upfront
- Foundation for future protocol improvements

### How is Taproot Different from SegWit?

Taproot builds on Segregated Witness (SegWit) with these improvements:

| Feature | SegWit (v0) | Taproot (v1) |
|---------|-------------|--------------|
| **Address prefix** | plm1q... | plm1p... |
| **Signature type** | ECDSA | Schnorr |
| **Address format** | Bech32 | Bech32m |
| **Privacy** | Script visible | Script hidden (unless used) |
| **Multisig efficiency** | Linear cost | Constant cost (key-path) |

## Address Types in Palladium

Palladium supports four address types. You can use any of them, and they all work together:

### 1. Legacy (P2PKH)
- **Starts with**: `1...`
- **Example**: `1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa`
- **Features**: Original address format, widely compatible
- **Use case**: Maximum compatibility with older wallets

### 2. P2SH-SegWit
- **Starts with**: `3...`
- **Example**: `3J98t1WpEZ73CNmYviecrnyiWrnqRhWNLy`
- **Features**: SegWit wrapped in P2SH for compatibility
- **Use case**: SegWit benefits with broader compatibility

### 3. Native SegWit - Bech32 (witness v0)
- **Starts with**: `plm1q...`
- **Example**: `plm1qw508d6qejxtdg4y5r3zarvary0c5xw7kg3g4ty`
- **Features**: Lower fees, error detection, all lowercase
- **Use case**: Modern default for efficient transactions

### 4. Taproot - Bech32m (witness v1) ← NEW in v2.0.0
- **Starts with**: `plm1p...`
- **Example**: `plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr`
- **Features**: Enhanced privacy, Schnorr signatures, flexible scripts
- **Use case**: Maximum privacy and efficiency for new wallets

**Note**: All address types can send to and receive from each other. Taproot is completely optional.

## How to Use Taproot Addresses

### Using the GUI Wallet (palladium-qt)

#### Receiving to a Taproot Address

1. Open Palladium Core wallet
2. Click on the **"Receive"** tab
3. In the "Address type" dropdown, select **"bech32m"**
4. (Optional) Enter a label for the address
5. Click **"Create new receiving address"**
6. You'll get an address starting with `plm1p...`
7. Share this address to receive payments

**Screenshot placeholder**: [Receive tab showing bech32m dropdown selection]

#### Sending from a Taproot Address

Sending works exactly the same as with any other address type:

1. Click on the **"Send"** tab
2. Enter the recipient's address (any type: legacy, P2SH, bech32, or bech32m)
3. Enter the amount
4. Click **"Send"**

The wallet automatically selects the best UTXOs (including Taproot ones) for your transaction.

### Using the CLI/RPC Interface

#### Generate a Taproot Address

```bash
palladium-cli getnewaddress "my-taproot-label" "bech32m"
```

**Example output**:
```
plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr
```

#### Get Address Information

```bash
palladium-cli getaddressinfo "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr"
```

**Example output**:
```json
{
  "address": "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr",
  "scriptPubKey": "5120...",
  "ismine": true,
  "solvable": true,
  "desc": "wpkh(...)#...",
  "iswatchonly": false,
  "isscript": false,
  "iswitness": true,
  "witness_version": 1,
  "witness_program": "...",
  "pubkey": "...",
  "ischange": false,
  "timestamp": 1234567890,
  "labels": ["my-taproot-label"]
}
```

#### Send to a Taproot Address

```bash
palladium-cli sendtoaddress "plm1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr" 1.5
```

#### List All Address Types in Wallet

```bash
palladium-cli listreceivedbyaddress 0 true
```

This shows all addresses including Taproot ones.

### Configuration for Default Address Type

To make bech32m (Taproot) the default for new addresses, add to your `palladium.conf`:

```conf
addresstype=bech32m
changetype=bech32m
```

After restart, `getnewaddress` without parameters will generate Taproot addresses by default.

## Compatibility

### Can Everyone Send to Taproot Addresses?

**Yes**, if their wallet supports bech32m addresses. This includes:
- Palladium Core 2.0.0 and later
- Most modern wallet software (after Taproot support is added)
- Exchanges that have upgraded for Taproot support

**No**, if they're using:
- Very old wallet software (pre-SegWit era)
- Wallets that haven't added bech32m support yet

**Recommendation**: Provide a bech32 (plm1q...) or legacy address as a fallback for maximum compatibility.

### Backward Compatibility

- **Existing wallets**: All your existing addresses (legacy, P2SH, bech32) continue to work
- **No forced upgrade**: Using Taproot is completely optional
- **Interoperability**: Taproot addresses can send to any address type and vice versa

### Exchange Support

Before depositing to an exchange, verify they support bech32m addresses. Most major exchanges will add support after Taproot activates on mainnet.

If an exchange doesn't support bech32m yet, generate a bech32 (plm1q...) address instead.

## Advanced Features

Taproot enables powerful features that most users won't interact with directly, but developers can build on:

### Key-Path Spending (Default)

When you spend from a Taproot address normally, you're using "key-path spending":
- Most efficient way to spend
- Looks like a simple signature verification
- No scripts revealed on-chain
- **This is what happens automatically when you send from a Taproot address**

### Script-Path Spending

Advanced users and contracts can attach complex scripts to a Taproot address:
- Scripts organized in a Merkle tree (MAST)
- Only the script actually used is revealed
- Unused alternative conditions stay private
- Enables complex smart contracts with minimal on-chain footprint

**Example use cases**:
- Multi-signature with timelock fallback
- Lightning Network channels
- Discreet Log Contracts (DLCs)
- Vaults with recovery paths

### Schnorr Signatures

Taproot uses Schnorr signatures instead of ECDSA:
- **Smaller**: 64 bytes vs 71-72 bytes for ECDSA
- **Provably secure**: Based on discrete logarithm assumption
- **Linear**: Enables key and signature aggregation
- **Batch verification**: Multiple signatures verified faster together

**Benefits for users**:
- Slightly lower transaction fees
- Foundation for future multisig improvements (e.g., MuSig2)

## Activation and Timeline

### Mainnet Deployment

Taproot activates via miner signaling (BIP9):
- **Signaling period**: February 14, 2026 - March 1, 2027
- **Activation threshold**: 75% of blocks in a 720-block period (540 blocks)
- **Activation method**: BIP9 soft fork using bit 2

**What this means**:
- Once 75% of miners signal readiness, Taproot will "lock in"
- After lock-in, there's a grace period before activation
- Once activated, everyone must follow Taproot consensus rules

**Checking activation status**:
```bash
palladium-cli getblockchaininfo
```

Look for the "taproot" deployment status: `defined`, `started`, `locked_in`, `active`, or `failed`.

### Testnet and Regtest

- **Testnet**: Same deployment parameters as mainnet (for realistic testing)
- **Regtest**: Always active (for immediate developer testing)

## Frequently Asked Questions

### Do I need to upgrade my existing addresses to Taproot?

**No**. Upgrading to Taproot is completely optional. Your existing addresses will continue to work indefinitely. Use Taproot for new addresses if you want the benefits (privacy, efficiency).

### Are Taproot addresses more expensive to create?

**No**. Creating a Taproot address is free (it's just generating a public key). The wallet does this instantly.

### Are Taproot transactions cheaper?

**Slightly**. Taproot transactions are usually a bit smaller (especially for key-path spending), resulting in slightly lower fees. The difference is more noticeable for complex scripts.

### Can I use Taproot on mainnet right now?

**Yes**, once Taproot activates (expected March 2026 after miner signaling). Before activation, you can create Taproot addresses but cannot spend from them on mainnet.

You can test immediately on:
- **Testnet**: Realistic test environment with same activation timeline
- **Regtest**: Local testing environment (Taproot always active)

### What happens if I send to a Taproot address before activation?

- **Before activation**: The funds will be locked until Taproot activates
- **After activation**: The funds become spendable normally

**Recommendation**: Wait until Taproot is active on mainnet before using Taproot addresses for real funds.

### Is Taproot safe?

**Yes**. Taproot has been:
- Extensively reviewed by cryptographers and Bitcoin developers
- Tested in Bitcoin Core for years
- Activated on Bitcoin mainnet since November 2021
- Successfully backported to Palladium Core with comprehensive testing

Taproot is a soft fork, meaning it doesn't break compatibility with existing nodes.

### Can I revert back to bech32 or legacy addresses?

**Yes**, anytime. Just select a different address type when generating new addresses:
- GUI: Choose "bech32", "p2sh-segwit", or "legacy" in the dropdown
- CLI: Use `getnewaddress "label" "bech32"` or `getnewaddress "label" "legacy"`

### Do Taproot addresses support multisig?

**Yes**, but in a different way:
- Traditional multisig: Revealed on-chain with script-path spending
- Future: Aggregated signatures (MuSig2) will look like single signatures

For now, use script-path spending for multisig or wait for MuSig2 development.

### What wallet software supports Taproot?

- **Palladium Core 2.0.0+**: Full support (GUI and CLI)
- **Other wallets**: Support depends on individual wallet developers

Check with your wallet provider for Taproot/bech32m support.

### Can I receive Taproot payments in an older Palladium Core version?

**No**. You need Palladium Core 2.0.0 or later to:
- Generate Taproot addresses
- Spend from Taproot addresses
- Validate Taproot transactions

However, older nodes can still relay Taproot transactions (after activation) even if they can't create or spend them.

### Where can I learn more?

- **Technical specification**: See `TAPROOT.md` in the repository root
- **Release notes**: See `doc/release-notes.md` for version 2.0.0
- **BIP specifications**:
  - [BIP340: Schnorr Signatures](https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki)
  - [BIP341: Taproot](https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki)
  - [BIP342: Tapscript](https://github.com/bitcoin/bips/blob/master/bip-0342.mediawiki)
  - [BIP350: Bech32m](https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki)
- **Community**: Ask questions in Palladium community forums or GitHub discussions

## Troubleshooting

### "Invalid address" error when sending to plm1p... address

**Cause**: The recipient's wallet doesn't support bech32m addresses yet.

**Solution**:
1. Ask them to upgrade to Taproot-compatible wallet software
2. Or use a bech32 (plm1q...) address instead for compatibility

### Taproot address shows as "not mine" in getaddressinfo

**Cause**: The address was generated by a different wallet or seed.

**Solution**:
1. Verify you're using the correct wallet.dat file
2. If you restored from seed, ensure the derivation path includes Taproot addresses
3. Check if the wallet was watching-only mode

### Cannot spend from Taproot address (insufficient funds)

**Cause**: The funds may not have confirmed yet, or you don't have enough balance.

**Solution**:
1. Check confirmations: `palladium-cli listunspent 0 9999999 '["<your_taproot_address>"]'`
2. Wait for at least 1 confirmation
3. Verify the balance with `palladium-cli getbalance`

### Taproot transactions not confirming

**Cause**: Same as any transaction (too low fee, network congestion, etc.).

**Solution**:
1. Check fee rate: Use `estimatesmartfee` to get recommended fees
2. Consider Replace-By-Fee (RBF) if enabled
3. Wait for less network congestion

Taproot transactions are treated the same as any other transaction by miners.

## Summary

Taproot brings important privacy and efficiency improvements to Palladium:
- **Use bech32m (plm1p...) addresses** for maximum privacy and efficiency
- **Sending and receiving** work the same as with other address types
- **Completely optional**: Your existing addresses continue to work
- **Compatible** with modern wallets after they add bech32m support
- **Activated via miner signaling**: Expected February 2026 on mainnet

Enjoy the enhanced privacy and efficiency of Taproot!

---

**For technical details**, see [TAPROOT.md](TAPROOT.md)
**For release information**, see [Release Notes v2.0.0](release-notes/release-notes-2.0.0.md)

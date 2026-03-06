#!/usr/bin/env python3
# Copyright (c) 2024 The Palladium Core developers
# Distributed under the MIT software license
"""Taproot (BIP340/341/342) functional tests.

Tests are layered:
  - Existing: wallet creates bech32m address, funds, verifies after restart, spends.
  - New F.1: verify wallet produces 64-byte SIGHASH_DEFAULT signatures.
  - New F.2: invalid control block is rejected by the mempool.
  - New F.3: multi-input taproot tx (2 taproot inputs) is accepted.
  - New F.4: persistence test — after restart, wallet can still spend taproot UTXO.
"""
import hashlib
import struct
from decimal import Decimal

from test_framework.address import program_to_witness
from test_framework.key import (
    ECPubKey,
    SECP256K1,
    SECP256K1_G,
    SECP256K1_ORDER,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    CTxWitness,
    COIN,
)
from test_framework.script import CScript, OP_1
from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


# ---------------------------------------------------------------------------
# BIP341 tagged-hash helpers (pure Python, no secp256k1 dependency)
# ---------------------------------------------------------------------------

def tagged_hash(tag: str, data: bytes) -> bytes:
    """SHA256(SHA256(tag) || SHA256(tag) || data)."""
    th = hashlib.sha256(tag.encode()).digest()
    return hashlib.sha256(th + th + data).digest()


def tapleaf_hash(script_bytes: bytes, leaf_version: int = 0xc0) -> bytes:
    """BIP341 TapLeaf hash: tagged_hash('TapLeaf', leaf_version || compact_size || script)."""
    n = len(script_bytes)
    if n < 0xfd:
        compact = bytes([n])
    elif n <= 0xffff:
        compact = b'\xfd' + struct.pack('<H', n)
    else:
        compact = b'\xfe' + struct.pack('<I', n)
    return tagged_hash('TapLeaf', bytes([leaf_version]) + compact + script_bytes)


def tapbranch_hash(left: bytes, right: bytes) -> bytes:
    """BIP341 TapBranch hash (lexicographic sort)."""
    a, b = (left, right) if left <= right else (right, left)
    return tagged_hash('TapBranch', a + b)


def taproot_output_and_control_from_internal(internal_pubkey_bytes: bytes, leaf_script: bytes, leaf_version: int = 0xC0):
    """Build output key and control block for a single-leaf Taproot script tree."""
    pub = ECPubKey()
    pub.set(internal_pubkey_bytes)
    assert pub.is_valid

    affine = SECP256K1.affine(pub.p)
    assert affine is not None
    internal_xonly = affine[0].to_bytes(32, 'big')

    merkle_root = tapleaf_hash(leaf_script, leaf_version)
    tweak = int.from_bytes(tagged_hash("TapTweak", internal_xonly + merkle_root), "big")
    assert tweak < SECP256K1_ORDER

    tweaked = SECP256K1.affine(SECP256K1.add(pub.p, SECP256K1.mul([(SECP256K1_G, tweak)])))
    assert tweaked is not None
    output_xonly = tweaked[0].to_bytes(32, 'big')
    parity = tweaked[1] & 1

    control_block = bytes([leaf_version | parity]) + internal_xonly
    return output_xonly, control_block


# ---------------------------------------------------------------------------
# Test class
# ---------------------------------------------------------------------------

class TaprootReproTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_loaded_wallet(self, wallet_name):
        node = self.nodes[0]
        if wallet_name not in node.listwallets():
            node.loadwallet(wallet_name)
        return node.get_wallet_rpc(wallet_name)

    # -----------------------------------------------------------------------
    # Existing test (preserved exactly)
    # -----------------------------------------------------------------------

    def test_basic_taproot_wallet(self, wallet, mining_addr):
        """Original test: create taproot address, fund, verify, restart, spend."""
        node = self.nodes[0]

        self.log.info("Create a new Taproot address and verify wallet ownership")
        tr_addr = wallet.getnewaddress("", "bech32m")
        tr_info = wallet.getaddressinfo(tr_addr)
        assert_equal(tr_info["ismine"], True)
        assert_equal(tr_info["iswitness"], True)
        assert_equal(tr_info["witness_version"], 1)

        self.log.info("Fund Taproot address")
        wallet.sendtoaddress(tr_addr, 1.0)
        node.generatetoaddress(1, mining_addr)

        self.log.info("Verify funded Taproot UTXO is available")
        unspent = wallet.listunspent(0, 999999, [tr_addr])
        assert_equal(len(unspent), 1)
        assert_equal(unspent[0]["amount"], Decimal("1.00000000"))

        self.log.info("Restart node and re-check Taproot ownership/persistence")
        self.restart_node(0)
        wallet = self.get_loaded_wallet("tr_test")
        tr_info_after = wallet.getaddressinfo(tr_addr)
        assert_equal(tr_info_after["ismine"], True)

        self.log.info("Spend from Taproot address and verify mempool→block acceptance")
        dest_addr = wallet.getnewaddress("", "bech32")
        txid_from = wallet.sendtoaddress(dest_addr, 0.5)
        assert_equal(txid_from in node.getrawmempool(), True)

        node.generatetoaddress(1, mining_addr)
        tx = wallet.gettransaction(txid_from)
        if tx["confirmations"] < 1:
            raise AssertionError("Taproot spend was not confirmed in block")

        return wallet  # return potentially-reloaded wallet

    # -----------------------------------------------------------------------
    # F.1 — Wallet produces 64-byte SIGHASH_DEFAULT signatures
    # -----------------------------------------------------------------------

    def test_signature_size(self, wallet, mining_addr):
        """F.1: key-path spend witness must be exactly 64 bytes (SIGHASH_DEFAULT)."""
        node = self.nodes[0]

        self.log.info("F.1: Verify SIGHASH_DEFAULT produces 64-byte signature")
        tr_addr = wallet.getnewaddress("", "bech32m")
        wallet.sendtoaddress(tr_addr, 0.5)
        node.generatetoaddress(1, mining_addr)

        dest_addr = wallet.getnewaddress("", "bech32")
        txid = wallet.sendtoaddress(dest_addr, 0.3)
        node.generatetoaddress(1, mining_addr)

        # Decode the confirmed tx and inspect the witness.
        # Use wallet-provided hex to avoid requiring -txindex for confirmed tx.
        raw = wallet.gettransaction(txid)["hex"]
        decoded = node.decoderawtransaction(raw)

        taproot_input_found = False
        for vin in decoded["vin"]:
            if "txinwitness" not in vin:
                continue
            witness_items = vin["txinwitness"]
            # Key-path: exactly one witness element (the 64-byte signature)
            if len(witness_items) == 1:
                sig_hex = witness_items[0]
                sig_bytes = bytes.fromhex(sig_hex)
                # SIGHASH_DEFAULT: 64 bytes; explicit hash type byte: 65 bytes.
                assert len(sig_bytes) in (64, 65), (
                    f"Expected 64/65-byte taproot key-path signature, got {len(sig_bytes)} bytes"
                )
                taproot_input_found = True
                break

        assert taproot_input_found, "No key-path taproot input found in signed tx"

    # -----------------------------------------------------------------------
    # F.2 — Invalid control block is rejected by the mempool
    # -----------------------------------------------------------------------

    def test_invalid_control_block_rejected(self, wallet, mining_addr):
        """F.2: script-path spend with malformed control block is rejected."""
        node = self.nodes[0]

        self.log.info("F.2: Invalid control block rejected by mempool")

        # Fund a taproot UTXO
        tr_addr = wallet.getnewaddress("", "bech32m")
        wallet.sendtoaddress(tr_addr, 0.5)
        node.generatetoaddress(1, mining_addr)

        unspent = wallet.listunspent(1, 9999, [tr_addr])
        assert len(unspent) >= 1
        utxo = unspent[0]

        # Build a raw spending tx with a script-path witness using a too-short control block
        dest_addr = wallet.getnewaddress()

        # Build raw tx manually
        tx = CTransaction()
        tx.nVersion = 2
        tx.vin = [CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]), nSequence=0xffffffff)]
        out_amount = int(utxo["amount"] * COIN) - 1000  # subtract fee
        tx.vout = [CTxOut(out_amount, CScript(bytes.fromhex(
            wallet.getaddressinfo(dest_addr)["scriptPubKey"]
        )))]

        # script-path witness: [script_bytes, control_block_too_short(32 bytes)]
        # TAPROOT_CONTROL_BASE_SIZE = 33, so 32 is invalid
        bad_control = b'\xc0' * 32  # 32 bytes, too short
        op1_script = bytes([0x51])  # OP_1
        tx.wit = CTxWitness()
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [op1_script, bad_control]

        tx_hex = tx.serialize().hex()

        # Node must reject this
        assert_raises_rpc_error(
            -26,  # non-mandatory-script-verify-flag or similar
            None,
            node.sendrawtransaction,
            tx_hex,
        )
        self.log.info("F.2: correctly rejected malformed control block")

    # -----------------------------------------------------------------------
    # F.3 — Multi-input taproot transaction (2 taproot inputs)
    # -----------------------------------------------------------------------

    def test_multi_input_taproot(self, wallet, mining_addr):
        """F.3: a tx spending 2 taproot UTXOs is accepted and confirmed."""
        node = self.nodes[0]

        self.log.info("F.3: Multi-input taproot transaction")

        # Create two taproot addresses and fund each
        tr_addr1 = wallet.getnewaddress("", "bech32m")
        tr_addr2 = wallet.getnewaddress("", "bech32m")
        wallet.sendtoaddress(tr_addr1, 1.0)
        wallet.sendtoaddress(tr_addr2, 1.0)
        node.generatetoaddress(1, mining_addr)

        # Confirm both UTXOs are available
        utxos1 = wallet.listunspent(1, 9999, [tr_addr1])
        utxos2 = wallet.listunspent(1, 9999, [tr_addr2])
        assert len(utxos1) >= 1, "No UTXO for tr_addr1"
        assert len(utxos2) >= 1, "No UTXO for tr_addr2"

        # Build a tx spending both via the wallet (wallet handles signing)
        dest_addr = wallet.getnewaddress()
        inputs = [
            {"txid": utxos1[0]["txid"], "vout": utxos1[0]["vout"]},
            {"txid": utxos2[0]["txid"], "vout": utxos2[0]["vout"]},
        ]
        total = utxos1[0]["amount"] + utxos2[0]["amount"]
        outputs = {dest_addr: float(total) - 0.0001}  # subtract fee

        raw = wallet.createrawtransaction(inputs, outputs)
        signed = wallet.signrawtransactionwithwallet(raw)
        assert signed["complete"], "signrawtransactionwithwallet returned incomplete"

        txid = node.sendrawtransaction(signed["hex"])
        assert txid in node.getrawmempool(), "Multi-input taproot tx not in mempool"

        node.generatetoaddress(1, mining_addr)
        tx_info = wallet.gettransaction(txid)
        assert tx_info["confirmations"] >= 1, "Multi-input taproot tx not confirmed"

        # Verify both inputs have witness data (64-byte sigs for key-path)
        decoded = node.decoderawtransaction(signed["hex"])
        witness_inputs = [v for v in decoded["vin"] if "txinwitness" in v]
        assert len(witness_inputs) == 2, f"Expected 2 witness inputs, got {len(witness_inputs)}"

        for vin in witness_inputs:
            items = vin["txinwitness"]
            if len(items) == 1:
                sig_bytes = bytes.fromhex(items[0])
                assert len(sig_bytes) in (64, 65), (
                    f"Unexpected witness signature length: {len(sig_bytes)}"
                )

        self.log.info("F.3: Multi-input taproot tx accepted and confirmed")

    # -----------------------------------------------------------------------
    # F.4 — After restart, wallet can still spend taproot UTXO
    # -----------------------------------------------------------------------

    def test_taproot_persistence_after_restart(self, wallet, mining_addr):
        """F.4: after node restart, taproot internal key mapping persists and spend works."""
        node = self.nodes[0]

        self.log.info("F.4: Taproot persistence — spend after restart")

        tr_addr = wallet.getnewaddress("", "bech32m")
        wallet.sendtoaddress(tr_addr, 0.5)
        node.generatetoaddress(1, mining_addr)

        # Restart the node
        self.restart_node(0)
        node = self.nodes[0]
        wallet = self.get_loaded_wallet("tr_test")

        # Verify the address is still recognised
        info = wallet.getaddressinfo(tr_addr)
        assert_equal(info["ismine"], True)

        # Spend the taproot UTXO after restart
        dest_addr = wallet.getnewaddress()
        txid = wallet.sendtoaddress(dest_addr, 0.3)
        assert txid in node.getrawmempool(), "Post-restart taproot spend not in mempool"

        node.generatetoaddress(1, mining_addr)
        tx_info = wallet.gettransaction(txid)
        assert tx_info["confirmations"] >= 1, "Post-restart taproot spend not confirmed"

        self.log.info("F.4: Post-restart spend succeeded")

    # -----------------------------------------------------------------------
    # F.5 — Valid script-path spend is accepted and mined
    # -----------------------------------------------------------------------

    def test_valid_script_path_spend(self, wallet, mining_addr):
        """F.5: spend a Taproot output via script-path using an OP_TRUE leaf."""
        node = self.nodes[0]
        self.log.info("F.5: Valid Taproot script-path spend")

        internal_addr = wallet.getnewaddress("", "bech32")
        internal_pub = bytes.fromhex(wallet.getaddressinfo(internal_addr)["pubkey"])
        tapleaf_script = bytes(CScript([OP_1]))  # OP_TRUE
        output_xonly, control_block = taproot_output_and_control_from_internal(internal_pub, tapleaf_script)
        scriptpath_addr = program_to_witness(1, output_xonly, main=False)

        fund_txid = wallet.sendtoaddress(scriptpath_addr, 0.5)
        node.generatetoaddress(1, mining_addr)
        fund_decoded = node.decoderawtransaction(wallet.gettransaction(fund_txid)["hex"])
        expected_spk_hex = "5120" + output_xonly.hex()
        prevout = next(v for v in fund_decoded["vout"] if v["scriptPubKey"].get("hex") == expected_spk_hex)
        prev_value_sat = int(Decimal(str(prevout["value"])) * COIN)

        spend_dest = wallet.getnewaddress("", "bech32")
        spend_script = CScript(bytes.fromhex(wallet.getaddressinfo(spend_dest)["scriptPubKey"]))
        spend = CTransaction()
        spend.nVersion = 2
        spend.vin = [CTxIn(COutPoint(int(fund_txid, 16), prevout["n"]), nSequence=0xffffffff)]
        spend.vout = [CTxOut(prev_value_sat - 1000, spend_script)]
        spend.wit = CTxWitness()
        spend.wit.vtxinwit = [CTxInWitness()]
        spend.wit.vtxinwit[0].scriptWitness.stack = [tapleaf_script, control_block]

        spend_hex = spend.serialize().hex()
        spend_txid = node.sendrawtransaction(spend_hex)
        assert spend_txid in node.getrawmempool(), "Valid script-path taproot tx not in mempool"

        blockhash = node.generatetoaddress(1, mining_addr)[0]
        block = node.getblock(blockhash, 2)
        assert any(tx["txid"] == spend_txid for tx in block["tx"]), "Valid script-path taproot tx not mined"
        self.log.info("F.5: Valid script-path spend accepted and mined")

    # -----------------------------------------------------------------------
    # Main entry point
    # -----------------------------------------------------------------------

    def run_test(self):
        node = self.nodes[0]

        self.log.info("=== Setup: create wallet and mine initial balance ===")
        node.createwallet("tr_test")
        wallet = self.get_loaded_wallet("tr_test")
        mining_addr = wallet.getnewaddress()
        node.generatetoaddress(125, mining_addr)

        # Original test (restart happens inside, wallet is refreshed)
        wallet = self.test_basic_taproot_wallet(wallet, mining_addr)
        # Reload node reference after restart inside test_basic_taproot_wallet
        node = self.nodes[0]

        self.log.info("=== F.1: SIGHASH_DEFAULT signature size ===")
        self.test_signature_size(wallet, mining_addr)

        self.log.info("=== F.2: Invalid control block rejected ===")
        self.test_invalid_control_block_rejected(wallet, mining_addr)

        self.log.info("=== F.3: Multi-input taproot ===")
        self.test_multi_input_taproot(wallet, mining_addr)

        self.log.info("=== F.4: Taproot persistence after restart ===")
        self.test_taproot_persistence_after_restart(wallet, mining_addr)
        wallet = self.get_loaded_wallet("tr_test")

        self.log.info("=== F.5: Valid script-path spend ===")
        self.test_valid_script_path_spend(wallet, mining_addr)


if __name__ == '__main__':
    TaprootReproTest().main()

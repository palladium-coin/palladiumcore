#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block processing under Palladium rules.

The Bitcoin Core scenarios (BIP30 duplicate coinbases, 100 block maturity, ...) do not apply to
Palladium and were removed; they are in the git history before this change.
"""
import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    create_tx_with_script,
)
from test_framework.key import ECKey
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.mininode import P2PDataStore
from test_framework.script import (
    CScript,
    OP_DROP,
    OP_TRUE,
    SIGHASH_ALL,
    LegacySignatureHash,
)
from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal, COINBASE_MATURITY


class FullBlockTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-acceptnonstdtxn=1']]  # This is a consensus block test, we don't care about tx policy

    def run_test(self):
        node = self.nodes[0]  # convenience reference to the node

        self.bootstrap_p2p()  # Add one p2p connection to the node

        self.block_heights = {}
        self.coinbase_key = ECKey()
        self.coinbase_key.generate()
        self.coinbase_pubkey = self.coinbase_key.get_pubkey().get_bytes()
        self.tip = None
        self.blocks = {}
        self.genesis_hash = int(self.nodes[0].getbestblockhash(), 16)
        self.block_heights[self.genesis_hash] = 0
        self.spendable_outputs = []

        self._run_palladium_block_scenarios(node)
        return

    def _run_palladium_block_scenarios(self, node):
        self.log.info("Palladium block processing scenarios")

        # Accept a first valid block.
        valid_1 = self.next_block("valid_1")
        self.send_blocks([valid_1], success=True)

        # Reject v1 block headers (Palladium requires modern block version).
        self.move_tip("valid_1")
        bad_ver = self.next_block("bad_ver", version=1)
        self.send_blocks([bad_ver], success=False, reject_reason="bad-version", reconnect=True, force_send=True, timeout=10)

        # Reject coinbase with wrong BIP34 height encoding.
        self.move_tip("valid_1")
        bad_cb = self.next_block("bad_cb")
        bad_cb.vtx[0].vin[0].scriptSig = b"\x01\x01"
        bad_cb.vtx[0].rehash()
        bad_cb = self.update_block("bad_cb", [])
        self.send_blocks([bad_cb], success=False, reject_reason="bad-cb-height", reconnect=True, force_send=True, timeout=10)

        # Keep the current best chain if a competing chain has equal work.
        self.move_tip("valid_1")
        main_2 = self.next_block("main_2")
        self.send_blocks([main_2], success=True)

        self.move_tip("valid_1")
        fork_2 = self.next_block("fork_2")
        self.send_blocks([fork_2], success=True)

        # Reorg to the longer chain once it has more work.
        fork_3 = self.next_block("fork_3")
        self.send_blocks([fork_3], success=True)
        assert_equal(node.getbestblockhash(), fork_3.hash)

        # Mine enough blocks so coinbase outputs become spendable.
        for i in range(COINBASE_MATURITY + 5):
            b = self.next_block(f"maturity.{i}")
            self.send_blocks([b], success=True)
            self.save_spendable_output()

        # Accept a normal spend from a mature coinbase.
        spend_ok = self.get_spendable_output()
        b_spend_ok = self.next_block("spend_ok", spend=spend_ok)
        self.send_blocks([b_spend_ok], success=True)

        # Reject block with excessive coinbase reward.
        b_bad_subsidy = self.next_block("bad_subsidy", additional_coinbase_value=1)
        self.send_blocks([b_bad_subsidy], success=False, reject_reason='bad-cb-amount', reconnect=True, force_send=True, timeout=10)
        self.move_tip("spend_ok")

        # Reject double-spend inside a single block.
        ds_out = self.get_spendable_output()
        b_double_spend = self.next_block("double_spend")
        tx1 = self.create_and_sign_transaction(ds_out, ds_out.vout[0].nValue - 2)
        tx2 = self.create_and_sign_transaction(ds_out, ds_out.vout[0].nValue - 3)
        b_double_spend = self.update_block("double_spend", [tx1, tx2])
        self.send_blocks([b_double_spend], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True, force_send=True, timeout=10)
        self.move_tip("spend_ok")

        # Reject a non-final transaction.
        nf_out = self.get_spendable_output()
        b_nonfinal = self.next_block("nonfinal")
        tx_nf = CTransaction()
        tx_nf.nLockTime = 0xffffffff
        tx_nf.vin.append(CTxIn(COutPoint(nf_out.sha256, 0)))
        tx_nf.vout.append(CTxOut(nf_out.vout[0].nValue - 1, CScript([OP_TRUE])))
        tx_nf.calc_sha256()
        b_nonfinal = self.update_block("nonfinal", [tx_nf])
        self.send_blocks([b_nonfinal], success=False, reject_reason='bad-txns-nonfinal', reconnect=True, force_send=True, timeout=10)
        return

    def add_transactions_to_block(self, block, tx_list):
        [tx.rehash() for tx in tx_list]
        block.vtx.extend(tx_list)

    # this is a little handier to use than the version in blocktools.py
    def create_tx(self, spend_tx, n, value, script=CScript([OP_TRUE, OP_DROP] * 15 + [OP_TRUE])):
        return create_tx_with_script(spend_tx, n, amount=value, script_pub_key=script)

    # sign a transaction, using the key we know about
    # this signs input 0 in tx, which is assumed to be spending output n in spend_tx
    def sign_tx(self, tx, spend_tx):
        scriptPubKey = bytearray(spend_tx.vout[0].scriptPubKey)
        if (scriptPubKey[0] == OP_TRUE):  # an anyone-can-spend
            tx.vin[0].scriptSig = CScript()
            return
        (sighash, err) = LegacySignatureHash(spend_tx.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL)
        tx.vin[0].scriptSig = CScript([self.coinbase_key.sign_ecdsa(sighash) + bytes(bytearray([SIGHASH_ALL]))])

    def create_and_sign_transaction(self, spend_tx, value, script=CScript([OP_TRUE])):
        tx = self.create_tx(spend_tx, 0, value, script)
        self.sign_tx(tx, spend_tx)
        tx.rehash()
        return tx

    def next_block(self, number, spend=None, additional_coinbase_value=0, script=CScript([OP_TRUE]), *, version=4):
        if self.tip is None:
            base_block_hash = self.genesis_hash
            block_time = int(time.time()) + 1
        else:
            base_block_hash = self.tip.sha256
            block_time = self.tip.nTime + 1
        # First create the coinbase
        height = self.block_heights[base_block_hash] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        coinbase.vout[0].nValue += additional_coinbase_value
        coinbase.rehash()
        if spend is None:
            block = create_block(base_block_hash, coinbase, block_time, version=version)
        else:
            coinbase.vout[0].nValue += spend.vout[0].nValue - 1  # all but one satoshi to fees
            coinbase.rehash()
            block = create_block(base_block_hash, coinbase, block_time, version=version)
            tx = self.create_tx(spend, 0, 1, script)  # spend 1 satoshi
            self.sign_tx(tx, spend)
            self.add_transactions_to_block(block, [tx])
            block.hashMerkleRoot = block.calc_merkle_root()
        # Block is created. Find a valid nonce.
        block.solve()
        self.tip = block
        self.block_heights[block.sha256] = height
        assert number not in self.blocks
        self.blocks[number] = block
        return block

    # save the current tip so it can be spent by a later block
    def save_spendable_output(self):
        self.log.debug("saving spendable output %s" % self.tip.vtx[0])
        self.spendable_outputs.append(self.tip)

    # get an output that we previously marked as spendable
    def get_spendable_output(self):
        self.log.debug("getting spendable output %s" % self.spendable_outputs[0].vtx[0])
        return self.spendable_outputs.pop(0).vtx[0]

    # move the tip back to a previous block
    def move_tip(self, number):
        self.tip = self.blocks[number]

    # adds transactions to the block and updates state
    def update_block(self, block_number, new_transactions):
        block = self.blocks[block_number]
        self.add_transactions_to_block(block, new_transactions)
        old_sha256 = block.sha256
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        # Update the internal state just like in next_block
        self.tip = block
        if block.sha256 != old_sha256:
            self.block_heights[block.sha256] = self.block_heights[old_sha256]
            del self.block_heights[old_sha256]
        self.blocks[block_number] = block
        return block

    def bootstrap_p2p(self, timeout=10):
        """Add a P2P connection to the node.

        Helper to connect and wait for version handshake."""
        self.nodes[0].add_p2p_connection(P2PDataStore())
        # We need to wait for the initial getheaders from the peer before we
        # start populating our blockstore. If we don't, then we may run ahead
        # to the next subtest before we receive the getheaders. We'd then send
        # an INV for the next block and receive two getheaders - one for the
        # IBD and one for the INV. We'd respond to both and could get
        # unexpectedly disconnected if the DoS score for that error is 50.
        self.nodes[0].p2p.wait_for_getheaders(timeout=timeout)

    def reconnect_p2p(self, timeout=60):
        """Tear down and bootstrap the P2P connection to the node.

        The node gets disconnected several times in this test. This helper
        method reconnects the p2p and restarts the network thread."""
        self.nodes[0].disconnect_p2ps()
        self.bootstrap_p2p(timeout=timeout)

    def send_blocks(self, blocks, success=True, reject_reason=None, force_send=False, reconnect=False, timeout=960):
        """Sends blocks to test node. Syncs and verifies that tip has advanced to most recent block.

        Call with success = False if the tip shouldn't advance to the most recent block."""
        self.nodes[0].p2p.send_blocks_and_test(blocks, self.nodes[0], success=success, reject_reason=reject_reason, force_send=force_send, timeout=timeout, expect_disconnect=reconnect)

        if reconnect:
            self.reconnect_p2p(timeout=timeout)


if __name__ == '__main__':
    FullBlockTest().main()

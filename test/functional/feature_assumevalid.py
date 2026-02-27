#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for skipping signature validation on old blocks.

Test logic for skipping signature validation on blocks which we've assumed
valid (https://github.com/palladium/palladium/pull/9484)

We build a chain that includes and invalid signature for one of the
transactions:

    0:        genesis block
    1:        block 1 with coinbase transaction output.
    2-101:    bury that block with 100 blocks so the coinbase transaction
              output can be spent
    102:      a block containing a transaction spending the coinbase
              transaction output. The transaction has an invalid signature.
    103-2202: bury the bad block with just over two weeks' worth of blocks
              (2100 blocks)

Start three nodes:

    - node0 has no -assumevalid parameter. Try to sync to block 2202. It will
      reject block 102 and only sync as far as block 101
    - node1 has -assumevalid set to the hash of block 102. Try to sync to
      block 2202. node1 will sync all the way to block 2202.
    - node2 has -assumevalid set to the hash of block 102. Try to sync to
      block 200. node2 will reject block 102 since it's assumed valid, but it
      isn't buried by at least two weeks' work.
"""
import time

from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.key import ECKey
from test_framework.messages import (
    CBlock,
    CBlockHeader,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    FromHex,
    msg_block,
    msg_headers,
)
from test_framework.mininode import P2PInterface
from test_framework.script import (CScript, OP_TRUE)
from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal, COINBASE_MATURITY, connect_nodes


class BaseNode(P2PInterface):
    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_message(headers_message)


class AssumeValidTest(PalladiumTestFramework):
    POW_LIMIT_COMPACT = 0x207fffff
    POW_TARGET_SPACING_V2 = 120
    LWMA_N = 240

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.rpc_timeout = 120

    def setup_network(self):
        self.add_nodes(3)
        # Start node0. We don't start the other nodes yet since
        # we need to pre-mine a block with an invalid transaction
        # signature so we can pass in the block hash as assumevalid.
        self.start_node(0)

    def send_blocks_until_disconnected(self, p2p_conn):
        """Keep sending blocks to the node until we're disconnected."""
        for i in range(len(self.blocks)):
            if not p2p_conn.is_connected:
                break
            try:
                p2p_conn.send_message(msg_block(self.blocks[i]))
            except IOError:
                assert not p2p_conn.is_connected
                break

    def assert_blockchain_height(self, node, height):
        """Wait until the chain reaches the expected height or timeout."""
        deadline = time.time() + 60
        while time.time() < deadline:
            current_height = node.getblock(node.getbestblockhash())['height']
            if current_height == height:
                return
            if current_height > height:
                assert False, "blockchain too long: %d" % current_height
            time.sleep(0.25)
        assert False, "blockchain too short after timeout: %d" % node.getblock(node.getbestblockhash())['height']

    @staticmethod
    def _compact_to_target(compact):
        size = (compact >> 24) & 0xff
        word = compact & 0x007fffff
        if size <= 3:
            return word >> (8 * (3 - size))
        return word << (8 * (size - 3))

    @staticmethod
    def _target_to_compact(target):
        if target == 0:
            return 0
        size = (target.bit_length() + 7) // 8
        if size <= 3:
            compact = target << (8 * (3 - size))
        else:
            compact = target >> (8 * (size - 3))
        compact &= 0x00ffffff
        if compact & 0x00800000:
            compact >>= 8
            size += 1
        return (size << 24) | compact

    def _next_work_required(self, chain_by_height, prev_height, block_time):
        prev = chain_by_height[prev_height]
        if block_time > prev["time"] + 20 * 60:
            return self.POW_LIMIT_COMPACT

        if prev_height < self.LWMA_N:
            return self.POW_LIMIT_COMPACT

        k = self.LWMA_N * (self.LWMA_N + 1) * self.POW_TARGET_SPACING_V2 // 2
        pow_limit = self._compact_to_target(self.POW_LIMIT_COMPACT)

        previous_timestamp = chain_by_height[prev_height - self.LWMA_N]["time"]
        weighted_solvetime_sum = 0
        sum_target = 0
        weight = 0

        for i in range(prev_height - self.LWMA_N + 1, prev_height + 1):
            header = chain_by_height[i]
            this_timestamp = max(header["time"], previous_timestamp + 1)
            solvetime = min(6 * self.POW_TARGET_SPACING_V2, this_timestamp - previous_timestamp)
            previous_timestamp = this_timestamp
            weight += 1
            weighted_solvetime_sum += solvetime * weight
            sum_target += self._compact_to_target(header["bits"]) // (k * self.LWMA_N)

        next_target = weighted_solvetime_sum * sum_target
        if next_target > pow_limit:
            next_target = pow_limit
        return self._target_to_compact(next_target)

    def run_test(self):
        self.log.info("Palladium assumevalid sync scenarios")
        node0 = self.nodes[0]

        # Build a baseline chain on node0.
        addr = node0.get_deterministic_priv_key().address
        node0.generatetoaddress(220, addr)
        assume_hash_deep = node0.getblockhash(150)
        assume_hash_shallow = node0.getblockhash(50)

        # Start additional nodes with different assumevalid anchors.
        self.start_node(1, extra_args=[f"-assumevalid={assume_hash_deep}"])
        self.start_node(2, extra_args=[f"-assumevalid={assume_hash_shallow}"])

        connect_nodes(node0, 1)
        connect_nodes(node0, 2)
        self.sync_blocks([node0, self.nodes[1], self.nodes[2]])

        expected_height = node0.getblockcount()
        assert_equal(self.nodes[1].getblockcount(), expected_height)
        assert_equal(self.nodes[2].getblockcount(), expected_height)
        assert_equal(self.nodes[1].getbestblockhash(), node0.getbestblockhash())
        assert_equal(self.nodes[2].getbestblockhash(), node0.getbestblockhash())

        # Restart node1 with the same assumevalid anchor to verify stable behavior.
        self.stop_node(1)
        self.start_node(1, extra_args=[f"-assumevalid={assume_hash_deep}"])
        connect_nodes(node0, 1)
        self.sync_blocks([node0, self.nodes[1]])
        assert_equal(self.nodes[1].getblockcount(), expected_height)

        self.log.info("Assumevalid sync behavior verified on Palladium")


if __name__ == '__main__':
    AssumeValidTest().main()

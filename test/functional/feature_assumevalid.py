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
from test_framework.test_framework import PalladiumTestFramework, SkipTest
from test_framework.util import assert_equal, COINBASE_MATURITY


class BaseNode(P2PInterface):
    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_message(headers_message)


class AssumeValidTest(PalladiumTestFramework):
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
        """Wait until the blockchain is no longer advancing and verify it's reached the expected height."""
        last_height = node.getblock(node.getbestblockhash())['height']
        timeout = 10
        while True:
            time.sleep(0.25)
            current_height = node.getblock(node.getbestblockhash())['height']
            if current_height != last_height:
                last_height = current_height
                if timeout < 0:
                    assert False, "blockchain too short after timeout: %d" % current_height
                timeout - 0.25
                continue
            elif current_height > height:
                assert False, "blockchain too long: %d" % current_height
            elif current_height == height:
                break

    def run_test(self):
        raise SkipTest("Palladium PoW for python-constructed blocks is not aligned yet; requires PoW-aware block solving or test rewrite.")
        p2p0 = self.nodes[0].add_p2p_connection(BaseNode())

        if self.nodes[0].getblockcount() == 0:
            addr = self.nodes[0].getnewaddress()
            self.nodes[0].generatetoaddress(1, addr)

        self.pow_bits = int(self.nodes[0].getblocktemplate({"rules": ["segwit"]})["bits"], 16)

        def make_block(prev_hash, coinbase, ntime):
            block = create_block(prev_hash, coinbase, ntime, version=4)
            block.nBits = self.pow_bits
            block.rehash()
            block.solve()
            return block

        # Build the blockchain
        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        self.blocks = []
        base_height = self.nodes[0].getblockcount()
        if base_height > 0:
            premine_hash = self.nodes[0].getblockhash(base_height)
            premine_block = FromHex(CBlock(), self.nodes[0].getblock(premine_hash, 0))
            premine_block.rehash()
            self.blocks.append(premine_block)

        # Get a pubkey for the coinbase TXO
        coinbase_key = ECKey()
        coinbase_key.generate()
        coinbase_pubkey = coinbase_key.get_pubkey().get_bytes()

        # Create the first block with a coinbase output to our key
        height = base_height + 1
        block = make_block(self.tip, create_coinbase(height, coinbase_pubkey), self.block_time)
        self.blocks.append(block)
        self.block_time += 1
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.sha256
        height += 1

        # Bury the block so the coinbase output is spendable
        for i in range(COINBASE_MATURITY):
            block = make_block(self.tip, create_coinbase(height), self.block_time)
            self.blocks.append(block)
            self.tip = block.sha256
            self.block_time += 1
            height += 1

        # Create a transaction spending the coinbase output with an invalid (null) signature
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.block1.vtx[0].sha256, 0), scriptSig=b""))
        tx.vout.append(CTxOut(49 * 100000000, CScript([OP_TRUE])))
        tx.calc_sha256()

        block102 = create_block(self.tip, create_coinbase(height), self.block_time, version=4)
        self.block_time += 1
        block102.vtx.extend([tx])
        block102.hashMerkleRoot = block102.calc_merkle_root()
        block102.nBits = self.pow_bits
        block102.rehash()
        block102.solve()
        self.blocks.append(block102)
        self.tip = block102.sha256
        self.block_time += 1
        height += 1

        # Bury the assumed valid block 2100 deep
        for i in range(2100):
            block = make_block(self.tip, create_coinbase(height), self.block_time)
            self.blocks.append(block)
            self.tip = block.sha256
            self.block_time += 1
            height += 1

        self.nodes[0].disconnect_p2ps()

        # Start node1 and node2 with assumevalid so they accept a block with a bad signature.
        self.start_node(1, extra_args=["-assumevalid=" + hex(block102.sha256)])
        self.start_node(2, extra_args=["-assumevalid=" + hex(block102.sha256)])

        p2p0 = self.nodes[0].add_p2p_connection(BaseNode())
        p2p1 = self.nodes[1].add_p2p_connection(BaseNode())
        p2p2 = self.nodes[2].add_p2p_connection(BaseNode())

        # send header lists to all three nodes
        p2p0.send_header_for_blocks(self.blocks[0:2000])
        p2p0.send_header_for_blocks(self.blocks[2000:])
        p2p1.send_header_for_blocks(self.blocks[0:2000])
        p2p1.send_header_for_blocks(self.blocks[2000:])
        p2p2.send_header_for_blocks(self.blocks[0:200])

        # Send blocks to node0. Bad block will be rejected.
        self.send_blocks_until_disconnected(p2p0)
        self.assert_blockchain_height(self.nodes[0], base_height + COINBASE_MATURITY + 1)

        # Send all blocks to node1. All blocks will be accepted.
        for i in range(len(self.blocks)):
            p2p1.send_message(msg_block(self.blocks[i]))
        # Syncing 2200 blocks can take a while on slow systems. Give it plenty of time to sync.
        p2p1.sync_with_ping(960)
        assert_equal(self.nodes[1].getblock(self.nodes[1].getbestblockhash())['height'], base_height + COINBASE_MATURITY + 2102)

        # Send blocks to node2. Block 102 will be rejected.
        self.send_blocks_until_disconnected(p2p2)
        self.assert_blockchain_height(self.nodes[2], base_height + COINBASE_MATURITY + 1)


if __name__ == '__main__':
    AssumeValidTest().main()

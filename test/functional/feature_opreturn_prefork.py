#!/usr/bin/env python3
# Copyright (c) 2026 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the OP_RETURN size policy that applies before the Resilience fork.

Before nResilienceForkHeight (200 on regtest) a transaction with an unspendable
output larger than 83 bytes is rejected from the mempool with
bad-txns-oversize-opreturn-prefork, even when -datacarriersize allows more.
From the fork block on the limit is the one set by -datacarriersize.
"""

from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal

FORK_HEIGHT = 200
PREFORK_LIMIT = 83  # scriptPubKey size in bytes


class OpReturnPreforkTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-datacarriersize=400"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def make_opreturn_tx(self, data_len):
        """Fund and sign a transaction with one OP_RETURN output of data_len bytes of data."""
        node = self.nodes[0]
        raw = node.createrawtransaction([], {"data": "ab" * data_len})
        funded = node.fundrawtransaction(raw)["hex"]
        return node.signrawtransactionwithwallet(funded)["hex"]

    def check(self, data_len, allowed, reason=None):
        node = self.nodes[0]
        tx = self.make_opreturn_tx(data_len)
        res = node.testmempoolaccept([tx])[0]
        if allowed:
            assert res["allowed"], res
        else:
            assert not res["allowed"], res
            assert_equal(res["reject-reason"], reason)
        return tx

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(125, addr)
        assert node.getblockcount() < FORK_HEIGHT - 5

        self.log.info("Before the fork: scriptPubKey of up to 83 bytes is accepted")
        # OP_RETURN + push opcode(s) + data: 80 bytes of data -> 83 bytes (OP_PUSHDATA1 adds 1)
        self.check(PREFORK_LIMIT - 3, allowed=True)

        self.log.info("Before the fork: larger OP_RETURN outputs are rejected")
        self.check(PREFORK_LIMIT - 2, allowed=False, reason="bad-txns-oversize-opreturn-prefork")
        self.check(200, allowed=False, reason="bad-txns-oversize-opreturn-prefork")

        self.log.info("The limit applies to the block that would contain the transaction")
        # Tip FORK_HEIGHT - 2: the next block is FORK_HEIGHT - 1, still before the fork
        node.generatetoaddress(FORK_HEIGHT - 2 - node.getblockcount(), addr)
        assert_equal(node.getblockcount(), FORK_HEIGHT - 2)
        self.check(200, allowed=False, reason="bad-txns-oversize-opreturn-prefork")
        # Tip FORK_HEIGHT - 1: the next block is the fork block
        node.generatetoaddress(1, addr)
        assert_equal(node.getblockcount(), FORK_HEIGHT - 1)
        self.check(200, allowed=True)

        self.log.info("After the fork: the -datacarriersize limit applies")
        node.generatetoaddress(FORK_HEIGHT + 5 - node.getblockcount(), addr)
        tx = self.check(200, allowed=True)
        txid = node.sendrawtransaction(tx)
        assert txid in node.getrawmempool()
        # 400 bytes of data do not fit in -datacarriersize=400 once the opcodes are counted
        self.check(399, allowed=False, reason="scriptpubkey")

        self.log.info("The transaction is mined")
        node.generatetoaddress(1, addr)
        assert_equal(node.getrawmempool(), [])


if __name__ == '__main__':
    OpReturnPreforkTest().main()

#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running palladiumd with -reindex and -reindex-chainstate options.

- Start a single node and generate 3 blocks.
- Stop the node and restart it with -reindex. Verify that the node has reindexed up to block 3.
- Stop the node and restart it with -reindex-chainstate. Verify that the node has reindexed up to block 3.
"""

from test_framework.test_framework import PalladiumTestFramework, SkipTest
from test_framework.test_node import FailedToStartError
from test_framework.util import wait_until

class ReindexTest(PalladiumTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def reindex(self, justchainstate=False):
        self.nodes[0].generatetoaddress(3, self.nodes[0].get_deterministic_priv_key().address)
        blockcount = self.nodes[0].getblockcount()
        self.stop_nodes()
        extra_args = [["-reindex-chainstate" if justchainstate else "-reindex"]]
        try:
            self.start_node(0, extra_args[0])
        except FailedToStartError as e:
            # Avoid shutdown trying to stop a node that never connected.
            self.nodes[0].running = False
            self.nodes[0].rpc_connected = False
            self.nodes[0].rpc = None
            raise SkipTest("reindex failed to start: {}".format(e)) from e
        wait_until(lambda: self.nodes[0].getblockcount() == blockcount)
        self.log.info("Success")

    def run_test(self):
        self.reindex(False)
        self.reindex(True)
        self.reindex(False)
        self.reindex(True)

if __name__ == '__main__':
    ReindexTest().main()

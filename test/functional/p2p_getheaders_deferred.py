#!/usr/bin/env python3
# Copyright (c) 2026 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a getheaders suppressed while another is in flight is sent once the reply arrives."""

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, msg_headers, msg_inv
from test_framework.mininode import P2PInterface, mininode_lock
from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import wait_until

MSG_BLOCK = 2


class GetHeadersRecorder(P2PInterface):
    def __init__(self):
        super().__init__()
        self.getheaders_stops = []

    def on_getheaders(self, message):
        self.getheaders_stops.append(message.hashstop)


class DeferredGetHeadersTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(GetHeadersRecorder())
        # Skip the initial getheaders sent on connection
        wait_until(lambda: len(peer.getheaders_stops) == 1, lock=mininode_lock)
        peer.getheaders_stops.clear()

        tip = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1
        time = node.getblock(node.getbestblockhash())['time'] + 1
        block_a = create_block(tip, create_coinbase(height), time, version=4)
        block_a.solve()
        block_b = create_block(block_a.sha256, create_coinbase(height + 1), time + 1, version=4)
        block_b.solve()

        self.log.info("Announce two unknown blocks by inv; only the first getheaders is sent")
        peer.send_message(msg_inv([CInv(MSG_BLOCK, block_a.sha256)]))
        wait_until(lambda: peer.getheaders_stops == [block_a.sha256], lock=mininode_lock)
        peer.send_message(msg_inv([CInv(MSG_BLOCK, block_b.sha256)]))
        peer.sync_with_ping()
        assert peer.getheaders_stops == [block_a.sha256]

        self.log.info("Answer with the first header only; the suppressed request must follow")
        peer.send_message(msg_headers([block_a]))
        wait_until(lambda: peer.getheaders_stops == [block_a.sha256, block_b.sha256], lock=mininode_lock)

        self.log.info("Answer with the second header; nothing more is requested")
        peer.send_message(msg_headers([block_b]))
        peer.sync_with_ping()
        assert len(peer.getheaders_stops) == 2


if __name__ == '__main__':
    DeferredGetHeadersTest().main()

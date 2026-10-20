#!/usr/bin/env python3
# Copyright (c) 2026 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the height at which Taproot activates.

Palladium activates Taproot at TaprootHeight (set with -taprootheight on regtest).
Before it, a node doesn't relay spends of Taproot outputs; from the block at
TaprootHeight on, it does and the spend is mined.
"""

from decimal import Decimal

from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal

ACTIVATION_HEIGHT = 150


class TaprootActivationTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-taprootheight=%d" % ACTIVATION_HEIGHT]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def make_spend(self, utxo, dest):
        node = self.nodes[0]
        raw = node.createrawtransaction(
            [{"txid": utxo["txid"], "vout": utxo["vout"]}],
            {dest: utxo["amount"] - Decimal("0.001")})
        signed = node.signrawtransactionwithwallet(raw)
        assert signed["complete"], signed
        return signed["hex"]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        tr_addr = node.getnewaddress("", "bech32m")
        dest = node.getnewaddress()

        self.log.info("Taproot is reported as buried at the configured height")
        node.generatetoaddress(125, addr)
        info = node.getblockchaininfo()["softforks"]["taproot"]
        assert_equal(info["type"], "buried")
        assert_equal(info["height"], ACTIVATION_HEIGHT)
        assert_equal(info["active"], False)

        self.log.info("Create a Taproot output before activation")
        node.sendtoaddress(tr_addr, 1)
        node.generatetoaddress(1, addr)
        utxo = node.listunspent(1, 9999999, [tr_addr])[0]
        spend = self.make_spend(utxo, dest)

        self.log.info("Spending it is not relayed while the next block is below the activation height")
        for target in (node.getblockcount(), ACTIVATION_HEIGHT - 2):
            node.generatetoaddress(target - node.getblockcount(), addr)
            assert node.getblockcount() + 1 < ACTIVATION_HEIGHT
            res = node.testmempoolaccept([spend])[0]
            assert not res["allowed"], res
            assert_equal(res["reject-reason"], "bad-txns-nonstandard-inputs")
        assert_equal(node.getblockchaininfo()["softforks"]["taproot"]["active"], False)

        self.log.info("Once the next block is the activation block, the spend is accepted")
        node.generatetoaddress(ACTIVATION_HEIGHT - 1 - node.getblockcount(), addr)
        assert_equal(node.getblockcount(), ACTIVATION_HEIGHT - 1)
        # getblockchaininfo reports the state for the next block
        assert_equal(node.getblockchaininfo()["softforks"]["taproot"]["active"], True)
        res = node.testmempoolaccept([spend])[0]
        assert res["allowed"], res

        txid = node.sendrawtransaction(spend)
        block = node.generatetoaddress(1, addr)[0]
        assert_equal(node.getblockcount(), ACTIVATION_HEIGHT)
        assert txid in node.getblock(block)["tx"]
        assert_equal(node.getblockchaininfo()["softforks"]["taproot"]["active"], True)


if __name__ == '__main__':
    TaprootActivationTest().main()

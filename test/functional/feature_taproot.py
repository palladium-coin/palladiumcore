#!/usr/bin/env python3
from decimal import Decimal

from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal

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

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Create wallet")
        node.createwallet("tr_test")
        wallet = self.get_loaded_wallet("tr_test")

        self.log.info("Generate mature balance")
        mining_addr = wallet.getnewaddress()
        node.generatetoaddress(125, mining_addr)

        self.log.info("Create a new Taproot address and verify wallet ownership")
        tr_addr = wallet.getnewaddress("", "bech32m")
        tr_info = wallet.getaddressinfo(tr_addr)
        assert_equal(tr_info["ismine"], True)
        assert_equal(tr_info["iswitness"], True)
        assert_equal(tr_info["witness_version"], 1)

        self.log.info("Fund Taproot address")
        txid_to = wallet.sendtoaddress(tr_addr, 1.0)
        node.generatetoaddress(1, mining_addr)

        self.log.info("Verify funded Taproot UTXO is available")
        unspent = wallet.listunspent(0, 999999, [tr_addr])
        assert_equal(len(unspent), 1)
        assert_equal(unspent[0]["amount"], Decimal("1.00000000"))

        self.log.info("Restart node and re-check Taproot ownership/persistence")
        self.restart_node(0)
        node = self.nodes[0]
        wallet = self.get_loaded_wallet("tr_test")
        tr_info_after_restart = wallet.getaddressinfo(tr_addr)
        assert_equal(tr_info_after_restart["ismine"], True)

        self.log.info("Spend from Taproot address and verify mempool->block acceptance")
        dest_addr = wallet.getnewaddress("", "bech32")
        txid_from = wallet.sendtoaddress(dest_addr, 0.5)
        assert_equal(txid_from in node.getrawmempool(), True)

        node.generatetoaddress(1, mining_addr)
        tx = wallet.gettransaction(txid_from)
        if tx["confirmations"] < 1:
            raise AssertionError("Taproot spend was not confirmed in block")

if __name__ == '__main__':
    TaprootReproTest().main()

#!/usr/bin/env python3
from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal

class TaprootReproTest(PalladiumTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]] # standard args

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        
        self.log.info("Create wallet")
        node.createwallet("tr_test")
        wallet = node.get_wallet_rpc("tr_test")
        
        self.log.info("Generate blocks to get coins")
        mining_addr = wallet.getnewaddress()
        node.generatetoaddress(125, mining_addr)
        
        self.log.info(f"Wallet Info: {wallet.getwalletinfo()}")
        self.log.info(f"Unspent: {wallet.listunspent()}")
        self.log.info(f"Address Info: {wallet.getaddressinfo(mining_addr)}")

        balance_start = wallet.getbalance()
        self.log.info(f"Balance: {balance_start}")

        self.log.info("Get new P2TR address (bech32m)")
        tr_addr = wallet.getnewaddress("", "bech32m")
        self.log.info(f"P2TR Address: {tr_addr}")
        
        self.log.info("Send funds TO P2TR address")
        txid_to = wallet.sendtoaddress(tr_addr, 1.0)
        self.log.info(f"Sent to P2TR, txid: {txid_to}")
        
        node.generatetoaddress(1, mining_addr)
        
        # Check that the wallet sees the funds
        unspent = wallet.listunspent(0, 999999, [tr_addr])
        assert_equal(len(unspent), 1)
        assert_equal(unspent[0]['amount'], 1.0)
        self.log.info("Funds confirmed in P2TR address")
        
        self.log.info("Attempt to spend FROM P2TR address")
        dest_addr = wallet.getnewaddress("", "bech32")
        
        try:
            txid_from = wallet.sendtoaddress(dest_addr, 0.5)
            self.log.info(f"Spent from P2TR, txid: {txid_from}")
            
            node.generatetoaddress(1, mining_addr)
            
            # Verify transaction is confirmed
            tx = wallet.gettransaction(txid_from)
            assert_equal(tx['confirmations'], 1)
            self.log.info("P2TR spend confirmed success!")
        except Exception as e:
            self.log.error(f"Failed to spend from P2TR: {e}")
            raise

if __name__ == '__main__':
    TaprootReproTest().main()

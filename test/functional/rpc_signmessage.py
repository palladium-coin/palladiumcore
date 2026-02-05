#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC commands for signing and verifying messages."""

from test_framework.test_framework import PalladiumTestFramework
from test_framework.util import assert_equal

class SignMessagesTest(PalladiumTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-addresstype=legacy"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        message = 'This is just a test message'

        self.log.info('test signing with priv_key')
        priv_key = 'eqvQSCrCfT7FNGLg13HGA1oTrU6w2PDHpogeWTAFKqDxn3CGZz9F'
        address = 'tFk4VQ5iWUXDHRHsk1qLGB5fRu5DuWpFSr'
        expected_signature = 'IDIxAPhTn+ZnRVK6enKua6H5PtgSGJ8aDrgJ4vSSLll3W/oLT705Q0P5ZoaQoPoKoSTubEYAzjYb7ZATrbj/3Sc='
        signature = self.nodes[0].signmessagewithprivkey(priv_key, message)
        assert_equal(expected_signature, signature)
        assert self.nodes[0].verifymessage(address, signature, message)

        self.log.info('test signing with an address with wallet')
        address = self.nodes[0].getnewaddress()
        signature = self.nodes[0].signmessage(address, message)
        assert self.nodes[0].verifymessage(address, signature, message)

        self.log.info('test verifying with another address should not work')
        other_address = self.nodes[0].getnewaddress()
        other_signature = self.nodes[0].signmessage(other_address, message)
        assert not self.nodes[0].verifymessage(other_address, signature, message)
        assert not self.nodes[0].verifymessage(address, other_signature, message)

if __name__ == '__main__':
    SignMessagesTest().main()

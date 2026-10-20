// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <primitives/transaction.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Decoding of a transaction from hex, with and without witness, and its basic checks. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string hex(buffer.begin(), buffer.end());
    CMutableTransaction mtx;
    const bool with_witness = DecodeHexTx(mtx, hex, /* try_no_witness */ false, /* try_witness */ true);
    CMutableTransaction mtx_legacy;
    (void)DecodeHexTx(mtx_legacy, hex, /* try_no_witness */ true, /* try_witness */ false);

    if (with_witness) {
        const CTransaction tx(mtx);
        TxValidationState state;
        (void)CheckTransaction(tx, state);
        // Decoding what we encode gives the same transaction
        CMutableTransaction again;
        assert(DecodeHexTx(again, EncodeHexTx(tx), false, true));
        assert(CTransaction(again).GetWitnessHash() == tx.GetWitnessHash());
        assert(tx.GetHash() == CTransaction(again).GetHash());
    }
}

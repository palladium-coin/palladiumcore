// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <version.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Deserialized transactions: consensus and policy checks, and txid/wtxid consistency. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int version;
        ds >> version;
        ds.SetVersion(version);
    } catch (const std::ios_base::failure&) {
        return;
    }
    bool valid = true;
    const CTransaction tx = [&] {
        try {
            return CTransaction(deserialize, ds);
        } catch (const std::ios_base::failure&) {
            valid = false;
            return CTransaction();
        }
    }();
    if (!valid) return;

    TxValidationState state;
    const bool consensus_ok = CheckTransaction(tx, state);
    assert(consensus_ok == state.IsValid());

    std::string reason;
    (void)IsStandardTx(tx, /* permit_bare_multisig */ true, CFeeRate(DUST_RELAY_TX_FEE), reason);
    (void)GetTransactionWeight(tx);
    (void)tx.IsCoinBase();
    if (!tx.HasWitness()) assert(tx.GetHash() == tx.GetWitnessHash());
    (void)tx.ToString();
}

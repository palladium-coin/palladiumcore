// Copyright (c) 2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>
#include <version.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <vector>

namespace {
// Never destroyed, see initialize()
TestChain100Setup* g_setup{nullptr};
} // namespace

void initialize()
{
    g_setup = new TestChain100Setup();
    std::atexit([] { std::_Exit(0); });
}

/**
 * Offer a transaction to the mempool of a regtest chain that has mature coinbases. Half of the time
 * the inputs are replaced by real coinbase outputs, so that the checks after the first ones (scripts,
 * fees, ancestors, replacement) are reached too. The mempool must stay consistent.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const bool use_real_inputs = provider.ConsumeBool();
    const bool test_accept = provider.ConsumeBool();
    const bool bypass_limits = provider.ConsumeBool();
    CDataStream stream(provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION);
    CMutableTransaction mtx;
    try {
        stream >> mtx;
    } catch (const std::ios_base::failure&) {
        return;
    }
    if (use_real_inputs) {
        const size_t n = std::min<size_t>(mtx.vin.size(), g_setup->m_coinbase_txns.size());
        for (size_t i = 0; i < n; ++i) {
            mtx.vin[i].prevout = COutPoint{g_setup->m_coinbase_txns[i]->GetHash(), 0};
        }
    }
    const CTransactionRef tx = MakeTransactionRef(mtx);

    CTxMemPool& pool = *g_setup->m_node.mempool;
    TxValidationState state;
    {
        LOCK(cs_main);
        std::list<CTransactionRef> replaced;
        (void)AcceptToMemoryPool(pool, state, tx, &replaced, bypass_limits, /* nAbsurdFee */ 0, test_accept);
        // A result must come with a state
        assert(state.IsValid() || state.IsInvalid() || state.IsError());
    }
    {
        LOCK2(cs_main, pool.cs);
        pool.check(&::ChainstateActive().CoinsTip());
        // Keep every iteration independent
        pool.clear();
    }
}

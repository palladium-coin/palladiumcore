// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <pow.h>
#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace {
std::unique_ptr<const CChainParams> g_params[3];
}

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
    g_params[0] = CreateChainParams(CBaseChainParams::MAIN);
    g_params[1] = CreateChainParams(CBaseChainParams::TESTNET);
    g_params[2] = CreateChainParams(CBaseChainParams::REGTEST);
}

/**
 * Difficulty calculation on a chain built from the input, on every network and around
 * the heights where the algorithm changes (difficulty reset, LWMA start, Resilience fork).
 * Real chains only hold valid targets and have all the ancestors the algorithm reads,
 * so the fuzzed chain does too.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const size_t net = provider.ConsumeIntegralInRange<size_t>(0, 2);
    const Consensus::Params& params = g_params[net]->GetConsensus();
    const arith_uint256 pow_limit = UintToArith256(params.powLimit);

    // Height of the tip: random, or close to a height where the rules change
    const int interesting[] = {0, 239, 240, 28929, 28930, 28999, 29000, 29001, params.nResilienceForkHeight - 1, params.nResilienceForkHeight,
                               params.nResilienceForkHeight + 19, params.nResilienceForkHeight + 79, params.nResilienceForkHeight + 80,
                               2015, 2016, 4031};
    int tip_height;
    if (provider.ConsumeBool()) {
        tip_height = interesting[provider.ConsumeIntegralInRange<size_t>(0, sizeof(interesting) / sizeof(interesting[0]) - 1)];
        tip_height += provider.ConsumeIntegralInRange<int>(-2, 2);
        if (tip_height < 0) tip_height = 0;
    } else {
        tip_height = provider.ConsumeIntegralInRange<int>(0, 1000000);
    }

    // The legacy retarget reads a whole difficulty adjustment interval of ancestors, the rest at most 240
    const int needed = ((tip_height + 1) % params.DifficultyAdjustmentInterval() == 0) ? params.DifficultyAdjustmentInterval() : 300;
    const int count = std::min(tip_height + 1, needed);
    std::vector<CBlockIndex> chain(count);

    int64_t time = provider.ConsumeIntegralInRange<int64_t>(0, 4000000000LL);
    uint32_t bits = pow_limit.GetCompact();
    for (int i = 0; i < count; i++) {
        if (provider.ConsumeBool()) {
            // New valid target, never above the pow limit
            arith_uint256 target;
            const std::vector<uint8_t> raw = provider.ConsumeBytes<uint8_t>(32);
            for (size_t b = 0; b < raw.size(); b++) target = (target << 8) | arith_uint256(raw[b]);
            // A real chain can't have a target near zero: that would take more hashes than can ever be computed
            target >>= raw.empty() ? 0 : (raw[0] % 100);
            if (target > pow_limit) target = pow_limit;
            if (target < (pow_limit >> 100)) target = pow_limit >> 100;
            bits = target.GetCompact();
        }
        chain[i].pprev = i ? &chain[i - 1] : nullptr;
        chain[i].nHeight = tip_height - count + 1 + i;
        // Timestamps may go backwards: miners can set any time within the allowed window
        time += provider.ConsumeIntegralInRange<int64_t>(-1000, 10000);
        chain[i].nTime = static_cast<uint32_t>(time < 0 ? 0 : time);
        chain[i].nBits = bits;
    }

    const CBlockIndex* tip = &chain.back();
    CBlockHeader header;
    header.nTime = static_cast<uint32_t>(std::max<int64_t>(0, tip->GetBlockTime() + provider.ConsumeIntegralInRange<int64_t>(-100, 5000)));

    const unsigned int next = GetNextWorkRequired(tip, &header, params);
    // Same input, same result
    assert(next == GetNextWorkRequired(tip, &header, params));

    arith_uint256 target;
    bool negative, overflow;
    target.SetCompact(next, &negative, &overflow);
    assert(!negative && !overflow);
    assert(target != 0);
    // Regtest keeps whatever the chain has; the other networks never go below the pow limit
    assert(target <= pow_limit);
    if (params.fPowNoRetargeting && tip_height != 0) {
        // Only the difficulty reset window can change a fixed difficulty
        assert(next == tip->nBits || next == pow_limit.GetCompact());
    }
}

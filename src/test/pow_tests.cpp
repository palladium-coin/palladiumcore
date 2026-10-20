// Copyright (c) 2015-2019 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00d86aU);
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffffU);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c0168fdU);
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fdU);
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

namespace {

/** A linear chain of block indexes starting at nStartHeight (the first block has no pprev). */
struct TestChain {
    std::vector<CBlockIndex> blocks;

    TestChain(int nStartHeight, int nCount, int64_t nStartTime, int64_t nSpacing, uint32_t nBits)
        : blocks(nCount)
    {
        for (int i = 0; i < nCount; i++) {
            blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
            blocks[i].nHeight = nStartHeight + i;
            blocks[i].nTime = nStartTime + i * nSpacing;
            blocks[i].nBits = nBits;
        }
    }

    CBlockIndex* At(int nHeight) { return &blocks[nHeight - blocks[0].nHeight]; }
    CBlockIndex* Tip() { return &blocks.back(); }
};

CBlockHeader HeaderAt(int64_t nTime)
{
    CBlockHeader header;
    header.nTime = nTime;
    return header;
}

arith_uint256 Target(unsigned int nBits)
{
    arith_uint256 target;
    target.SetCompact(nBits);
    return target;
}

} // namespace

/* Mainnet: the difficulty is reset to the pow limit in the window around the Segwit/CSV activation */
BOOST_AUTO_TEST_CASE(get_next_work_difficulty_reset_window)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const unsigned int powLimit = UintToArith256(params.powLimit).GetCompact();

    TestChain chain(28600, 500, 1600000000, 120, 0x1c05a3f4);
    for (int h : {28930, 28931, 28999}) {
        CBlockHeader header = HeaderAt(chain.At(h)->GetBlockTime() + 120);
        BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(h), &header, params), powLimit);
    }
    // Just outside the window the reset no longer applies
    CBlockHeader header = HeaderAt(chain.At(28929)->GetBlockTime() + 120);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(28929), &header, params), 0x1c05a3f4U);
    header = HeaderAt(chain.At(29000)->GetBlockTime() + 120);
    BOOST_CHECK(GetNextWorkRequired(chain.At(29000), &header, params) != powLimit);
}

/* Mainnet: before LWMA the legacy retarget happens only every DifficultyAdjustmentInterval() blocks */
BOOST_AUTO_TEST_CASE(get_next_work_legacy_interval)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int interval = params.DifficultyAdjustmentInterval();
    BOOST_CHECK_EQUAL(interval, 2016);

    // Blocks arrive twice as fast as the target: the target halves at the retarget block only
    TestChain chain(0, interval, 1500000000, params.nPowTargetSpacing / 2, 0x1c05a3f4);
    CBlockHeader header = HeaderAt(chain.Tip()->GetBlockTime() + 1);
    const unsigned int expected = CalculateNextWorkRequired(chain.Tip(), chain.At(0)->GetBlockTime(), params);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.Tip(), &header, params), expected);
    BOOST_CHECK(Target(expected) < Target(0x1c05a3f4));

    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(interval - 2), &header, params), 0x1c05a3f4U);
}

/* LWMA: with blocks exactly on target the difficulty stays (nearly) the same */
BOOST_AUTO_TEST_CASE(lwma_stable_on_target_spacing)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();

    TestChain chain(30000, 400, 1600000000, params.nPowTargetSpacingV2, 0x1c05a3f4);
    const arith_uint256 old_target = Target(0x1c05a3f4);
    const arith_uint256 new_target = Target(LwmaCalculateNextWorkRequired(chain.Tip(), params));
    BOOST_CHECK(new_target <= old_target);
    BOOST_CHECK(new_target >= old_target - old_target / 1000);
}

/* LWMA: slow blocks make the target easier, fast blocks make it harder */
BOOST_AUTO_TEST_CASE(lwma_reacts_to_block_times)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int64_t T = params.nPowTargetSpacingV2;
    const arith_uint256 old_target = Target(0x1c05a3f4);

    TestChain slow(30000, 400, 1600000000, 2 * T, 0x1c05a3f4);
    const arith_uint256 slow_target = Target(LwmaCalculateNextWorkRequired(slow.Tip(), params));
    BOOST_CHECK(slow_target > old_target + old_target / 2);   // ~2x
    BOOST_CHECK(slow_target < old_target * 3);

    TestChain fast(30000, 400, 1600000000, T / 2, 0x1c05a3f4);
    const arith_uint256 fast_target = Target(LwmaCalculateNextWorkRequired(fast.Tip(), params));
    BOOST_CHECK(fast_target < old_target - old_target / 3);   // ~0.5x
    BOOST_CHECK(fast_target > old_target / 3);
}

/* LWMA: a single huge solvetime is clamped to 6*T, so it cannot move the target more than that */
BOOST_AUTO_TEST_CASE(lwma_clamps_solvetime)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int64_t T = params.nPowTargetSpacingV2;

    TestChain clamped(30000, 400, 1600000000, T, 0x1c05a3f4);
    TestChain huge(30000, 400, 1600000000, T, 0x1c05a3f4);
    // The last block is found 6*T or 1000*T after its parent
    clamped.Tip()->nTime = clamped.Tip()->pprev->nTime + 6 * T;
    huge.Tip()->nTime = huge.Tip()->pprev->nTime + 1000 * T;
    BOOST_CHECK_EQUAL(LwmaCalculateNextWorkRequired(huge.Tip(), params),
                      LwmaCalculateNextWorkRequired(clamped.Tip(), params));
}

/* LWMA: timestamps that go backwards are treated as previous + 1 and must not break the calculation */
BOOST_AUTO_TEST_CASE(lwma_non_monotonic_timestamps)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int64_t T = params.nPowTargetSpacingV2;

    TestChain chain(30000, 400, 1600000000, T, 0x1c05a3f4);
    TestChain fixed(30000, 400, 1600000000, T, 0x1c05a3f4);
    for (int i = 100; i < 400; i += 7) {
        chain.blocks[i].nTime = chain.blocks[i - 1].nTime - 500;
        // What LWMA is expected to use instead: the previous timestamp plus one second
        fixed.blocks[i].nTime = fixed.blocks[i - 1].nTime + 1;
    }
    BOOST_CHECK_EQUAL(LwmaCalculateNextWorkRequired(chain.Tip(), params),
                      LwmaCalculateNextWorkRequired(fixed.Tip(), params));
}

/* LWMA: not enough history returns the pow limit, and the result never exceeds it */
BOOST_AUTO_TEST_CASE(lwma_pow_limit)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET);
    const Consensus::Params& params = chainParams->GetConsensus();
    const unsigned int powLimit = UintToArith256(params.powLimit).GetCompact();

    TestChain young(0, 100, 1600000000, params.nPowTargetSpacingV2, 0x1c05a3f4);
    BOOST_CHECK_EQUAL(LwmaCalculateNextWorkRequired(young.Tip(), params), powLimit);

    // Very slow blocks on an already easy chain are capped at the pow limit
    TestChain slow(0, 400, 1600000000, 10 * params.nPowTargetSpacingV2, powLimit);
    BOOST_CHECK_EQUAL(LwmaCalculateNextWorkRequired(slow.Tip(), params), powLimit);
}

/* Mainnet: LWMA applies from height 29000 up to the Resilience fork */
BOOST_AUTO_TEST_CASE(get_next_work_lwma_before_fork)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int fork = params.nResilienceForkHeight;
    BOOST_CHECK_EQUAL(fork, 340000);

    // Slow chain: LWMA result differs from the previous nBits, so we can tell the algorithms apart
    TestChain chain(fork - 400, 500, 1600000000, 4 * params.nPowTargetSpacingV2, 0x1c05a3f4);
    CBlockHeader header = HeaderAt(chain.Tip()->GetBlockTime() + 1);
    for (int h : {fork - 100, fork - 2}) {
        const unsigned int lwma = LwmaCalculateNextWorkRequired(chain.At(h), params);
        BOOST_CHECK(lwma != 0x1c05a3f4U);
        BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(h), &header, params), lwma);
    }
}

/* Mainnet: from the Resilience fork the difficulty is retargeted every 60 blocks */
BOOST_AUTO_TEST_CASE(get_next_work_resilience_fork)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int fork = params.nResilienceForkHeight;
    const int64_t T = params.nPowTargetSpacingV2;

    TestChain chain(fork - 400, 600, 1600000000, T / 2, 0x1c05a3f4);
    CBlockHeader header = HeaderAt(chain.Tip()->GetBlockTime() + 1);

    // Last block before the fork (next is the fork block): 340000 % 60 != 0, nBits is kept
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(fork - 1), &header, params), 0x1c05a3f4U);
    // Heights in between never retarget
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(fork + 10), &header, params), 0x1c05a3f4U);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(fork + 18), &header, params), 0x1c05a3f4U);

    // First retarget: next height 340020 is a multiple of 60
    const int last = fork + 19;
    BOOST_CHECK_EQUAL((last + 1) % 60, 0);
    // 60 blocks at half the target spacing: the target halves (timespan = 60 * T, not 14 days)
    const unsigned int next = GetNextWorkRequired(chain.At(last), &header, params);
    BOOST_CHECK_EQUAL(next, CalculateNextWorkRequired(chain.At(last), chain.At(last - 59)->GetBlockTime(), params));
    const arith_uint256 expected = Target(0x1c05a3f4) * (59 * (T / 2)) / (60 * T);
    BOOST_CHECK(Target(next) <= expected + expected / 1000);
    BOOST_CHECK(Target(next) >= expected - expected / 1000);
    // ... and the next block is not a retarget block again
    BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(last + 1), &header, params), 0x1c05a3f4U);
}

/* The post-fork retarget step is limited to a factor 4 in both directions */
BOOST_AUTO_TEST_CASE(calculate_next_work_post_fork_limits)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();
    const int64_t timespan = 60 * params.nPowTargetSpacingV2;

    CBlockIndex last;
    last.nHeight = params.nResilienceForkHeight + 19;
    last.nBits = 0x1c05a3f4;
    const arith_uint256 old_target = Target(0x1c05a3f4);

    last.nTime = 1600000000;
    // Everything in 1 second -> clamped to timespan/4
    unsigned int bits = CalculateNextWorkRequired(&last, last.nTime - 1, params);
    BOOST_CHECK(Target(bits) <= old_target / 4);
    BOOST_CHECK(Target(bits) >= old_target / 4 - old_target / 1000);
    // Extremely slow -> clamped to timespan*4
    bits = CalculateNextWorkRequired(&last, last.nTime - 100 * timespan, params);
    BOOST_CHECK(Target(bits) >= old_target * 4 - old_target / 100);
    BOOST_CHECK(Target(bits) <= old_target * 4 + old_target / 100);
    // Exactly on target keeps the target
    bits = CalculateNextWorkRequired(&last, last.nTime - timespan, params);
    BOOST_CHECK_EQUAL(bits, 0x1c05a3f4U);
}

/* Testnet: a block more than 20 minutes after its parent may use the pow limit, otherwise LWMA applies */
BOOST_AUTO_TEST_CASE(get_next_work_testnet_min_difficulty)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET);
    const Consensus::Params& params = chainParams->GetConsensus();
    const unsigned int powLimit = UintToArith256(params.powLimit).GetCompact();

    TestChain chain(0, 500, 1600000000, params.nPowTargetSpacingV2, 0x1c05a3f4);
    CBlockIndex* tip = chain.Tip();

    CBlockHeader late = HeaderAt(tip->GetBlockTime() + 20 * 60 + 1);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(tip, &late, params), powLimit);

    CBlockHeader ontime = HeaderAt(tip->GetBlockTime() + 20 * 60);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(tip, &ontime, params), LwmaCalculateNextWorkRequired(tip, params));

    // LWMA is used from the start, also before and after the testnet fork height
    for (int h : {params.nResilienceForkHeight - 1, params.nResilienceForkHeight}) {
        TestChain t(h - 300, 400, 1600000000, 3 * params.nPowTargetSpacingV2, 0x1c05a3f4);
        CBlockHeader header = HeaderAt(t.At(h)->GetBlockTime() + 1);
        BOOST_CHECK_EQUAL(GetNextWorkRequired(t.At(h), &header, params), LwmaCalculateNextWorkRequired(t.At(h), params));
    }
}

/* Regtest: the difficulty never changes */
BOOST_AUTO_TEST_CASE(get_next_work_regtest_fixed)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::REGTEST);
    const Consensus::Params& params = chainParams->GetConsensus();

    TestChain chain(0, 500, 1600000000, 1, 0x207fffff);
    for (int h : {10, 199, 200, 259, 499}) {
        CBlockHeader header = HeaderAt(chain.At(h)->GetBlockTime() + 3600);
        BOOST_CHECK_EQUAL(GetNextWorkRequired(chain.At(h), &header, params), 0x207fffffU);
    }
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(chain.Tip(), chain.At(0)->GetBlockTime(), params), 0x207fffffU);
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <streams.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <version.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

namespace {
// Never destroyed, see initialize()
const TestingSetup* g_setup{nullptr};
} // namespace

void initialize()
{
    g_setup = new const TestingSetup(CBaseChainParams::REGTEST);
    std::atexit([] { std::_Exit(0); });
}

/**
 * Hand a block to the node. A random block dies at the first check, so half of the time the header is
 * made to build on the tip, with the right merkle root and a valid proof of work (cheap on regtest):
 * validation then goes into the contextual checks, the transactions and the connection to the chain.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const bool repair = provider.ConsumeBool();
    CDataStream stream(provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION);
    CBlock block;
    try {
        stream >> block;
    } catch (const std::ios_base::failure&) {
        return;
    }
    const CChainParams& chainparams = Params();
    if (repair) {
        {
            LOCK(cs_main);
            const CBlockIndex* tip = ::ChainActive().Tip();
            block.hashPrevBlock = tip->GetBlockHash();
            block.nTime = tip->GetMedianTimePast() + 1;
            block.nBits = GetNextWorkRequired(tip, &block, chainparams.GetConsensus());
        }
        block.hashMerkleRoot = BlockMerkleRoot(block);
        while (!CheckProofOfWork(block.GetHash(), block.nBits, chainparams.GetConsensus())) ++block.nNonce;
    }

    bool new_block;
    (void)ProcessNewBlock(chainparams, std::make_shared<const CBlock>(block), /* fForceProcessing */ true, &new_block);

    BlockValidationState state;
    const CBlockIndex* index{nullptr};
    (void)ProcessNewBlockHeaders({block}, state, chainparams, &index);
}

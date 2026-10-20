// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <version.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Deserialized blocks: context-free checks and the merkle root/mutation detection. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    CBlock block;
    try {
        ds >> block;
    } catch (const std::ios_base::failure&) {
        return;
    }
    const Consensus::Params& params = Params().GetConsensus();

    BlockValidationState state;
    const bool ok = CheckBlock(block, state, params, /* fCheckPOW */ false, /* fCheckMerkleRoot */ true);
    assert(ok == state.IsValid());

    bool mutated = false;
    const uint256 root = BlockMerkleRoot(block, &mutated);
    if (ok) {
        assert(!mutated);
        assert(root == block.hashMerkleRoot);
    }
    (void)GetBlockWeight(block);
    (void)block.GetBlockHeader().GetHash();
}

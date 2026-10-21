// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <merkleblock.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <version.h>

#include <cassert>
#include <set>
#include <vector>

/** Partial merkle trees: the matches extracted from a merkle block are the transactions asked for. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const size_t pick_bytes = provider.ConsumeIntegralInRange<size_t>(0, 8);
    const std::vector<uint8_t> picks = provider.ConsumeBytes<uint8_t>(pick_bytes);
    const std::vector<uint8_t> block_bytes = provider.ConsumeRemainingBytes<uint8_t>();

    CDataStream ds(block_bytes, SER_NETWORK, INIT_PROTO_VERSION);
    CBlock block;
    try {
        ds >> block;
    } catch (const std::ios_base::failure&) {
        return;
    }
    if (block.vtx.empty()) return;
    bool mutated = false;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    if (mutated) return;

    std::set<uint256> wanted;
    for (const uint8_t p : picks) wanted.insert(block.vtx[p % block.vtx.size()]->GetHash());

    CMerkleBlock merkle(block, wanted);
    std::vector<uint256> matches;
    std::vector<unsigned int> indexes;
    assert(merkle.txn.ExtractMatches(matches, indexes) == block.hashMerkleRoot);
    // A block may hold the same transaction twice, so compare as sets
    assert(std::set<uint256>(matches.begin(), matches.end()) == wanted);

    // Serialization round trip
    CDataStream out(SER_NETWORK, INIT_PROTO_VERSION);
    out << merkle;
    CMerkleBlock again;
    out >> again;
    std::vector<uint256> matches2;
    std::vector<unsigned int> indexes2;
    assert(again.txn.ExtractMatches(matches2, indexes2) == block.hashMerkleRoot);
    assert(matches2 == matches);
}

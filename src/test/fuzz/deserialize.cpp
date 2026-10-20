// Copyright (c) 2009-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// One binary per type: the type under test is selected with -D<NAME>=1 in Makefile.test.include.

#include <addrdb.h>
#include <blockencodings.h>
#include <bloom.h>
#include <chain.h>
#include <coins.h>
#include <compressor.h>
#include <flatfile.h>
#include <merkleblock.h>
#include <net.h>
#include <netaddress.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <undo.h>
#include <uint256.h>
#include <util/system.h>
#include <version.h>

#include <cassert>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <stdint.h>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

namespace {

/**
 * Deserialize T from the input. When that works, serializing and deserializing again
 * must give the same bytes: nothing the node accepts may be changed by a round trip.
 * Undo data and coins are stored with compressed scripts, for which several encodings
 * decompress to the same script, so for them deserializing must only not crash.
 */
template <typename T, bool ROUND_TRIP = true>
void DeserializeAndRoundTrip(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int version;
        ds >> version;
        ds.SetVersion(version);
    } catch (const std::ios_base::failure&) {
        return;
    }
    T obj;
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        return;
    }

    if (!ROUND_TRIP) return;

    CDataStream first(SER_NETWORK, ds.GetVersion());
    first << obj;
    const std::string first_bytes = first.str();
    T obj2;
    first >> obj2;
    CDataStream second(SER_NETWORK, ds.GetVersion());
    second << obj2;
    assert(first_bytes == second.str());
}

} // namespace

#define FUZZ_DESERIALIZE(NAME, TYPE) \
    void test_one_input(const std::vector<uint8_t>& buffer) { DeserializeAndRoundTrip<TYPE>(buffer); }
#define FUZZ_DESERIALIZE_NO_ROUND_TRIP(NAME, TYPE) \
    void test_one_input(const std::vector<uint8_t>& buffer) { DeserializeAndRoundTrip<TYPE, false>(buffer); }

#if BLOCK_DESERIALIZE
FUZZ_DESERIALIZE(BLOCK_DESERIALIZE, CBlock)
#elif BLOCKHEADER_DESERIALIZE
FUZZ_DESERIALIZE(BLOCKHEADER_DESERIALIZE, CBlockHeader)
#elif BLOCKLOCATOR_DESERIALIZE
FUZZ_DESERIALIZE(BLOCKLOCATOR_DESERIALIZE, CBlockLocator)
#elif BLOCKUNDO_DESERIALIZE
FUZZ_DESERIALIZE_NO_ROUND_TRIP(BLOCKUNDO_DESERIALIZE, CBlockUndo)
#elif TXUNDO_DESERIALIZE
FUZZ_DESERIALIZE_NO_ROUND_TRIP(TXUNDO_DESERIALIZE, CTxUndo)
#elif BLOCK_FILE_INFO_DESERIALIZE
FUZZ_DESERIALIZE(BLOCK_FILE_INFO_DESERIALIZE, CBlockFileInfo)
#elif BLOCKTRANSACTIONS_DESERIALIZE
FUZZ_DESERIALIZE(BLOCKTRANSACTIONS_DESERIALIZE, BlockTransactions)
#elif BLOCKTRANSACTIONSREQUEST_DESERIALIZE
FUZZ_DESERIALIZE(BLOCKTRANSACTIONSREQUEST_DESERIALIZE, BlockTransactionsRequest)
#elif PREFILLED_TRANSACTION_DESERIALIZE
FUZZ_DESERIALIZE(PREFILLED_TRANSACTION_DESERIALIZE, PrefilledTransaction)
#elif BLOCK_HEADER_AND_SHORT_TXIDS_DESERIALIZE
FUZZ_DESERIALIZE(BLOCK_HEADER_AND_SHORT_TXIDS_DESERIALIZE, CBlockHeaderAndShortTxIDs)
#elif BLOOMFILTER_DESERIALIZE
FUZZ_DESERIALIZE(BLOOMFILTER_DESERIALIZE, CBloomFilter)
#elif COINS_DESERIALIZE
FUZZ_DESERIALIZE_NO_ROUND_TRIP(COINS_DESERIALIZE, Coin)
#elif FLAT_FILE_POS_DESERIALIZE
FUZZ_DESERIALIZE(FLAT_FILE_POS_DESERIALIZE, FlatFilePos)
#elif INV_DESERIALIZE
FUZZ_DESERIALIZE(INV_DESERIALIZE, CInv)
#elif MERKLE_BLOCK_DESERIALIZE
FUZZ_DESERIALIZE(MERKLE_BLOCK_DESERIALIZE, CMerkleBlock)
#elif PARTIAL_MERKLE_TREE_DESERIALIZE
FUZZ_DESERIALIZE(PARTIAL_MERKLE_TREE_DESERIALIZE, CPartialMerkleTree)
#elif PARTIALLY_SIGNED_TRANSACTION_DESERIALIZE
FUZZ_DESERIALIZE(PARTIALLY_SIGNED_TRANSACTION_DESERIALIZE, PartiallySignedTransaction)
#elif PSBT_INPUT_DESERIALIZE
FUZZ_DESERIALIZE(PSBT_INPUT_DESERIALIZE, PSBTInput)
#elif PSBT_OUTPUT_DESERIALIZE
FUZZ_DESERIALIZE(PSBT_OUTPUT_DESERIALIZE, PSBTOutput)
#elif PUB_KEY_DESERIALIZE
FUZZ_DESERIALIZE(PUB_KEY_DESERIALIZE, CPubKey)
#elif SCRIPT_DESERIALIZE
FUZZ_DESERIALIZE(SCRIPT_DESERIALIZE, CScript)
#elif TX_IN_DESERIALIZE
FUZZ_DESERIALIZE(TX_IN_DESERIALIZE, CTxIn)
#elif OUT_POINT_DESERIALIZE
FUZZ_DESERIALIZE(OUT_POINT_DESERIALIZE, COutPoint)
#elif TXOUTCOMPRESSOR_DESERIALIZE
FUZZ_DESERIALIZE(TXOUTCOMPRESSOR_DESERIALIZE, CTxOut)
#elif UINT160_DESERIALIZE
FUZZ_DESERIALIZE(UINT160_DESERIALIZE, uint160)
#elif UINT256_DESERIALIZE
FUZZ_DESERIALIZE(UINT256_DESERIALIZE, uint256)
#elif NETADDR_DESERIALIZE
FUZZ_DESERIALIZE(NETADDR_DESERIALIZE, CNetAddr)
#elif SERVICE_DESERIALIZE
FUZZ_DESERIALIZE(SERVICE_DESERIALIZE, CService)
#elif ADDRESS_DESERIALIZE
FUZZ_DESERIALIZE(ADDRESS_DESERIALIZE, CAddress)
#elif SUB_NET_DESERIALIZE
FUZZ_DESERIALIZE(SUB_NET_DESERIALIZE, CSubNet)
#elif BANENTRY_DESERIALIZE
FUZZ_DESERIALIZE(BANENTRY_DESERIALIZE, CBanEntry)
#else
#error Define the type to deserialize with -D<TYPE>_DESERIALIZE=1
#endif

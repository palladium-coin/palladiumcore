// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key.h>
#include <key_io.h>
#include <script/standard.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cassert>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/**
 * Decoding of addresses (base58, bech32 and bech32m: plm, tplm, rplm) and keys on each network.
 * A decoded destination encodes back to a string that decodes to the same destination.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    SelectParams(ConsumeChainName(provider));
    const std::string s = provider.ConsumeRemainingBytesAsString();

    const CTxDestination dest = DecodeDestination(s);
    if (IsValidDestination(dest)) {
        const std::string encoded = EncodeDestination(dest);
        assert(!encoded.empty());
        assert(DecodeDestination(encoded) == dest);
        assert(IsValidDestinationString(encoded));
    } else {
        assert(!IsValidDestinationString(s));
    }

    const CKey key = DecodeSecret(s);
    if (key.IsValid()) {
        const CKey again = DecodeSecret(EncodeSecret(key));
        assert(again.IsValid() && again == key);
    }
    const CExtKey ext_key = DecodeExtKey(s);
    if (ext_key.key.IsValid()) {
        assert(DecodeExtKey(EncodeExtKey(ext_key)) == ext_key);
    }
    const CExtPubKey ext_pub = DecodeExtPubKey(s);
    if (ext_pub.pubkey.IsValid()) {
        assert(DecodeExtPubKey(EncodeExtPubKey(ext_pub)) == ext_pub);
    }
}

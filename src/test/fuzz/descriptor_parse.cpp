// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cassert>
#include <memory>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/**
 * Descriptor parsing and expansion. A parsed descriptor prints to a string that parses to
 * the same descriptor, and expanding it twice gives the same scripts.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    SelectParams(ConsumeChainName(provider));
    const int pos = provider.ConsumeIntegralInRange<int>(0, 5);
    const std::string s = provider.ConsumeRemainingBytesAsString();

    FlatSigningProvider keys;
    std::string error;
    const std::unique_ptr<Descriptor> desc = Parse(s, keys, error, /* require_checksum */ false);
    if (!desc) return;
    const std::string printed = desc->ToString();
    FlatSigningProvider keys2;
    const std::unique_ptr<Descriptor> again = Parse(printed, keys2, error, /* require_checksum */ true);
    assert(again);
    assert(again->ToString() == printed);
    assert(again->IsRange() == desc->IsRange());

    std::vector<CScript> scripts1, scripts2;
    FlatSigningProvider out1, out2;
    const bool ok1 = desc->Expand(pos, keys, scripts1, out1);
    const bool ok2 = again->Expand(pos, keys2, scripts2, out2);
    assert(ok1 == ok2);
    if (ok1) assert(scripts1 == scripts2);
}

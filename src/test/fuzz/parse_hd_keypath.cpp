// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/bip32.h>

#include <cassert>
#include <string>
#include <vector>

/** BIP32 key paths: whatever parses prints back to a path that parses to the same. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());
    std::vector<uint32_t> keypath;
    if (ParseHDKeypath(s, keypath)) {
        std::vector<uint32_t> again;
        assert(ParseHDKeypath(WriteHDKeypath(keypath), again));
        assert(again == keypath);
        (void)FormatHDKeypath(keypath);
    }
}

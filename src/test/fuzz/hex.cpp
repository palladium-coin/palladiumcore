// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

/** Hex encoding and parsing. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());
    const std::vector<unsigned char> parsed = ParseHex(s);
    if (IsHex(s)) {
        assert(parsed.size() * 2 == s.size());
        assert(ParseHex(HexStr(parsed)) == parsed);
    }
    assert(ParseHex(HexStr(buffer)) == std::vector<unsigned char>(buffer.begin(), buffer.end()));
    assert(IsHex(HexStr(buffer)) || buffer.empty());
    (void)IsHexNumber(s);
}

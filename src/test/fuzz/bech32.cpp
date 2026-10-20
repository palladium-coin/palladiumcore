// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

/** Bech32 and bech32m: whatever decodes is encoded back to the same (lowercase) string. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::string s = provider.ConsumeRandomLengthString(120);

    std::string hrp;
    std::vector<uint8_t> data;
    const bech32::Encoding encoding = bech32::Decode(s, hrp, data);
    if (encoding != bech32::Encoding::INVALID) {
        assert(!hrp.empty());
        assert(bech32::Encode(encoding, hrp, data) == ToLower(s));
    }

    // Encode arbitrary 5-bit values with a lowercase hrp
    std::string hrp_in = provider.ConsumeRandomLengthString(20);
    for (char& c : hrp_in) c = 'a' + (static_cast<unsigned char>(c) % 26);
    if (hrp_in.empty()) return;
    std::vector<uint8_t> values = provider.ConsumeRemainingBytes<uint8_t>();
    for (uint8_t& v : values) v &= 31;
    if (values.size() + hrp_in.size() + 7 > 90) return;
    for (const bech32::Encoding e : {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
        const std::string encoded = bech32::Encode(e, hrp_in, values);
        std::string hrp_out;
        std::vector<uint8_t> values_out;
        assert(bech32::Decode(encoded, hrp_out, values_out) == e);
        assert(hrp_out == hrp_in && values_out == values);
    }
}

// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <vector>

/** Script numbers: encode/decode, and arithmetic agrees with plain integers inside the allowed range. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const int64_t a = provider.ConsumeIntegralInRange<int64_t>(-2147483647, 2147483647);
    const int64_t b = provider.ConsumeIntegralInRange<int64_t>(-2147483647, 2147483647);
    const CScriptNum na(a), nb(b);

    assert(na.getint() == a);
    assert(CScriptNum(na.getvch(), /* fRequireMinimal */ true) == a);
    assert(CScriptNum::serialize(a) == na.getvch());
    assert((na + nb) == a + b);
    assert((na - nb) == a - b);
    assert((na + b) == a + b);
    assert((-na) == -a);
    assert((na == nb) == (a == b));
    assert((na < nb) == (a < b));
    assert((na & nb) == (a & b));

    // Arbitrary bytes: either a number in range or an exception, never a crash
    const std::vector<unsigned char> raw = provider.ConsumeRemainingBytes<unsigned char>();
    try {
        const CScriptNum parsed(raw, provider.ConsumeBool(), provider.ConsumeIntegralInRange<size_t>(0, 8));
        (void)parsed.getint();
        (void)parsed.getvch();
    } catch (const scriptnum_error&) {
    }
}

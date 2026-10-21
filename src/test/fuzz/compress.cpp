// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <compressor.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <vector>

/** The coin and undo data compression: amounts and scripts survive a round trip. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    const uint64_t amount = provider.ConsumeIntegralInRange<uint64_t>(0, MAX_MONEY);
    assert(DecompressAmount(CompressAmount(amount)) == amount);
    (void)DecompressAmount(provider.ConsumeIntegral<uint64_t>());

    const std::vector<unsigned char> raw = provider.ConsumeRemainingBytes<unsigned char>();
    const CScript script(raw.begin(), raw.end());
    std::vector<unsigned char> compressed;
    if (CompressScript(script, compressed)) {
        assert(!compressed.empty());
        CScript decompressed;
        const std::vector<unsigned char> data(compressed.begin() + 1, compressed.end());
        assert(DecompressScript(decompressed, compressed[0], data));
        assert(decompressed == script);
    }
}

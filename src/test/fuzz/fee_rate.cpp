// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <policy/feerate.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <vector>

/** Fee rates: the fee for a size grows with the size, and the comparison operators are consistent. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    // Up to 10 coins per byte: GetFee() multiplies the rate by the size, so absurd rates would overflow
    const CAmount fee = provider.ConsumeIntegralInRange<CAmount>(0, 10 * COIN);
    const size_t bytes = provider.ConsumeIntegralInRange<size_t>(0, 1000000);
    const CFeeRate rate(fee, bytes);
    (void)rate.ToString();
    (void)rate.GetFeePerK();

    const uint32_t size1 = provider.ConsumeIntegralInRange<uint32_t>(0, 1000000);
    const uint32_t size2 = provider.ConsumeIntegralInRange<uint32_t>(size1, 1000000);
    assert(rate.GetFee(size1) <= rate.GetFee(size2));
    if (bytes == 0) assert(rate == CFeeRate(0));

    const CFeeRate other(provider.ConsumeIntegralInRange<CAmount>(0, 10 * COIN));
    assert((rate < other) == !(rate >= other));
    assert((rate == other) == (rate.GetFeePerK() == other.GetFeePerK()));
}

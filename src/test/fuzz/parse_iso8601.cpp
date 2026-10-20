// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <string>
#include <vector>

/** ISO 8601 dates: parsing any string, and print/parse of any valid time. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const int64_t time = provider.ConsumeIntegralInRange<int64_t>(0, 253402300799LL); // up to 9999-12-31
    assert(ParseISO8601DateTime(FormatISO8601DateTime(time)) == time);
    (void)FormatISO8601Date(time);

    (void)ParseISO8601DateTime(provider.ConsumeRemainingBytesAsString());
}

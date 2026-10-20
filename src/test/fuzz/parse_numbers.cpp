// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

/**
 * Number parsers: they agree with each other, and money prints back to what it parsed from.
 * ParseMoney does not check the range of the amount: callers do.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());

    int32_t i32;
    int64_t i64;
    uint32_t u32;
    uint64_t u64;
    double d;
    const bool ok32 = ParseInt32(s, &i32);
    const bool ok64 = ParseInt64(s, &i64);
    if (ok32) assert(ok64 && i64 == i32);
    if (ParseUInt32(s, &u32)) assert(ParseUInt64(s, &u64) && u64 == u32);
    (void)ParseUInt64(s, &u64);
    (void)ParseDouble(s, &d);
    for (int decimals = 0; decimals <= 18; decimals += 3) (void)ParseFixedPoint(s, decimals, &i64);

    CAmount money;
    if (ParseMoney(s, money)) {
        CAmount again;
        assert(ParseMoney(FormatMoney(money), again));
        assert(again == money);
    }
}

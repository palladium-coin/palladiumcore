// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/spanparsing.h>

#include <string>
#include <vector>

/** The helpers that parse descriptors and similar strings. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::string name = provider.ConsumeBytesAsString(provider.ConsumeIntegralInRange<size_t>(0, 8));
    const char sep = provider.ConsumeIntegral<char>();
    const std::string s = provider.ConsumeRemainingBytesAsString();

    Span<const char> sp = MakeSpan(s);
    (void)spanparsing::Const(name, sp);
    sp = MakeSpan(s);
    (void)spanparsing::Func(name, sp);
    sp = MakeSpan(s);
    (void)spanparsing::Expr(sp);
    (void)spanparsing::Split(MakeSpan(s), sep);
}

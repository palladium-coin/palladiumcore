// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <util/time.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

namespace {
CAddress ConsumeAddress(FuzzedDataProvider& provider)
{
    in_addr ip;
    ip.s_addr = provider.ConsumeIntegral<uint32_t>();
    CAddress addr(CService(CNetAddr(ip), provider.ConsumeIntegral<uint16_t>()), ServiceFlags(provider.ConsumeIntegral<uint64_t>()));
    addr.nTime = provider.ConsumeIntegralInRange<uint32_t>(0, 4000000000U);
    return addr;
}
} // namespace

/** The address manager through any sequence of operations: it stays consistent and returns what it holds. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    SetMockTime(provider.ConsumeIntegralInRange<int64_t>(1500000000, 2000000000));
    CAddrMan addrman;
    addrman.Clear();

    while (provider.remaining_bytes() > 0) {
        switch (provider.ConsumeIntegralInRange<int>(0, 6)) {
        case 0:
            (void)addrman.Add(ConsumeAddress(provider), ConsumeAddress(provider), provider.ConsumeIntegralInRange<int64_t>(0, 100000));
            break;
        case 1:
            addrman.Good(ConsumeAddress(provider), provider.ConsumeBool(), provider.ConsumeIntegralInRange<int64_t>(1500000000, 2000000000));
            break;
        case 2:
            addrman.Attempt(ConsumeAddress(provider), provider.ConsumeBool(), provider.ConsumeIntegralInRange<int64_t>(1500000000, 2000000000));
            break;
        case 3:
            if (addrman.size() > 0) {
                const CAddrInfo info = addrman.Select(provider.ConsumeBool());
                (void)info.ToString();
            }
            break;
        case 4: {
            const std::vector<CAddress> addrs = addrman.GetAddr();
            assert(addrs.size() <= addrman.size());
            break;
        }
        case 5:
            addrman.SetServices(ConsumeAddress(provider), ServiceFlags(provider.ConsumeIntegral<uint64_t>()));
            break;
        case 6:
            addrman.Clear();
            assert(addrman.size() == 0);
            break;
        }
    }
    SetMockTime(0);
}

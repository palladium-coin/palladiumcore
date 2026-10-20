// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <string>
#include <vector>

/** Parsing of host names, services and subnets (no name resolution), and printing them back. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());

    CNetAddr addr;
    if (LookupHost(s, addr, /* fAllowLookup */ false)) {
        (void)addr.GetNetwork();
        (void)addr.IsRoutable();
        (void)addr.GetGroup(std::vector<bool>{});
        const std::string printed = addr.ToString();
        assert(!printed.empty());
        CNetAddr again;
        assert(LookupHost(printed, again, false));
        assert(again == addr);
    }
    CService service;
    if (Lookup(s, service, /* portDefault */ 2333, /* fAllowLookup */ false)) {
        assert(!service.ToString().empty());
        (void)service.GetPort();
    }
    CSubNet subnet;
    if (LookupSubNet(s, subnet)) {
        (void)subnet.IsValid();
        (void)subnet.ToString();
    }
}

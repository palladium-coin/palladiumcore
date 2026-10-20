// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <test/fuzz/fuzz.h>

#include <string>
#include <vector>

/** The -whitelist and -whitebind option parsers. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());
    std::string error;
    NetWhitebindPermissions whitebind;
    if (NetWhitebindPermissions::TryParse(s, whitebind, error)) {
        (void)whitebind.m_service.ToString();
        (void)NetPermissions::ToStrings(whitebind.m_flags);
    }
    NetWhitelistPermissions whitelist;
    if (NetWhitelistPermissions::TryParse(s, whitelist, error)) {
        (void)whitelist.m_subnet.ToString();
        (void)NetPermissions::ToStrings(whitelist.m_flags);
    }
}

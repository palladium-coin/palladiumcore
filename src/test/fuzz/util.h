// Copyright (c) 2009-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_TEST_FUZZ_UTIL_H
#define PALLADIUM_TEST_FUZZ_UTIL_H

#include <arith_uint256.h>
#include <chainparamsbase.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <uint256.h>
#include <version.h>

#include <string>
#include <vector>

/** One of the three networks, chosen by the fuzzer. */
inline const std::string& ConsumeChainName(FuzzedDataProvider& provider)
{
    static const std::string names[] = {CBaseChainParams::MAIN, CBaseChainParams::TESTNET, CBaseChainParams::REGTEST};
    return names[provider.ConsumeIntegralInRange<size_t>(0, 2)];
}

inline uint256 ConsumeUInt256(FuzzedDataProvider& provider)
{
    const std::vector<uint8_t> bytes = provider.ConsumeBytes<uint8_t>(32);
    uint256 result;
    if (bytes.size() == 32) memcpy(result.begin(), bytes.data(), 32);
    return result;
}

/** A stream over the fuzzer input, ready for deserialization. */
inline CDataStream ConsumeDataStream(const std::vector<uint8_t>& buffer, int version = INIT_PROTO_VERSION)
{
    return CDataStream(buffer, SER_NETWORK, version);
}

#endif // PALLADIUM_TEST_FUZZ_UTIL_H

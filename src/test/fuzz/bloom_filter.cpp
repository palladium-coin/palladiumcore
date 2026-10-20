// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bloom.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>

#include <cassert>
#include <vector>

/** Bloom filters: what was inserted is always found. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CBloomFilter filter(provider.ConsumeIntegralInRange<unsigned int>(1, 1000),
                        provider.ConsumeFloatingPointInRange<double>(0.0001, 0.5),
                        provider.ConsumeIntegral<unsigned int>(),
                        provider.ConsumeIntegral<uint8_t>());
    CRollingBloomFilter rolling(provider.ConsumeIntegralInRange<unsigned int>(1, 1000),
                                provider.ConsumeFloatingPointInRange<double>(0.0001, 0.5));

    while (provider.remaining_bytes() > 0) {
        switch (provider.ConsumeIntegralInRange<int>(0, 2)) {
        case 0: {
            const std::vector<unsigned char> key = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 40));
            filter.insert(key);
            rolling.insert(key);
            assert(filter.contains(key));
            assert(rolling.contains(key));
            break;
        }
        case 1: {
            uint256 hash;
            const std::vector<unsigned char> raw = provider.ConsumeBytes<unsigned char>(32);
            if (raw.size() == 32) memcpy(hash.begin(), raw.data(), 32);
            filter.insert(hash);
            rolling.insert(hash);
            assert(filter.contains(hash));
            assert(rolling.contains(hash));
            break;
        }
        case 2: {
            const COutPoint outpoint(uint256(), provider.ConsumeIntegral<uint32_t>());
            filter.insert(outpoint);
            assert(filter.contains(outpoint));
            break;
        }
        }
        (void)filter.IsWithinSizeConstraints();
    }
    rolling.reset();
}

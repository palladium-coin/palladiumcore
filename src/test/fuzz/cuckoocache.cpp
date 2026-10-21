// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cuckoocache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>

#include <cstring>
#include <vector>

namespace {
struct Hasher {
    template <uint8_t hash_select>
    uint32_t operator()(const uint256& key) const
    {
        static_assert(hash_select < 8, "cuckoo cache has at most 8 hashes");
        uint32_t u;
        memcpy(&u, key.begin() + 4 * hash_select, 4);
        return u;
    }
};
} // namespace

/** The signature cache container: setup, insert, lookup and erase, with entries that collide. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CuckooCache::cache<uint256, Hasher> cache;
    cache.setup_bytes(provider.ConsumeIntegralInRange<size_t>(1, 1 << 16));

    while (provider.remaining_bytes() > 0) {
        uint256 key;
        // Few distinct keys, so lookups hit
        key.begin()[0] = provider.ConsumeIntegral<uint8_t>() % 32;
        switch (provider.ConsumeIntegralInRange<int>(0, 2)) {
        case 0:
            cache.insert(key);
            break;
        case 1:
            (void)cache.contains(key, /* erase */ false);
            break;
        case 2:
            (void)cache.contains(key, /* erase */ true);
            break;
        }
    }
}

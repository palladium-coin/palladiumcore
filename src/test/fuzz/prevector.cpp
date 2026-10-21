// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prevector.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <cassert>
#include <vector>

/** prevector behaves like std::vector through any sequence of operations. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    prevector<8, int> pv;
    std::vector<int> v;

    while (provider.remaining_bytes() > 0) {
        switch (provider.ConsumeIntegralInRange<int>(0, 8)) {
        case 0: {
            const int x = provider.ConsumeIntegral<int>();
            pv.push_back(x);
            v.push_back(x);
            break;
        }
        case 1:
            if (!v.empty()) {
                pv.pop_back();
                v.pop_back();
            }
            break;
        case 2: {
            const unsigned int n = provider.ConsumeIntegralInRange<unsigned int>(0, 40);
            pv.resize(n);
            v.resize(n);
            break;
        }
        case 3: {
            const size_t pos = provider.ConsumeIntegralInRange<size_t>(0, v.size());
            const int x = provider.ConsumeIntegral<int>();
            pv.insert(pv.begin() + pos, x);
            v.insert(v.begin() + pos, x);
            break;
        }
        case 4:
            if (!v.empty()) {
                const size_t pos = provider.ConsumeIntegralInRange<size_t>(0, v.size() - 1);
                pv.erase(pv.begin() + pos);
                v.erase(v.begin() + pos);
            }
            break;
        case 5:
            pv.clear();
            v.clear();
            break;
        case 6:
            pv.shrink_to_fit();
            break;
        case 7: {
            const unsigned int n = provider.ConsumeIntegralInRange<unsigned int>(0, 40);
            const int x = provider.ConsumeIntegral<int>();
            pv.assign(n, x);
            v.assign(n, x);
            break;
        }
        case 8: {
            const prevector<8, int> copy = pv;
            assert(copy == pv);
            pv = copy;
            break;
        }
        }
        assert(pv.size() == v.size());
        assert(std::equal(pv.begin(), pv.end(), v.begin()));
        assert(pv.empty() == v.empty());
    }
}

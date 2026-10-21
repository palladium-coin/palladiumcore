// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkqueue.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <vector>

namespace {
struct FuzzCheck {
    bool result;
    FuzzCheck() : result(true) {}
    explicit FuzzCheck(bool r) : result(r) {}
    bool operator()() { return result; }
    void swap(FuzzCheck& other) { std::swap(result, other.result); }
};
} // namespace

/** The script check queue, with the master thread doing the work: it fails if and only if a check fails. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CCheckQueue<FuzzCheck> queue{provider.ConsumeIntegralInRange<unsigned int>(1, 16)};
    bool expected = true;
    {
        CCheckQueueControl<FuzzCheck> control(&queue);
        const size_t batches = provider.ConsumeIntegralInRange<size_t>(0, 8);
        for (size_t i = 0; i < batches; i++) {
            std::vector<FuzzCheck> checks;
            const size_t n = provider.ConsumeIntegralInRange<size_t>(0, 40);
            for (size_t j = 0; j < n; j++) {
                const bool ok = provider.ConsumeBool();
                expected = expected && ok;
                checks.emplace_back(ok);
            }
            control.Add(checks);
        }
        assert(control.Wait() == expected);
    }
}

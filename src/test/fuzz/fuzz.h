// Copyright (c) 2009-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_TEST_FUZZ_FUZZ_H
#define PALLADIUM_TEST_FUZZ_FUZZ_H

#include <stdint.h>
#include <vector>

/** Called once before the first input. Optional: a target that doesn't define it is not initialized. */
void initialize() __attribute__((weak));

/** The code under test: every fuzz target defines this. It must not crash for any input. */
void test_one_input(const std::vector<uint8_t>& buffer);

#endif // PALLADIUM_TEST_FUZZ_FUZZ_H

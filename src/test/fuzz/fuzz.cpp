// Copyright (c) 2009-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <functional>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

// Needed by BasicTestingSetup: the fuzz targets log nothing
extern const std::function<void(const std::string&)> G_TEST_LOG_FUN;
const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

// libFuzzer entry points. The harness is only meaningful when built with
// -fsanitize=fuzzer (see doc/fuzzing.md).
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    if (initialize) initialize();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    test_one_input(std::vector<uint8_t>(data, data + size));
    return 0;
}

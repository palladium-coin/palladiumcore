// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <stdexcept>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** The script assembly parser, and the disassembler on the same bytes. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());
    try {
        const CScript script = ParseScript(s);
        (void)ScriptToAsmStr(script);
    } catch (const std::runtime_error&) {
    }
    const CScript raw(buffer.begin(), buffer.end());
    (void)ScriptToAsmStr(raw, true);
    (void)FormatScript(raw);
}

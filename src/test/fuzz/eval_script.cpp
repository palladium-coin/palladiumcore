// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Evaluate an arbitrary script as legacy, segwit v0 and tapscript, with arbitrary flags. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const unsigned int flags = provider.ConsumeIntegral<unsigned int>() & (SCRIPT_VERIFY_END_MARKER - 1);
    const SigVersion sigversions[] = {SigVersion::BASE, SigVersion::WITNESS_V0, SigVersion::TAPSCRIPT};
    const SigVersion sigversion = sigversions[provider.ConsumeIntegralInRange<size_t>(0, 2)];

    std::vector<std::vector<unsigned char>> stack;
    const size_t n = provider.ConsumeIntegralInRange<size_t>(0, 5);
    for (size_t i = 0; i < n; i++) stack.push_back(provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 40)));
    const std::vector<unsigned char> raw = provider.ConsumeRemainingBytes<unsigned char>();
    const CScript script(raw.begin(), raw.end());

    // What VerifyScript sets up before running a tapscript
    ScriptExecutionData execdata;
    execdata.m_tapleaf_hash_init = true;
    execdata.m_codeseparator_pos_init = true;
    execdata.m_annex_init = true;
    execdata.m_annex_present = false;
    execdata.m_validation_weight_left_init = true;
    execdata.m_validation_weight_left = provider.ConsumeIntegralInRange<int64_t>(-100, 5000);
    ScriptExecutionData execdata_copy = execdata;

    const BaseSignatureChecker checker;
    ScriptError err = SCRIPT_ERR_OK;
    std::vector<std::vector<unsigned char>> stack_copy = stack;
    const bool ok = EvalScript(stack, script, flags, checker, sigversion, execdata, &err);
    ScriptError err2 = SCRIPT_ERR_OK;
    const bool ok2 = EvalScript(stack_copy, script, flags, checker, sigversion, execdata_copy, &err2);
    assert(ok == ok2 && err == err2 && stack == stack_copy);
    assert(ok == (err == SCRIPT_ERR_OK));
}

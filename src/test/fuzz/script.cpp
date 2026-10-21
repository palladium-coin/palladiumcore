// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <script/standard.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Script classification: Solver and destination extraction on any script. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const CScript script(buffer.begin(), buffer.end());

    (void)script.IsPayToScriptHash();
    (void)script.IsPayToWitnessScriptHash();
    (void)script.IsPushOnly();
    (void)script.IsUnspendable();
    (void)script.HasValidOps();
    (void)script.GetSigOpCount(true);
    (void)script.GetSigOpCount(false);
    int version;
    std::vector<unsigned char> program;
    (void)script.IsWitnessProgram(version, program);

    opcodetype opcode;
    std::vector<unsigned char> data;
    for (CScript::const_iterator it = script.begin(); it != script.end() && script.GetOp(it, opcode, data);) {
    }

    std::vector<std::vector<unsigned char>> solutions;
    const txnouttype type = Solver(script, solutions);
    (void)GetTxnOutputType(type);

    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        assert(IsValidDestination(dest));
        // The script of a destination is a script for that destination
        CTxDestination again;
        assert(ExtractDestination(GetScriptForDestination(dest), again));
        assert(again == dest);
    }
    std::vector<CTxDestination> dests;
    int required;
    txnouttype type2;
    (void)ExtractDestinations(script, type2, dests, required);
}

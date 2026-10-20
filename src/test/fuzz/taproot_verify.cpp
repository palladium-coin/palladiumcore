// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/sha256.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <cassert>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

namespace {

uint256 TapLeafHash(uint8_t leaf_version, const CScript& script)
{
    unsigned char th[CSHA256::OUTPUT_SIZE];
    const std::string tag = "TapLeaf";
    CSHA256().Write((const unsigned char*)tag.data(), tag.size()).Finalize(th);
    uint256 result;
    CDataStream ss(SER_GETHASH, 0);
    ss << leaf_version << script;
    CSHA256().Write(th, 32).Write(th, 32).Write((const unsigned char*)ss.data(), ss.size()).Finalize(result.begin());
    return result;
}

/** Script truthiness of the program push: all zeros, or negative zero, makes scriptPubKey evaluate to false. */
bool ProgramIsTrue(const CScript& spk)
{
    for (size_t i = 2; i < spk.size(); i++) {
        if (spk[i] != 0) return !(i == spk.size() - 1 && spk[i] == 0x80);
    }
    return false;
}

} // namespace

/**
 * Spend a witness v1 output with a witness made from the input.
 *
 * Mode 1 uses an arbitrary program and witness, so the Taproot checks in the interpreter
 * see malformed control blocks, annexes, wrong sizes and so on. Mode 2 builds a valid
 * output for a fuzzed tapscript, so the script itself is executed (OP_SUCCESS, CHECKSIGADD, ...).
 *
 * Whatever the flags, the result must be deterministic, and before Taproot is enforced
 * (no SCRIPT_VERIFY_TAPROOT) a v1 witness program must be spendable with any non-empty witness.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    CScript spk;
    CScriptWitness witness;
    const CAmount amount = provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY);

    if (provider.ConsumeBool()) {
        spk << OP_1 << provider.ConsumeBytes<unsigned char>(provider.ConsumeBool() ? 32 : provider.ConsumeIntegralInRange<size_t>(0, 40));
        const size_t n = provider.ConsumeIntegralInRange<size_t>(0, 6);
        for (size_t i = 0; i < n; i++) {
            witness.stack.push_back(provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 80)));
        }
    } else {
        const std::vector<unsigned char> internal_bytes = provider.ConsumeBytes<unsigned char>(32);
        const XOnlyPubKey internal(internal_bytes);
        if (!internal.IsFullyValid()) return;
        const std::vector<unsigned char> raw = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 200));
        const CScript leaf(raw.begin(), raw.end());
        const uint8_t leaf_version = provider.ConsumeBool() ? TAPROOT_LEAF_TAPSCRIPT : (provider.ConsumeIntegral<uint8_t>() & TAPROOT_LEAF_MASK);
        const uint256 leaf_hash = TapLeafHash(leaf_version, leaf);
        const std::pair<XOnlyPubKey, bool> output = internal.CreatePayToTaprootPubKey(&leaf_hash);
        if (!output.first.IsFullyValid()) return;
        spk << OP_1 << std::vector<unsigned char>(output.first.begin(), output.first.end());

        const size_t n = provider.ConsumeIntegralInRange<size_t>(0, 4);
        for (size_t i = 0; i < n; i++) {
            witness.stack.push_back(provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 80)));
        }
        witness.stack.push_back(std::vector<unsigned char>(leaf.begin(), leaf.end()));
        std::vector<unsigned char> control;
        control.push_back((leaf_version & TAPROOT_LEAF_MASK) | (output.second ? 1 : 0));
        control.insert(control.end(), internal.begin(), internal.end());
        witness.stack.push_back(control);
        if (provider.ConsumeBool()) {
            // Annex: last element, starts with 0x50
            std::vector<unsigned char> annex = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 20));
            annex.insert(annex.begin(), 0x50);
            witness.stack.push_back(annex);
        }
    }

    // Flags: any combination of the defined ones, WITNESS (and P2SH) always on
    unsigned int flags = provider.ConsumeIntegralInRange<unsigned int>(0, (SCRIPT_VERIFY_END_MARKER - 1) * 2U) & (SCRIPT_VERIFY_END_MARKER - 1);
    flags |= SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS;
    if (!provider.ConsumeBool()) flags |= SCRIPT_VERIFY_TAPROOT;

    CMutableTransaction credit;
    credit.nVersion = 1;
    credit.vin.resize(1);
    credit.vout.resize(1);
    credit.vout[0].scriptPubKey = spk;
    credit.vout[0].nValue = amount;
    CMutableTransaction spend;
    spend.nVersion = provider.ConsumeBool() ? 1 : 2;
    spend.nLockTime = provider.ConsumeIntegral<uint32_t>();
    spend.vin.resize(1);
    spend.vin[0].prevout = COutPoint(CTransaction(credit).GetHash(), 0);
    spend.vin[0].nSequence = provider.ConsumeBool() ? CTxIn::SEQUENCE_FINAL : provider.ConsumeIntegral<uint32_t>();
    spend.vin[0].scriptWitness = witness;
    spend.vout.resize(1);
    spend.vout[0].nValue = 0;

    const std::vector<CTxOut> spent{credit.vout[0]};
    const PrecomputedTransactionData txdata(spend, spent);
    const MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);

    ScriptError err1 = SCRIPT_ERR_OK, err2 = SCRIPT_ERR_OK;
    const bool ok1 = VerifyScript(CScript(), spk, &witness, flags, checker, &err1);
    const bool ok2 = VerifyScript(CScript(), spk, &witness, flags, checker, &err2);
    assert(ok1 == ok2 && err1 == err2);
    assert(ok1 == (err1 == SCRIPT_ERR_OK));

    // Before Taproot is enforced a witness v1 program is anyone-can-spend (unless the program itself is a false value)
    const bool is_v1_program = spk.size() == 34 && spk[1] == 0x20;
    if (is_v1_program && !(flags & SCRIPT_VERIFY_TAPROOT) && !(flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) && !witness.stack.empty() && ProgramIsTrue(spk)) {
        assert(ok1);
    }
}

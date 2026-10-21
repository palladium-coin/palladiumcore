// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <version.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Signature checkers on arbitrary transactions: the mutable and the immutable one give the same answers. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const CAmount amount = provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY);
    const unsigned int n_in_raw = provider.ConsumeIntegral<unsigned int>();
    const std::vector<uint8_t> tx_bytes = provider.ConsumeBytes<uint8_t>(provider.ConsumeIntegralInRange<size_t>(0, 400));
    CMutableTransaction mtx;
    CDataStream ds(tx_bytes, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        ds >> mtx;
    } catch (const std::ios_base::failure&) {
        return;
    }
    // Checkers are only created for inputs that exist
    if (mtx.vin.empty()) return;
    const unsigned int n_in = n_in_raw % mtx.vin.size();
    const CTransaction tx(mtx);
    const std::vector<unsigned char> sig = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 80));
    const std::vector<unsigned char> pubkey = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 66));
    const std::vector<unsigned char> raw_script = provider.ConsumeRemainingBytes<unsigned char>();
    const CScript script(raw_script.begin(), raw_script.end());
    const CScriptNum lock_time(provider.ConsumeIntegralInRange<int64_t>(0, 2147483647));

    const TransactionSignatureChecker checker(&tx, n_in, amount);
    const MutableTransactionSignatureChecker mutable_checker(&mtx, n_in, amount);
    for (const SigVersion sigversion : {SigVersion::BASE, SigVersion::WITNESS_V0}) {
        assert(checker.CheckSig(sig, pubkey, script, sigversion) == mutable_checker.CheckSig(sig, pubkey, script, sigversion));
    }
    assert(checker.CheckLockTime(lock_time) == mutable_checker.CheckLockTime(lock_time));
    assert(checker.CheckSequence(lock_time) == mutable_checker.CheckSequence(lock_time));
}

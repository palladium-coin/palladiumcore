// Copyright (c) 2024 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/sha256.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers (replicate interpreter.cpp internals for cross-validation)
// ─────────────────────────────────────────────────────────────────────────────

/** BIP341 tagged SHA256: SHA256(SHA256(tag)||SHA256(tag)||data). */
static uint256 TaggedHashTest(const std::string& tag, const unsigned char* data, size_t len)
{
    unsigned char th[CSHA256::OUTPUT_SIZE];
    CSHA256().Write((const unsigned char*)tag.data(), tag.size()).Finalize(th);
    uint256 result;
    CSHA256().Write(th, 32).Write(th, 32).Write(data, len).Finalize(result.begin());
    return result;
}

/** Compute TapLeaf hash with the same serialization as interpreter.cpp. */
static uint256 ComputeTapleafHashTest(uint8_t leaf_version, const CScript& script)
{
    // CSHA256Writer("TapLeaf") << leaf_version << script
    // serializes leaf_version as 1 byte, script as compact_size + bytes
    CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << leaf_version;
    ss << script;
    return TaggedHashTest("TapLeaf", (const unsigned char*)ss.data(), ss.size());
}

/** Compute TapBranch hash (lexicographic sort of the two children). */
static uint256 ComputeTapbranchHashTest(const uint256& a, const uint256& b)
{
    unsigned char buf[64];
    if (std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end())) {
        memcpy(buf,      a.begin(), 32);
        memcpy(buf + 32, b.begin(), 32);
    } else {
        memcpy(buf,      b.begin(), 32);
        memcpy(buf + 32, a.begin(), 32);
    }
    return TaggedHashTest("TapBranch", buf, 64);
}

/** Standard taproot verification flags. */
static const unsigned int TAPROOT_FLAGS =
    SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH;

/** Build "OP_1 <32-byte xonly>" scriptPubKey. */
static CScript MakeTaprootSPK(const XOnlyPubKey& output_key)
{
    CScript spk;
    spk << OP_1 << std::vector<unsigned char>(output_key.begin(), output_key.end());
    return spk;
}

/** Build a valid script-path control block (depth-0 leaf, no sibling hashes). */
static std::vector<unsigned char> MakeControlBlock(uint8_t leaf_version, bool parity,
                                                    const XOnlyPubKey& internal_xonly)
{
    std::vector<unsigned char> ctrl;
    ctrl.push_back(leaf_version | (parity ? 1 : 0));
    ctrl.insert(ctrl.end(), internal_xonly.begin(), internal_xonly.end());
    return ctrl;
}

/** Helper: call CreatePayToTaprootPubKey and unpack the pair (C++11-compatible). */
static bool CreateTaprootOutput(const XOnlyPubKey& xonly, const uint256* merkle_root,
                                 XOnlyPubKey& out_key, bool& out_parity)
{
    std::pair<XOnlyPubKey, bool> result = xonly.CreatePayToTaprootPubKey(merkle_root);
    out_key    = result.first;
    out_parity = result.second;
    return out_key.IsFullyValid();
}

/** Known WIF key for deterministic tests. */
static const std::string WIF_KEY = "Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw";


// =============================================================================
// Suite A — TapTweak primitive
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(taproot_tweak_tests, BasicTestingSetup)

// A.1 — ComputeTapTweak(nullptr) matches the BIP341 manual formula
BOOST_AUTO_TEST_CASE(tap_tweak_key_path_only)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();

    const uint256 tweak_api = xonly.ComputeTapTweak(nullptr);

    // Reference: SHA256(SHA256("TapTweak")||SHA256("TapTweak")||xonly_key)
    const uint256 tweak_ref = TaggedHashTest("TapTweak", xonly.data(), XOnlyPubKey::SIZE);

    BOOST_CHECK(!tweak_api.IsNull());
    BOOST_CHECK(tweak_api == tweak_ref);
}

// A.2 — ComputeTapTweak with merkle root differs from key-path-only and matches formula
BOOST_AUTO_TEST_CASE(tap_tweak_with_merkle_root)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();

    uint256 some_root = uint256S("0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");

    const uint256 tweak_no_root   = xonly.ComputeTapTweak(nullptr);
    const uint256 tweak_with_root = xonly.ComputeTapTweak(&some_root);

    BOOST_CHECK(tweak_no_root != tweak_with_root);

    // Reference: SHA256(SHA256("TapTweak")||SHA256("TapTweak")||xonly_key||merkle_root)
    unsigned char data[64];
    memcpy(data,      xonly.data(),      32);
    memcpy(data + 32, some_root.begin(), 32);
    const uint256 tweak_ref = TaggedHashTest("TapTweak", data, 64);
    BOOST_CHECK(tweak_with_root == tweak_ref);
}

// A.3 — CreatePayToTaprootPubKey is consistent with VerifyTaprootCommitment
BOOST_AUTO_TEST_CASE(create_taproot_pubkey_consistency)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, nullptr, output_key, parity));

    CScript taproot_spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction credit = BuildCreditingTransaction(taproot_spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, taproot_spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    uint256 sighash = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount,
                                    SigVersion::TAPROOT, &txdata);
    BOOST_REQUIRE(!sighash.IsNull());

    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key.SignSchnorrTaproot(sighash, sig, nullptr));
    BOOST_CHECK_EQUAL(sig.size(), (size_t)XOnlyPubKey::SCHNORR_SIGNATURE_SIZE);

    spend.vin[0].scriptWitness.stack = {sig};
    CTransaction spend_tx(spend);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(VerifyScript(CScript(), taproot_spk, &spend_tx.vin[0].scriptWitness,
                             TAPROOT_FLAGS, checker, &serror));
}

// A.4 — Flipping the parity bit in a script-path control block causes MISMATCH
BOOST_AUTO_TEST_CASE(parity_bit_flip_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_1;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);

    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript taproot_spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(taproot_spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> ctrl_flipped = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, !parity, internal_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        ctrl_flipped
    };
    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, taproot_spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), taproot_spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================
// Suite B — SignatureHashSchnorr (untested before, highest priority)
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(taproot_sighash_tests, BasicTestingSetup)

// Helper: build a single-input taproot tx and return the spent_outputs vector.
// The caller constructs PrecomputedTransactionData(spend_out, spent_outs_out)
// because PrecomputedTransactionData has no default constructor.
static void BuildKeyPathTx(const std::string& wif, CAmount amount,
                            CMutableTransaction& spend_out,
                            std::vector<CTxOut>& spent_outs_out,
                            CScript& spk_out)
{
    CKey key = DecodeSecret(wif);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));

    spk_out = MakeTaprootSPK(output_key);
    CMutableTransaction credit = BuildCreditingTransaction(spk_out, amount);
    CTransaction credit_tx(credit);
    spend_out = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
    spent_outs_out.clear();
    spent_outs_out.push_back(CTxOut(amount, spk_out));
}

// B.1 — SIGHASH_DEFAULT is deterministic and non-null
BOOST_AUTO_TEST_CASE(sighash_default_is_deterministic)
{
    const CAmount amount = 5000000;
    CMutableTransaction spend;
    std::vector<CTxOut> spent_outs;
    CScript spk;
    BuildKeyPathTx(WIF_KEY, amount, spend, spent_outs, spk);
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    uint256 h1 = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    uint256 h2 = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    BOOST_CHECK(!h1.IsNull());
    BOOST_CHECK(h1 == h2);
}

// B.2 — SIGHASH_DEFAULT (0x00) and SIGHASH_ALL (0x01) produce different hashes
BOOST_AUTO_TEST_CASE(sighash_default_vs_sighash_all_differ)
{
    const CAmount amount = 5000000;
    CMutableTransaction spend;
    std::vector<CTxOut> spent_outs;
    CScript spk;
    BuildKeyPathTx(WIF_KEY, amount, spend, spent_outs, spk);
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    uint256 h_def = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    uint256 h_all = SignatureHash(CScript(), spend, 0, SIGHASH_ALL,     amount, SigVersion::TAPROOT, &txdata);
    BOOST_CHECK(!h_def.IsNull());
    BOOST_CHECK(!h_all.IsNull());
    BOOST_CHECK(h_def != h_all);
}

// B.3 — All valid hash types: sign → verify succeeds
BOOST_AUTO_TEST_CASE(sighash_all_variants_sign_verify)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 5000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    const int valid_htypes[] = {
        SIGHASH_DEFAULT,
        SIGHASH_ALL,
        SIGHASH_NONE,
        SIGHASH_SINGLE,
        SIGHASH_ALL  | SIGHASH_ANYONECANPAY,
        SIGHASH_NONE | SIGHASH_ANYONECANPAY,
        SIGHASH_SINGLE | SIGHASH_ANYONECANPAY,
    };

    for (size_t i = 0; i < sizeof(valid_htypes)/sizeof(valid_htypes[0]); ++i) {
        int htype = valid_htypes[i];
        uint256 sighash = SignatureHash(CScript(), spend, 0, htype, amount, SigVersion::TAPROOT, &txdata);
        BOOST_CHECK_MESSAGE(!sighash.IsNull(), "null sighash for htype=" + std::to_string(htype));

        std::vector<unsigned char> sig;
        BOOST_REQUIRE_MESSAGE(key.SignSchnorrTaproot(sighash, sig, nullptr),
                              "SignSchnorrTaproot failed for htype=" + std::to_string(htype));

        if (htype != SIGHASH_DEFAULT) {
            sig.push_back(static_cast<unsigned char>(htype));
            BOOST_CHECK_EQUAL(sig.size(), (size_t)(XOnlyPubKey::SCHNORR_SIGNATURE_SIZE + 1));
        } else {
            BOOST_CHECK_EQUAL(sig.size(), (size_t)XOnlyPubKey::SCHNORR_SIGNATURE_SIZE);
        }

        CMutableTransaction spend_copy = spend;
        spend_copy.vin[0].scriptWitness.stack = {sig};
        std::vector<CTxOut> spent_outs2;
        spent_outs2.push_back(CTxOut(amount, spk));
        const PrecomputedTransactionData txdata2(spend_copy, spent_outs2);
        CTransaction spend_tx(spend_copy);
        MutableTransactionSignatureChecker checker(&spend_copy, 0, amount, txdata2);
        ScriptError serror;
        bool ok = VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                               TAPROOT_FLAGS, checker, &serror);
        BOOST_CHECK_MESSAGE(ok, "VerifyScript failed for htype=" + std::to_string(htype)
                            + " err=" + std::to_string(serror));
    }
}

// B.4 — Invalid hash types produce a null sighash
BOOST_AUTO_TEST_CASE(sighash_invalid_hashtypes_rejected)
{
    const CAmount amount = 5000000;
    CMutableTransaction spend;
    std::vector<CTxOut> spent_outs;
    CScript spk;
    BuildKeyPathTx(WIF_KEY, amount, spend, spent_outs, spk);
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    const int invalid_htypes[] = {0x04, 0x05, 0x40, 0x7F, 0xFF, 0x84, 0x85};
    for (size_t i = 0; i < sizeof(invalid_htypes)/sizeof(invalid_htypes[0]); ++i) {
        int htype = invalid_htypes[i];
        uint256 h = SignatureHash(CScript(), spend, 0, htype, amount, SigVersion::TAPROOT, &txdata);
        BOOST_CHECK_MESSAGE(h.IsNull(), "Expected null sighash for invalid htype=" + std::to_string(htype));
    }
}

// B.5 — SIGHASH_SINGLE with no matching output returns null sighash
BOOST_AUTO_TEST_CASE(sighash_single_no_matching_output_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 5000000;

    CMutableTransaction c0 = BuildCreditingTransaction(spk, amount);
    CMutableTransaction c1 = BuildCreditingTransaction(spk, amount);
    CTransaction ctx0(c0), ctx1(c1);

    CMutableTransaction spend;
    spend.nVersion = 2;
    CTxIn in0; in0.prevout = COutPoint(ctx0.GetHash(), 0); in0.nSequence = CTxIn::SEQUENCE_FINAL;
    CTxIn in1; in1.prevout = COutPoint(ctx1.GetHash(), 0); in1.nSequence = CTxIn::SEQUENCE_FINAL;
    spend.vin = {in0, in1};
    spend.vout = {CTxOut(amount * 2 - 1000, CScript())};

    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    // in_pos=1, vout.size()=1 → failure
    uint256 h = SignatureHash(CScript(), spend, 1, SIGHASH_SINGLE, amount, SigVersion::TAPROOT, &txdata);
    BOOST_CHECK(h.IsNull());
}

// B.6 — Without spent_outputs, m_bip341_taproot_ready is false → null sighash
BOOST_AUTO_TEST_CASE(sighash_requires_spent_outputs)
{
    const CAmount amount = 5000000;
    CMutableTransaction spend;
    std::vector<CTxOut> spent_outs;
    CScript spk;
    BuildKeyPathTx(WIF_KEY, amount, spend, spent_outs, spk);

    const PrecomputedTransactionData txdata_empty(spend);
    BOOST_CHECK(!txdata_empty.m_bip341_taproot_ready);

    uint256 h = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount,
                              SigVersion::TAPROOT, &txdata_empty);
    BOOST_CHECK(h.IsNull());
}

// B.7 — Single 0x50-prefixed element in key-path is NOT treated as annex → size error
BOOST_AUTO_TEST_CASE(sighash_single_annex_tag_not_stripped)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    unsigned char fake[] = {0x50, 0xAA, 0xBB};
    spend.vin[0].scriptWitness.stack = {std::vector<unsigned char>(fake, fake + 3)};

    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG_SIZE);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================
// Suite C — Key-path spend end-to-end
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(taproot_keypath_tests, BasicTestingSetup)

// C.1 — Full key-path spend pipeline succeeds
BOOST_AUTO_TEST_CASE(keypath_spend_valid)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    uint256 sighash = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount,
                                    SigVersion::TAPROOT, &txdata);
    BOOST_REQUIRE(!sighash.IsNull());

    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key.SignSchnorrTaproot(sighash, sig, nullptr));
    BOOST_CHECK_EQUAL(sig.size(), (size_t)XOnlyPubKey::SCHNORR_SIGNATURE_SIZE);

    spend.vin[0].scriptWitness.stack = {sig};
    CTransaction spend_tx(spend);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                             TAPROOT_FLAGS, checker, &serror));
}

// C.2 — Signature over wrong message fails
BOOST_AUTO_TEST_CASE(keypath_spend_wrong_sig_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);

    uint256 random_hash = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    std::vector<unsigned char> bad_sig;
    BOOST_REQUIRE(key.SignSchnorrTaproot(random_hash, bad_sig, nullptr));

    spend.vin[0].scriptWitness.stack = {bad_sig};
    CTransaction spend_tx(spend);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG);
}

// C.3 — Valid signature from wrong key fails
BOOST_AUTO_TEST_CASE(keypath_spend_wrong_key_fails)
{
    CKey key_a = DecodeSecret(WIF_KEY);
    CKey key_b;
    key_b.MakeNewKey(true);
    BOOST_REQUIRE(key_a.IsValid() && key_b.IsValid());

    XOnlyPubKey xonly_b = key_b.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key_b; bool parity_b;
    BOOST_REQUIRE(CreateTaprootOutput(xonly_b, nullptr, output_key_b, parity_b));
    CScript spk = MakeTaprootSPK(output_key_b);
    const CAmount amount = 1000000;

    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);

    uint256 sighash = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount,
                                    SigVersion::TAPROOT, &txdata);

    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key_a.SignSchnorrTaproot(sighash, sig, nullptr));

    spend.vin[0].scriptWitness.stack = {sig};
    CTransaction spend_tx(spend);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG);
}

// C.4 — Empty witness stack fails
BOOST_AUTO_TEST_CASE(keypath_spend_empty_witness_fails)
{
    const CAmount amount = 1000000;
    CMutableTransaction spend;
    std::vector<CTxOut> spent_outs;
    CScript spk;
    taproot_sighash_tests::BuildKeyPathTx(WIF_KEY, amount, spend, spent_outs, spk);
    const PrecomputedTransactionData txdata(spend, spent_outs);

    spend.vin[0].scriptWitness.stack = {};
    CTransaction spend_tx(spend);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================
// Suite D — Script-path spend
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(taproot_scriptpath_tests, BasicTestingSetup)

// D.1 — OP_1 (always-true) script-path spend succeeds
BOOST_AUTO_TEST_CASE(scriptpath_op1_valid)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_1;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        ctrl
    };

    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                             TAPROOT_FLAGS, checker, &serror));
}

// D.2 — Wrong internal key in control block → WITNESS_PROGRAM_MISMATCH
BOOST_AUTO_TEST_CASE(scriptpath_wrong_internal_key_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_1;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    CKey other_key;
    other_key.MakeNewKey(true);
    XOnlyPubKey other_xonly = other_key.GetPubKey().GetXOnlyPubKey();
    std::vector<unsigned char> bad_ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, other_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        bad_ctrl
    };
    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
}

// D.3 — All-zero internal key (not a valid x-only pubkey) → WITNESS_PROGRAM_MISMATCH
BOOST_AUTO_TEST_CASE(scriptpath_invalid_internal_key_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_1;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> bad_ctrl(TAPROOT_CONTROL_BASE_SIZE, 0x00);
    bad_ctrl[0] = TAPROOT_LEAF_TAPSCRIPT | (parity ? 1 : 0);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        bad_ctrl
    };
    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
}

// D.4 — Various invalid control block sizes → TAPROOT_WRONG_CONTROL_SIZE
BOOST_AUTO_TEST_CASE(scriptpath_wrong_control_size_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_1;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);

    const size_t bad_sizes[] = {32, 34, TAPROOT_CONTROL_MAX_SIZE + 32};
    for (size_t si = 0; si < sizeof(bad_sizes)/sizeof(bad_sizes[0]); ++si) {
        std::vector<unsigned char> ctrl(bad_sizes[si], 0xAA);
        CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
        spend.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
            ctrl
        };
        CTransaction spend_tx(spend);
        std::vector<CTxOut> spent_outs;
        spent_outs.push_back(CTxOut(amount, spk));
        PrecomputedTransactionData txdata(spend, spent_outs);
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        ScriptError serror;
        bool ok = VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                               TAPROOT_FLAGS, checker, &serror);
        BOOST_CHECK_MESSAGE(!ok, "Expected failure for ctrl size=" + std::to_string(bad_sizes[si]));
        BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_TAPROOT_WRONG_CONTROL_SIZE);
    }
}

// D.5 — Two-leaf Merkle tree: spend via either leaf, wrong sibling fails
BOOST_AUTO_TEST_CASE(scriptpath_two_leaves_merkle)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript script_a; script_a << OP_1;
    CScript script_b; script_b << OP_1; script_b << OP_DROP; script_b << OP_1;

    uint256 leaf_a = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, script_a);
    uint256 leaf_b = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, script_b);
    uint256 merkle_root = ComputeTapbranchHashTest(leaf_a, leaf_b);

    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &merkle_root, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    // Spend via leaf A (sibling = leaf_b)
    {
        CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
        CTransaction credit_tx(credit);
        CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
        std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
        ctrl.insert(ctrl.end(), leaf_b.begin(), leaf_b.end());
        spend.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(script_a.begin(), script_a.end()), ctrl
        };
        CTransaction spend_tx(spend);
        std::vector<CTxOut> spent_outs;
        spent_outs.push_back(CTxOut(amount, spk));
        PrecomputedTransactionData txdata(spend, spent_outs);
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        ScriptError serror;
        BOOST_CHECK(VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                 TAPROOT_FLAGS, checker, &serror));
    }

    // Spend via leaf B (sibling = leaf_a)
    {
        CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
        CTransaction credit_tx(credit);
        CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
        std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
        ctrl.insert(ctrl.end(), leaf_a.begin(), leaf_a.end());
        spend.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(script_b.begin(), script_b.end()), ctrl
        };
        CTransaction spend_tx(spend);
        std::vector<CTxOut> spent_outs;
        spent_outs.push_back(CTxOut(amount, spk));
        PrecomputedTransactionData txdata(spend, spent_outs);
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        ScriptError serror;
        BOOST_CHECK(VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                 TAPROOT_FLAGS, checker, &serror));
    }

    // Wrong sibling → commitment fails
    {
        CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
        CTransaction credit_tx(credit);
        CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
        uint256 wrong_sib = uint256S("0x1111111111111111111111111111111111111111111111111111111111111111");
        std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
        ctrl.insert(ctrl.end(), wrong_sib.begin(), wrong_sib.end());
        spend.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(script_a.begin(), script_a.end()), ctrl
        };
        CTransaction spend_tx(spend);
        std::vector<CTxOut> spent_outs;
        spent_outs.push_back(CTxOut(amount, spk));
        PrecomputedTransactionData txdata(spend, spent_outs);
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        ScriptError serror;
        BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                  TAPROOT_FLAGS, checker, &serror));
    }
}

// D.6 — Leaf version mismatch (Case A), future version success (Case B), DISCOURAGE (Case C)
BOOST_AUTO_TEST_CASE(scriptpath_wrong_leaf_version)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();
    CScript leaf_script; leaf_script << OP_1;
    const CAmount amount = 1000000;

    // Case A: committed with 0xc0, control uses 0xc2 → MISMATCH
    {
        uint256 lh = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
        XOnlyPubKey out_key; bool par;
        BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &lh, out_key, par));
        CScript spk = MakeTaprootSPK(out_key);
        CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
        CTransaction credit_tx(credit);
        CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
        std::vector<unsigned char> ctrl = MakeControlBlock(0xc2, par, internal_xonly);
        spend.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()), ctrl
        };
        CTransaction spend_tx(spend);
        std::vector<CTxOut> spent_outs; spent_outs.push_back(CTxOut(amount, spk));
        PrecomputedTransactionData txdata(spend, spent_outs);
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        ScriptError serror;
        BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                  TAPROOT_FLAGS, checker, &serror));
        BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
    }

    // Cases B & C: committed with 0xc2, control uses 0xc2
    {
        uint256 lh = ComputeTapleafHashTest(0xc2, leaf_script);
        XOnlyPubKey out_key; bool par;
        BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &lh, out_key, par));
        CScript spk = MakeTaprootSPK(out_key);
        std::vector<unsigned char> ctrl = MakeControlBlock(0xc2, par, internal_xonly);

        // Case B: no DISCOURAGE flag → success (future taproot version)
        {
            CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
            CTransaction credit_tx(credit);
            CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
            spend.vin[0].scriptWitness.stack = {
                std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()), ctrl
            };
            CTransaction spend_tx(spend);
            std::vector<CTxOut> spent_outs; spent_outs.push_back(CTxOut(amount, spk));
            PrecomputedTransactionData txdata(spend, spent_outs);
            MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
            ScriptError serror;
            BOOST_CHECK(VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                     TAPROOT_FLAGS, checker, &serror));
        }

        // Case C: with DISCOURAGE flag → fails
        {
            CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
            CTransaction credit_tx(credit);
            CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);
            spend.vin[0].scriptWitness.stack = {
                std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()), ctrl
            };
            CTransaction spend_tx(spend);
            std::vector<CTxOut> spent_outs; spent_outs.push_back(CTxOut(amount, spk));
            PrecomputedTransactionData txdata(spend, spent_outs);
            MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
            ScriptError serror;
            unsigned int disc_flags = TAPROOT_FLAGS | SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION;
            BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                                      disc_flags, checker, &serror));
            BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION);
        }
    }
}

// D.7 — Single 0x50-prefixed element in key-path: NOT annex → SCHNORR_SIG_SIZE
BOOST_AUTO_TEST_CASE(scriptpath_annex_single_element_not_stripped)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    unsigned char annex_like[] = {0x50, 0xAA};
    spend.vin[0].scriptWitness.stack = {std::vector<unsigned char>(annex_like, annex_like + 2)};
    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG_SIZE);
}

// D.8 — Tapscript OP_CHECKSIG with empty pubkey fails with TAPSCRIPT_EMPTY_PUBKEY
BOOST_AUTO_TEST_CASE(scriptpath_checksig_empty_pubkey_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_CHECKSIG;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>{},
        std::vector<unsigned char>{},
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        ctrl
    };

    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_TAPSCRIPT_EMPTY_PUBKEY);
}

// D.9 — Tapscript OP_CHECKSIG with unknown pubkey type is discouraged when flag is set
BOOST_AUTO_TEST_CASE(scriptpath_checksig_upgradable_pubkeytype_discouraged)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_CHECKSIG;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>{},
        std::vector<unsigned char>{0x02},
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        ctrl
    };

    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    const unsigned int flags = TAPROOT_FLAGS | SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              flags, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE);
}

// D.10 — Without discourage flag, unknown pubkey type in OP_CHECKSIG falls through to EVAL_FALSE
BOOST_AUTO_TEST_CASE(scriptpath_checksig_upgradable_pubkeytype_without_discourage_eval_false)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey internal_xonly = key.GetPubKey().GetXOnlyPubKey();

    CScript leaf_script;
    leaf_script << OP_CHECKSIG;
    uint256 leaf_hash = ComputeTapleafHashTest(TAPROOT_LEAF_TAPSCRIPT, leaf_script);
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(internal_xonly, &leaf_hash, output_key, parity));

    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;
    CMutableTransaction credit = BuildCreditingTransaction(spk, amount);
    CTransaction credit_tx(credit);
    CMutableTransaction spend = BuildSpendingTransaction(CScript(), CScriptWitness(), credit_tx);

    std::vector<unsigned char> ctrl = MakeControlBlock(TAPROOT_LEAF_TAPSCRIPT, parity, internal_xonly);
    spend.vin[0].scriptWitness.stack = {
        std::vector<unsigned char>{},
        std::vector<unsigned char>{0x02},
        std::vector<unsigned char>(leaf_script.begin(), leaf_script.end()),
        ctrl
    };

    CTransaction spend_tx(spend);
    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata(spend, spent_outs);
    MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[0].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_EVAL_FALSE);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================
// Suite E — Multi-input taproot transactions
// =============================================================================

BOOST_FIXTURE_TEST_SUITE(taproot_multiinput_tests, BasicTestingSetup)

// E.1 — spent_outputs count must equal vin.size() for m_bip341_taproot_ready
BOOST_AUTO_TEST_CASE(multiinput_spent_outputs_required)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction c0 = BuildCreditingTransaction(spk, amount);
    CMutableTransaction c1 = BuildCreditingTransaction(spk, amount);
    CTransaction ctx0(c0), ctx1(c1);

    CMutableTransaction spend;
    spend.nVersion = 2;
    CTxIn in0; in0.prevout = COutPoint(ctx0.GetHash(), 0); in0.nSequence = CTxIn::SEQUENCE_FINAL;
    CTxIn in1; in1.prevout = COutPoint(ctx1.GetHash(), 0); in1.nSequence = CTxIn::SEQUENCE_FINAL;
    spend.vin = {in0, in1};
    spend.vout = {CTxOut(amount * 2 - 1000, CScript())};

    std::vector<CTxOut> one_out; one_out.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata_one(spend, one_out);
    BOOST_CHECK(!txdata_one.m_bip341_taproot_ready);

    std::vector<CTxOut> two_outs;
    two_outs.push_back(CTxOut(amount, spk));
    two_outs.push_back(CTxOut(amount, spk));
    PrecomputedTransactionData txdata_two(spend, two_outs);
    BOOST_CHECK(txdata_two.m_bip341_taproot_ready);
}

// E.2 — Both inputs of a 2-input tx verify, sighashes differ by position
BOOST_AUTO_TEST_CASE(multiinput_both_signed_verified)
{
    CKey key_a = DecodeSecret(WIF_KEY);
    CKey key_b; key_b.MakeNewKey(true);
    BOOST_REQUIRE(key_a.IsValid() && key_b.IsValid());

    XOnlyPubKey xonly_a = key_a.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey xonly_b = key_b.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey out_a; bool par_a;
    XOnlyPubKey out_b; bool par_b;
    BOOST_REQUIRE(CreateTaprootOutput(xonly_a, nullptr, out_a, par_a));
    BOOST_REQUIRE(CreateTaprootOutput(xonly_b, nullptr, out_b, par_b));
    CScript spk_a = MakeTaprootSPK(out_a);
    CScript spk_b = MakeTaprootSPK(out_b);
    const CAmount amount = 1000000;

    CMutableTransaction ca = BuildCreditingTransaction(spk_a, amount);
    CMutableTransaction cb = BuildCreditingTransaction(spk_b, amount);
    CTransaction ctxa(ca), ctxb(cb);

    CMutableTransaction spend;
    spend.nVersion = 2;
    CTxIn in0; in0.prevout = COutPoint(ctxa.GetHash(), 0); in0.nSequence = CTxIn::SEQUENCE_FINAL;
    CTxIn in1; in1.prevout = COutPoint(ctxb.GetHash(), 0); in1.nSequence = CTxIn::SEQUENCE_FINAL;
    spend.vin = {in0, in1};
    spend.vout = {CTxOut(amount * 2 - 1000, CScript())};

    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk_a));
    spent_outs.push_back(CTxOut(amount, spk_b));
    const PrecomputedTransactionData txdata(spend, spent_outs);
    BOOST_REQUIRE(txdata.m_bip341_taproot_ready);

    uint256 sh0 = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    uint256 sh1 = SignatureHash(CScript(), spend, 1, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    BOOST_CHECK(sh0 != sh1);

    std::vector<unsigned char> sig_a, sig_b;
    BOOST_REQUIRE(key_a.SignSchnorrTaproot(sh0, sig_a, nullptr));
    BOOST_REQUIRE(key_b.SignSchnorrTaproot(sh1, sig_b, nullptr));

    spend.vin[0].scriptWitness.stack = {sig_a};
    spend.vin[1].scriptWitness.stack = {sig_b};
    CTransaction spend_tx(spend);

    ScriptError serror;
    {
        MutableTransactionSignatureChecker checker(&spend, 0, amount, txdata);
        BOOST_CHECK(VerifyScript(CScript(), spk_a, &spend_tx.vin[0].scriptWitness,
                                 TAPROOT_FLAGS, checker, &serror));
    }
    {
        MutableTransactionSignatureChecker checker(&spend, 1, amount, txdata);
        BOOST_CHECK(VerifyScript(CScript(), spk_b, &spend_tx.vin[1].scriptWitness,
                                 TAPROOT_FLAGS, checker, &serror));
    }
}

// E.3 — Reusing signature from input 0 for input 1 fails (position is committed)
BOOST_AUTO_TEST_CASE(multiinput_cross_signature_fails)
{
    CKey key = DecodeSecret(WIF_KEY);
    BOOST_REQUIRE(key.IsValid());
    XOnlyPubKey xonly = key.GetPubKey().GetXOnlyPubKey();
    XOnlyPubKey output_key; bool parity;
    BOOST_REQUIRE(CreateTaprootOutput(xonly, nullptr, output_key, parity));
    CScript spk = MakeTaprootSPK(output_key);
    const CAmount amount = 1000000;

    CMutableTransaction c0 = BuildCreditingTransaction(spk, amount);
    CMutableTransaction c1 = BuildCreditingTransaction(spk, amount);
    CTransaction ctx0(c0), ctx1(c1);

    CMutableTransaction spend;
    spend.nVersion = 2;
    CTxIn in0; in0.prevout = COutPoint(ctx0.GetHash(), 0); in0.nSequence = CTxIn::SEQUENCE_FINAL;
    CTxIn in1; in1.prevout = COutPoint(ctx1.GetHash(), 0); in1.nSequence = CTxIn::SEQUENCE_FINAL;
    spend.vin = {in0, in1};
    spend.vout = {CTxOut(amount * 2 - 1000, CScript())};

    std::vector<CTxOut> spent_outs;
    spent_outs.push_back(CTxOut(amount, spk));
    spent_outs.push_back(CTxOut(amount, spk));
    const PrecomputedTransactionData txdata(spend, spent_outs);

    uint256 sh0 = SignatureHash(CScript(), spend, 0, SIGHASH_DEFAULT, amount, SigVersion::TAPROOT, &txdata);
    std::vector<unsigned char> sig0;
    BOOST_REQUIRE(key.SignSchnorrTaproot(sh0, sig0, nullptr));

    spend.vin[0].scriptWitness.stack = {sig0};
    spend.vin[1].scriptWitness.stack = {sig0};  // reuse sig from input 0
    CTransaction spend_tx(spend);

    MutableTransactionSignatureChecker checker(&spend, 1, amount, txdata);
    ScriptError serror;
    BOOST_CHECK(!VerifyScript(CScript(), spk, &spend_tx.vin[1].scriptWitness,
                              TAPROOT_FLAGS, checker, &serror));
    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_SCHNORR_SIG);
}

BOOST_AUTO_TEST_SUITE_END()

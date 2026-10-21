// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <pubkey.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <cassert>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Keys and signatures: every signature made with a key verifies, and nothing crashes on garbage. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // Garbage public keys and signatures
    const std::vector<unsigned char> garbage_key = provider.ConsumeBytes<unsigned char>(provider.ConsumeBool() ? 33 : 65);
    CPubKey garbage(garbage_key);
    const uint256 hash = ConsumeUInt256(provider);
    const std::vector<unsigned char> garbage_sig = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 80));
    if (garbage.IsValid()) {
        (void)garbage.IsFullyValid();
        (void)garbage.Verify(hash, garbage_sig);
        CPubKey copy = garbage;
        (void)copy.Decompress();
        (void)copy.GetID();
    }
    CPubKey recovered;
    (void)recovered.RecoverCompact(hash, garbage_sig);
    const XOnlyPubKey xonly(provider.ConsumeBytes<unsigned char>(32));
    (void)xonly.IsFullyValid();
    (void)xonly.VerifySchnorr(hash, garbage_sig);

    const std::vector<unsigned char> secret = provider.ConsumeBytes<unsigned char>(32);
    if (secret.size() != 32) return;
    CKey key;
    key.Set(secret.begin(), secret.end(), provider.ConsumeBool());
    if (!key.IsValid()) return;

    const CPubKey pubkey = key.GetPubKey();
    assert(pubkey.IsFullyValid());
    assert(key.VerifyPubKey(pubkey));

    std::vector<unsigned char> sig;
    assert(key.Sign(hash, sig));
    assert(pubkey.Verify(hash, sig));
    uint256 other = hash;
    *other.begin() ^= 1;
    assert(!pubkey.Verify(other, sig));

    std::vector<unsigned char> compact;
    assert(key.SignCompact(hash, compact));
    CPubKey from_compact;
    assert(from_compact.RecoverCompact(hash, compact));
    assert(from_compact == pubkey);

    std::vector<unsigned char> schnorr;
    assert(key.SignSchnorr(hash, schnorr));
    assert(pubkey.GetXOnlyPubKey().VerifySchnorr(hash, schnorr));

    // BIP32: a child of a key and of its public key agree (non-hardened)
    ChainCode chain_code = ConsumeUInt256(provider);
    const unsigned int n = provider.ConsumeIntegralInRange<unsigned int>(0, 0x7fffffff);
    CKey child_key;
    ChainCode child_chain_code;
    CPubKey child_pubkey;
    ChainCode child_pub_chain_code;
    if (key.Derive(child_key, child_chain_code, n, chain_code) && pubkey.Derive(child_pubkey, child_pub_chain_code, n, chain_code)) {
        assert(child_key.GetPubKey() == child_pubkey);
        assert(child_chain_code == child_pub_chain_code);
    }
}

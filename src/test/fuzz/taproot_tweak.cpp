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

/**
 * BIP341 output key tweaking, for any internal key and merkle root.
 * A key path signature made with the tweaked private key must verify against the
 * output key that CreatePayToTaprootPubKey gives for the same internal key and root.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    const std::vector<unsigned char> internal_bytes = provider.ConsumeBytes<unsigned char>(32);
    const XOnlyPubKey internal(internal_bytes);
    const bool has_root = provider.ConsumeBool();
    const uint256 root = ConsumeUInt256(provider);
    const uint256* root_ptr = has_root ? &root : nullptr;

    // Never crashes, even for 32 bytes that aren't a point
    const std::pair<XOnlyPubKey, bool> output = internal.CreatePayToTaprootPubKey(root_ptr);
    assert(internal.ComputeTapTweak(root_ptr) == internal.ComputeTapTweak(root_ptr));
    if (internal.IsFullyValid()) {
        assert(output.first.IsFullyValid());
    }

    const std::vector<unsigned char> secret = provider.ConsumeBytes<unsigned char>(32);
    CKey key;
    if (secret.size() != 32) return;
    key.Set(secret.begin(), secret.end(), true);
    if (!key.IsValid()) return;

    const XOnlyPubKey key_internal = key.GetPubKey().GetXOnlyPubKey();
    assert(key_internal.IsFullyValid());
    const std::pair<XOnlyPubKey, bool> key_output = key_internal.CreatePayToTaprootPubKey(root_ptr);
    assert(key_output.first.IsFullyValid());

    const uint256 hash = ConsumeUInt256(provider);
    std::vector<unsigned char> sig;
    if (!key.SignSchnorrTaproot(hash, sig, root_ptr)) return;
    assert(sig.size() == XOnlyPubKey::SCHNORR_SIGNATURE_SIZE);
    assert(key_output.first.VerifySchnorr(hash, sig));
    // ... and not against another message
    uint256 other = hash;
    *other.begin() ^= 1;
    assert(!key_output.first.VerifySchnorr(other, sig));
}

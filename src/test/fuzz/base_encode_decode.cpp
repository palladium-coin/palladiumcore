// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

/** base58, base58check, base32 and base64: encode and decode are inverse. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());

    std::vector<unsigned char> decoded;
    if (DecodeBase58(s, decoded, 100)) {
        assert(DecodeBase58(EncodeBase58(decoded), decoded, 100) || decoded.size() > 100);
    }
    if (DecodeBase58Check(s, decoded, 100)) {
        std::vector<unsigned char> again;
        assert(DecodeBase58Check(EncodeBase58Check(decoded), again, 100));
        assert(again == decoded);
    }

    bool invalid = false;
    const std::vector<unsigned char> b32 = DecodeBase32(s.c_str(), &invalid);
    if (!invalid) {
        const std::string re = EncodeBase32(b32.data(), b32.size());
        bool invalid2 = false;
        assert(DecodeBase32(re.c_str(), &invalid2) == b32 && !invalid2);
    }
    invalid = false;
    const std::vector<unsigned char> b64 = DecodeBase64(s.c_str(), &invalid);
    if (!invalid) {
        bool invalid2 = false;
        assert(DecodeBase64(EncodeBase64(b64.data(), b64.size()).c_str(), &invalid2) == b64 && !invalid2);
    }

    // The raw bytes round-trip through every encoder
    assert(DecodeBase58(EncodeBase58(buffer), decoded, buffer.size() + 1) && decoded == buffer);
    assert(DecodeBase58Check(EncodeBase58Check(buffer), decoded, buffer.size() + 1) && decoded == buffer);
    bool invalid3 = false;
    assert(DecodeBase32(EncodeBase32(buffer.data(), buffer.size()).c_str(), &invalid3) == buffer && !invalid3);
    assert(DecodeBase64(EncodeBase64(buffer.data(), buffer.size()).c_str(), &invalid3) == buffer && !invalid3);
}

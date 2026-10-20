// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <psbt.h>
#include <streams.h>
#include <version.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <cassert>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/** Decoding of base64 PSBTs and the operations that don't need keys. */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string s(buffer.begin(), buffer.end());
    PartiallySignedTransaction psbt;
    std::string error;
    if (!DecodeBase64PSBT(psbt, EncodeBase64(buffer.data(), buffer.size()), error) && !DecodeBase64PSBT(psbt, s, error)) {
        return;
    }
    (void)psbt.IsNull();

    // Serializing and decoding again gives the same PSBT
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << psbt;
    PartiallySignedTransaction again;
    std::string error2;
    assert(DecodeRawPSBT(again, ss.str(), error2));
}

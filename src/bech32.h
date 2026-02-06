// Copyright (c) 2017 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Bech32 is a string encoding format used in newer address types.
// The output consists of a human-readable part (alphanumeric), a
// separator character (1), and a base32 data section, the last
// 6 characters of which are a checksum.
//
// For more information, see BIP 173.

#ifndef PALLADIUM_BECH32_H
#define PALLADIUM_BECH32_H

#include <stdint.h>
#include <string>
#include <vector>

namespace bech32
{

/** The Bech32 and Bech32m checksum encodings. */
enum class Encoding {
    INVALID,
    BECH32,
    BECH32M,
};

/** Encode a Bech32 or Bech32m string. If hrp contains uppercase characters, this will cause an assertion error. */
std::string Encode(Encoding encoding, const std::string& hrp, const std::vector<uint8_t>& values);

/** Legacy Bech32-only encoder for callers that don't pass an encoding type. */
std::string Encode(const std::string& hrp, const std::vector<uint8_t>& values);

/** Decode a Bech32 string. Returns (hrp, data). Empty hrp means failure. */
std::pair<std::string, std::vector<uint8_t>> Decode(const std::string& str);

/** Decode a Bech32 or Bech32m string, returning the checksum encoding used. */
Encoding Decode(const std::string& str, std::string& out_hrp, std::vector<uint8_t>& out_values);

} // namespace bech32

#endif // PALLADIUM_BECH32_H

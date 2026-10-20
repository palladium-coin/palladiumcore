// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <net.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <version.h>

#include <algorithm>
#include <cstring>
#include <hash.h>
#include <string>
#include <vector>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup;
}

/**
 * The deserializer of the P2P wire format, fed in chunks of any size. Half of the inputs are
 * well framed messages (right magic, size and checksum), possibly with one corrupted byte,
 * so that the fuzzer gets past the header checks.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    V1TransportDeserializer deserializer{Params().MessageStart(), SER_NETWORK, INIT_PROTO_VERSION};
    deserializer.SetVersion(INIT_PROTO_VERSION);

    std::vector<char> stream;
    if (provider.ConsumeBool()) {
        const std::vector<std::string>& types = getAllNetMessageTypes();
        const std::string command = types[provider.ConsumeIntegralInRange<size_t>(0, types.size() - 1)];
        const std::vector<unsigned char> payload = provider.ConsumeBytes<unsigned char>(provider.ConsumeIntegralInRange<size_t>(0, 200));
        CMessageHeader header(Params().MessageStart(), command.c_str(), payload.size());
        const uint256 hash = Hash(payload.begin(), payload.end());
        memcpy(header.pchChecksum, hash.begin(), CMessageHeader::CHECKSUM_SIZE);
        CDataStream ss(SER_NETWORK, INIT_PROTO_VERSION);
        ss << header;
        stream.assign(ss.begin(), ss.end());
        stream.insert(stream.end(), payload.begin(), payload.end());
        if (provider.ConsumeBool() && !stream.empty()) {
            stream[provider.ConsumeIntegralInRange<size_t>(0, stream.size() - 1)] ^= provider.ConsumeIntegral<uint8_t>();
        }
    } else {
        stream = provider.ConsumeRemainingBytes<char>();
    }

    size_t pos = 0;
    while (pos < stream.size()) {
        const size_t chunk = std::min<size_t>(stream.size() - pos, provider.ConsumeIntegralInRange<size_t>(1, 64));
        const int consumed = deserializer.Read(stream.data() + pos, chunk);
        if (consumed < 0) return; // invalid header: the connection would be closed
        pos += consumed > 0 ? consumed : chunk;
        if (deserializer.Complete()) {
            const CNetMessage msg = deserializer.GetMessage(Params().MessageStart(), 0);
            (void)msg.m_valid_header;
            (void)msg.m_valid_netmagic;
            (void)msg.m_valid_checksum;
            (void)msg.m_command;
        }
    }
}

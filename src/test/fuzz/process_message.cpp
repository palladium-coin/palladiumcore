// Copyright (c) 2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <chainparams.h>
#include <net.h>
#include <net_processing.h>
#include <protocol.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <validationinterface.h>
#include <version.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

bool ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CTxMemPool& mempool, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc);

namespace {
// Never destroyed, see initialize()
const TestingSetup* g_setup{nullptr};
NodeId g_next_node_id = 0;
} // namespace

void initialize()
{
    // Regtest: valid proof of work is cheap, so blocks and headers get past the first checks
    g_setup = new const TestingSetup(CBaseChainParams::REGTEST);
    // The scheduler thread of the setup is still running when the process exits, and the destructors of
    // globals of the node (condition variables, mutexes) then assert. Skip them: this runs before the
    // destructors of everything created earlier.
    std::atexit([] { std::_Exit(0); });
}

/**
 * Feed one network message of a known type, with fuzzed contents, to a connected peer.
 * Whatever the peer sends, the node must not crash.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::vector<std::string>& types = getAllNetMessageTypes();
    const std::string msg_type = types[provider.ConsumeIntegralInRange<size_t>(0, types.size() - 1)];
    CDataStream payload(provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION);

    CConnman& connman = *g_setup->m_node.connman;
    CNode node{g_next_node_id++,
               ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_BLOOM),
               /* nMyStartingHeightIn */ 0,
               INVALID_SOCKET,
               CAddress{CService{in_addr{0x0100007f}, 7777}, NODE_NETWORK},
               /* nKeyedNetGroupIn */ 0,
               /* nLocalHostNonceIn */ 0,
               CAddress{},
               std::string{},
               /* fInboundIn */ false};
    node.SetSendVersion(PROTOCOL_VERSION);
    g_setup->m_node.peer_logic->InitializeNode(&node);
    node.fSuccessfullyConnected = true;
    node.nVersion = PROTOCOL_VERSION;

    const std::atomic<bool> interrupt{false};
    try {
        (void)ProcessMessage(&node, msg_type, payload, GetTimeMicros(), Params(), *g_setup->m_node.mempool, &connman, g_setup->m_node.banman.get(), interrupt);
    } catch (const std::ios_base::failure&) {
        // Malformed payloads make deserialization throw; the real message loop catches it too
    }
    SyncWithValidationInterfaceQueue();

    bool update_connection_time = false;
    g_setup->m_node.peer_logic->FinalizeNode(node.GetId(), update_connection_time);
}

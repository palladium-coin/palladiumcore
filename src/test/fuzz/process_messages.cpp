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
    g_setup = new const TestingSetup(CBaseChainParams::REGTEST);
    std::atexit([] { std::_Exit(0); });
}

/**
 * A whole conversation with one peer: a few messages in a row, the first usually the handshake.
 * The messages of the process_message target start from a connected peer; here the state that
 * builds up between messages (version, verack, headers, inventory, bloom filter) is exercised.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CConnman& connman = *g_setup->m_node.connman;
    const bool inbound = provider.ConsumeBool();
    CNode node{g_next_node_id++,
               ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_BLOOM),
               /* nMyStartingHeightIn */ 0,
               INVALID_SOCKET,
               CAddress{CService{in_addr{0x0100007f}, 7777}, NODE_NETWORK},
               /* nKeyedNetGroupIn */ 0,
               /* nLocalHostNonceIn */ 0,
               CAddress{},
               std::string{},
               inbound};
    g_setup->m_node.peer_logic->InitializeNode(&node);

    const std::vector<std::string>& types = getAllNetMessageTypes();
    const std::atomic<bool> interrupt{false};
    const size_t num_messages = provider.ConsumeIntegralInRange<size_t>(1, 8);
    for (size_t i = 0; i < num_messages && provider.remaining_bytes() > 0; ++i) {
        const std::string& type = types[provider.ConsumeIntegralInRange<size_t>(0, types.size() - 1)];
        const size_t len = provider.ConsumeIntegralInRange<size_t>(0, std::min<size_t>(provider.remaining_bytes(), 600));
        CDataStream payload(provider.ConsumeBytes<unsigned char>(len), SER_NETWORK, PROTOCOL_VERSION);
        try {
            (void)ProcessMessage(&node, type, payload, GetTimeMicros(), Params(), *g_setup->m_node.mempool, &connman, g_setup->m_node.banman.get(), interrupt);
        } catch (const std::ios_base::failure&) {
        }
        if (node.fDisconnect) break;
    }
    SyncWithValidationInterfaceQueue();

    bool update_connection_time = false;
    g_setup->m_node.peer_logic->FinalizeNode(node.GetId(), update_connection_time);
}

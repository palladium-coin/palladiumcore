// Copyright (c) 2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

namespace {
// Never destroyed, see initialize()
const TestingSetup* g_setup{nullptr};

// Commands that end the process, block, mine, or touch the disk and the network in ways that are
// not part of what is being tested here
const std::set<std::string> SKIPPED{
    "stop",
    "generate",
    "generateblock",
    "generatetoaddress",
    "generatetodescriptor",
    "setgenerate",
    "invalidateblock",
    "reconsiderblock",
    "pruneblockchain",
    "savemempool",
    "dumptxoutset",
    "loadtxoutset",
    "verifychain",
    "scantxoutset",
    "gettxoutsetinfo",
    "addnode",
    "setnetworkactive",
    "disconnectnode",
    "setban",
    "submitblock",
    "submitheader",
    "importmempool",
    "logging",
    "setmocktime",
    "echoipc",
    "uptime",
    "waitforblock",
    "waitforblockheight",
    "waitfornewblock",
};

std::string ConsumeParam(FuzzedDataProvider& provider)
{
    switch (provider.ConsumeIntegralInRange<int>(0, 7)) {
    case 0: return provider.ConsumeRandomLengthString(64);
    case 1: return std::to_string(provider.ConsumeIntegral<int64_t>());
    case 2: return provider.ConsumeBool() ? "true" : "false";
    case 3: return "\"" + provider.ConsumeRandomLengthString(40) + "\"";
    case 4: return "[" + provider.ConsumeRandomLengthString(40) + "]";
    case 5: return "\"" + HexStr(provider.ConsumeBytes<uint8_t>(32)) + "\"";
    case 6: return "\"" + HexStr(provider.ConsumeRandomLengthString(80)) + "\"";
    default: return "{" + provider.ConsumeRandomLengthString(40) + "}";
    }
}
} // namespace

void initialize()
{
    g_setup = new const TestingSetup(CBaseChainParams::REGTEST);
    std::atexit([] { std::_Exit(0); });
}

/**
 * Call any registered RPC with fuzzed parameters. Errors are fine (they come back as exceptions of
 * type UniValue or std::runtime_error), anything else is a bug.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::vector<std::string> commands = tableRPC.listCommands();
    const std::string& method = commands[provider.ConsumeIntegralInRange<size_t>(0, commands.size() - 1)];
    if (SKIPPED.count(method)) return;

    std::vector<std::string> params;
    const size_t num_params = provider.ConsumeIntegralInRange<size_t>(0, 6);
    for (size_t i = 0; i < num_params; ++i) {
        params.push_back(ConsumeParam(provider));
    }

    JSONRPCRequest request;
    request.strMethod = method;
    try {
        request.params = RPCConvertValues(method, params);
    } catch (const std::runtime_error&) {
        // Parameter that is not valid JSON
        return;
    }
    if (!request.params.isArray()) return;
    request.fHelp = false;

    try {
        (void)tableRPC.execute(request);
    } catch (const UniValue&) {
        // JSONRPCError
    } catch (const std::runtime_error&) {
        // includes std::ios_base::failure
    }
}

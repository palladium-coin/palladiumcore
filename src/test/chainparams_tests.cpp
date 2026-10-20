// Copyright (c) 2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <consensus/params.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <util/system.h>
#include <validation.h>

#include <algorithm>
#include <limits>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(chainparams_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(mainnet_params)
{
    const auto params = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& c = params->GetConsensus();

    BOOST_CHECK_EQUAL(params->GetDefaultPort(), 2333);
    BOOST_CHECK_EQUAL(params->Bech32HRP(), "plm");
    BOOST_CHECK_EQUAL(params->MessageStart()[0], 0xfa);
    BOOST_CHECK_EQUAL(params->MessageStart()[1], 0xc7);
    BOOST_CHECK_EQUAL(params->MessageStart()[2], 0xb2);
    BOOST_CHECK_EQUAL(params->MessageStart()[3], 0xda);
    BOOST_CHECK(params->RequireStandard());
    BOOST_CHECK(!params->MineBlocksOnDemand());

    BOOST_CHECK_EQUAL(c.nResilienceForkHeight, 340000);
    BOOST_CHECK_EQUAL(c.TaprootHeight, 405000);
    BOOST_CHECK_EQUAL(c.SegwitHeight, 29000);
    BOOST_CHECK_EQUAL(c.BIP34Height, 29000);
    BOOST_CHECK_EQUAL(c.BIP65Height, 29000);
    BOOST_CHECK_EQUAL(c.BIP66Height, 29000);
    BOOST_CHECK_EQUAL(c.CSVHeight, 29000);

    // The legacy values must not change on mainnet: they are used for historical validation
    BOOST_CHECK_EQUAL(c.nPowTargetSpacing, 10 * 60);
    BOOST_CHECK_EQUAL(c.nPowTargetTimespan, 14 * 24 * 60 * 60);
    BOOST_CHECK_EQUAL(c.nPowTargetSpacingV2, 2 * 60);
    BOOST_CHECK_EQUAL(c.DifficultyAdjustmentInterval(), 2016);
    BOOST_CHECK(!c.fPowAllowMinDifficultyBlocks);
    BOOST_CHECK(!c.fPowNoRetargeting);

    BOOST_CHECK_EQUAL(c.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit, 2);
    BOOST_CHECK_EQUAL(c.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime, 1771027200);
    BOOST_CHECK_EQUAL(c.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout, 1803859200);
}

BOOST_AUTO_TEST_CASE(testnet_params)
{
    const auto params = CreateChainParams(CBaseChainParams::TESTNET);
    const Consensus::Params& c = params->GetConsensus();

    BOOST_CHECK_EQUAL(params->GetDefaultPort(), 12333);
    BOOST_CHECK_EQUAL(params->Bech32HRP(), "tplm");
    BOOST_CHECK_EQUAL(params->MessageStart()[0], 0x0c);
    BOOST_CHECK_EQUAL(params->MessageStart()[1], 0x12);
    BOOST_CHECK_EQUAL(params->MessageStart()[2], 0x0a);
    BOOST_CHECK_EQUAL(params->MessageStart()[3], 0x08);

    BOOST_CHECK_EQUAL(c.nResilienceForkHeight, 2000);
    BOOST_CHECK_EQUAL(c.TaprootHeight, 2500);
    // LWMA from genesis: min-difficulty rule on, spacing is the post-fork one from the start
    BOOST_CHECK(c.fPowAllowMinDifficultyBlocks);
    BOOST_CHECK(!c.fPowNoRetargeting);
    BOOST_CHECK_EQUAL(c.nPowTargetSpacing, 2 * 60);
    BOOST_CHECK_EQUAL(c.nPowTargetSpacingV2, 2 * 60);
}

BOOST_AUTO_TEST_CASE(regtest_params)
{
    const auto params = CreateChainParams(CBaseChainParams::REGTEST);
    const Consensus::Params& c = params->GetConsensus();

    BOOST_CHECK_EQUAL(params->GetDefaultPort(), 28444);
    BOOST_CHECK_EQUAL(params->Bech32HRP(), "rplm");
    BOOST_CHECK_EQUAL(params->MessageStart()[0], 0xfa);
    BOOST_CHECK_EQUAL(params->MessageStart()[1], 0xbf);
    BOOST_CHECK_EQUAL(params->MessageStart()[2], 0xb5);
    BOOST_CHECK_EQUAL(params->MessageStart()[3], 0xda);
    BOOST_CHECK(params->MineBlocksOnDemand());

    BOOST_CHECK_EQUAL(c.nResilienceForkHeight, 200);
    BOOST_CHECK_EQUAL(c.TaprootHeight, 0);
    BOOST_CHECK(c.fPowNoRetargeting);
    BOOST_CHECK_EQUAL(c.nPowTargetSpacing, 2 * 60);
    BOOST_CHECK_EQUAL(c.nPowTargetSpacingV2, 2 * 60);
}

BOOST_AUTO_TEST_CASE(base_params_rpc_ports)
{
    BOOST_CHECK_EQUAL(CreateBaseChainParams(CBaseChainParams::MAIN)->RPCPort(), 2332);
    BOOST_CHECK_EQUAL(CreateBaseChainParams(CBaseChainParams::TESTNET)->RPCPort(), 12332);
    BOOST_CHECK_EQUAL(CreateBaseChainParams(CBaseChainParams::REGTEST)->RPCPort(), 28443);
}

BOOST_AUTO_TEST_CASE(networks_do_not_share_identifiers)
{
    const auto main = CreateChainParams(CBaseChainParams::MAIN);
    const auto test = CreateChainParams(CBaseChainParams::TESTNET);
    const auto reg = CreateChainParams(CBaseChainParams::REGTEST);

    BOOST_CHECK(main->GetDefaultPort() != test->GetDefaultPort());
    BOOST_CHECK(main->GetDefaultPort() != reg->GetDefaultPort());
    BOOST_CHECK(test->GetDefaultPort() != reg->GetDefaultPort());
    BOOST_CHECK(main->Bech32HRP() != test->Bech32HRP());
    BOOST_CHECK(main->Bech32HRP() != reg->Bech32HRP());
    BOOST_CHECK(test->Bech32HRP() != reg->Bech32HRP());
    BOOST_CHECK(!std::equal(main->MessageStart(), main->MessageStart() + CMessageHeader::MESSAGE_START_SIZE, test->MessageStart()));
    BOOST_CHECK(!std::equal(main->MessageStart(), main->MessageStart() + CMessageHeader::MESSAGE_START_SIZE, reg->MessageStart()));
    BOOST_CHECK(!std::equal(test->MessageStart(), test->MessageStart() + CMessageHeader::MESSAGE_START_SIZE, reg->MessageStart()));
}

BOOST_AUTO_TEST_CASE(regtest_taprootheight_arg)
{
    gArgs.ForceSetArg("-taprootheight", "123");
    BOOST_CHECK_EQUAL(CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus().TaprootHeight, 123);

    // -1 disables Taproot
    gArgs.ForceSetArg("-taprootheight", "-1");
    BOOST_CHECK_EQUAL(CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus().TaprootHeight, std::numeric_limits<int>::max());

    // Out of range values are rejected
    gArgs.ForceSetArg("-taprootheight", "-2");
    BOOST_CHECK_THROW(CreateChainParams(CBaseChainParams::REGTEST), std::runtime_error);
    gArgs.ForceSetArg("-taprootheight", strprintf("%d", std::numeric_limits<int>::max()));
    BOOST_CHECK_THROW(CreateChainParams(CBaseChainParams::REGTEST), std::runtime_error);

    // 0 is the regtest default; the argument is only meaningful for regtest
    gArgs.ForceSetArg("-taprootheight", "0");
    BOOST_CHECK_EQUAL(CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus().TaprootHeight, 0);
    BOOST_CHECK_EQUAL(CreateChainParams(CBaseChainParams::MAIN)->GetConsensus().TaprootHeight, 405000);
}

BOOST_AUTO_TEST_SUITE_END()

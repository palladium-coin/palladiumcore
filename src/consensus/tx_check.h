// Copyright (c) 2009-2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_CONSENSUS_TX_CHECK_H
#define PALLADIUM_CONSENSUS_TX_CHECK_H

/**
 * Context-independent transaction checking code that can be called outside the
 * palladium server and doesn't depend on chain or mempool state. Transaction
 * verification code that does call server functions or depend on server state
 * belongs in tx_verify.h/cpp instead.
 */

class CTransaction;
class TxValidationState;

bool CheckTransaction(const CTransaction& tx, TxValidationState& state);

#endif // PALLADIUM_CONSENSUS_TX_CHECK_H

// Copyright (c) 2009-2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_ZMQ_ZMQCONFIG_H
#define PALLADIUM_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include <config/palladium-config.h>
#endif

#include <stdarg.h>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include <primitives/transaction.h>

void zmqError(const char *str);

#endif // PALLADIUM_ZMQ_ZMQCONFIG_H

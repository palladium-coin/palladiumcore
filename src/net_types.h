// Copyright (c) 2009-2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_NET_TYPES_H
#define PALLADIUM_NET_TYPES_H

#include <map>

class CBanEntry;
class CSubNet;

using banmap_t = std::map<CSubNet, CBanEntry>;

#endif // PALLADIUM_NET_TYPES_H

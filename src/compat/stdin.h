// Copyright (c) 2009-2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_COMPAT_STDIN_H
#define PALLADIUM_COMPAT_STDIN_H

struct NoechoInst {
    NoechoInst();
    ~NoechoInst();
};

#define NO_STDIN_ECHO() NoechoInst _no_echo

bool StdinTerminal();
bool StdinReady();

#endif // PALLADIUM_COMPAT_STDIN_H

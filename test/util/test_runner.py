#!/usr/bin/env python3
# Copyright (c) 2026 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run util test scripts."""

import os
import subprocess
import sys


def run(cmd):
    print("Running:", " ".join(cmd), flush=True)
    return subprocess.call(cmd)


def main():
    base = os.path.dirname(__file__)
    tests = [
        [sys.executable, os.path.join(base, "palladium-util-test.py")],
        [sys.executable, os.path.join(base, "rpcauth-test.py")],
    ]

    exit_code = 0
    for cmd in tests:
        rc = run(cmd)
        if rc != 0:
            exit_code = rc

    sys.exit(exit_code)


if __name__ == "__main__":
    main()

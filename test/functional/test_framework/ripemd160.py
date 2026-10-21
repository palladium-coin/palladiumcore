#!/usr/bin/env python3
# Copyright (c) 2021 The Palladium Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Pure Python RIPEMD-160, for systems whose OpenSSL no longer provides it (OpenSSL 3).

Only used when hashlib does not support 'ripemd160'. It is slow and not constant time: test use only.
"""

# Message word selection for the left and right lines
ML = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
    3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
    1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
    4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13,
]
MR = [
    5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
    6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
    15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
    8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
    12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11,
]
# Rotation amounts
SL = [
    11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
    7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
    11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
    11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
    9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6,
]
SR = [
    8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
    9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
    9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
    15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
    8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11,
]
KL = [0x00000000, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xA953FD4E]
KR = [0x50A28BE6, 0x5C4DD124, 0x6D703EF3, 0x7A6D76E9, 0x00000000]
MASK = 0xFFFFFFFF


def _rol(x, n):
    return ((x << n) | (x >> (32 - n))) & MASK


def _f(round_index, x, y, z):
    if round_index == 0:
        return x ^ y ^ z
    if round_index == 1:
        return (x & y) | (~x & MASK & z)
    if round_index == 2:
        return (x | (~y & MASK)) ^ z
    if round_index == 3:
        return (x & z) | (y & (~z & MASK))
    return x ^ (y | (~z & MASK))


def _compress(state, block):
    words = [int.from_bytes(block[4 * i:4 * i + 4], 'little') for i in range(16)]
    al, bl, cl, dl, el = state
    ar, br, cr, dr, er = state
    for j in range(80):
        rnd = j // 16
        t = (_rol((al + _f(rnd, bl, cl, dl) + words[ML[j]] + KL[rnd]) & MASK, SL[j]) + el) & MASK
        al, bl, cl, dl, el = el, t, bl, _rol(cl, 10), dl
        t = (_rol((ar + _f(4 - rnd, br, cr, dr) + words[MR[j]] + KR[rnd]) & MASK, SR[j]) + er) & MASK
        ar, br, cr, dr, er = er, t, br, _rol(cr, 10), dr
    t = (state[1] + cl + dr) & MASK
    return [t,
            (state[2] + dl + er) & MASK,
            (state[3] + el + ar) & MASK,
            (state[4] + al + br) & MASK,
            (state[0] + bl + cr) & MASK]


def ripemd160(data):
    """Return the RIPEMD-160 digest of the bytes `data`."""
    state = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0]
    padded = data + b'\x80' + b'\x00' * ((55 - len(data)) % 64) + (8 * len(data)).to_bytes(8, 'little')
    for i in range(0, len(padded), 64):
        state = _compress(state, padded[i:i + 64])
    return b''.join(word.to_bytes(4, 'little') for word in state)

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>
#include <util/string.h>

#include <tinyformat.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <limits>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
    CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%", // SAFE_CHARS_URI
};

std::string SanitizeString(const std::string& str, int rule)
{
    std::string strResult;
    for (const unsigned char c : str) {
        if (SAFE_CHARS[rule].find(c) != std::string::npos) {
            strResult.push_back(c);
        }
    }
    return strResult;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

bool IsHex(const std::string& str)
{
    for(std::string::size_type i = 0; i < str.size(); i++)
    {
        if (HexDigit(str[i]) < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

std::vector<unsigned char> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<unsigned char> vch;
    while (true)
    {
        while (isspace(*psz))
            psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        unsigned char n = (c << 4);
        c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break; // incomplete byte
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

std::vector<unsigned char> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

static bool PrecisionValid(const std::string& str)
{
    bool sign = false;
    bool decimal = false;
    bool digit = false;
    for (auto const& c : str) {
        if (c == '-' || c == '+') {
            if (sign || decimal || digit) return false;
            sign = true;
        } else if (c == '.') {
            if (decimal) return false;
            decimal = true;
        } else if (c >= '0' && c <= '9') {
            digit = true;
        } else {
            return false;
        }
    }
    return true;
}

bool ParseInt32(const std::string& str, int32_t *out)
{
    char *endp = nullptr;
    errno = 0; // strtol will not set errno if valid
    long int n = strtol(str.c_str(), &endp, 10);
    if(out) *out = (int32_t)n;
    // Note: the check for n > INT_MAX is not strictly needed on most
    // platforms, because long int is often 32-bit. However, on
    // platforms where long int is 64-bit, we need this check.
    return endp && *endp == 0 && !str.empty() &&
           errno == 0 && n >= std::numeric_limits<int32_t>::min() &&
           n <= std::numeric_limits<int32_t>::max();
}

bool ParseInt64(const std::string& str, int64_t *out)
{
    char *endp = nullptr;
    errno = 0; // strtoll will not set errno if valid
    long long int n = strtoll(str.c_str(), &endp, 10);
    if(out) *out = (int64_t)n;
    return endp && *endp == 0 && !str.empty() &&
           errno == 0 && n >= std::numeric_limits<int64_t>::min() &&
           n <= std::numeric_limits<int64_t>::max();
}

bool ParseUint32(const std::string& str, uint32_t *out)
{
    char *endp = nullptr;
    errno = 0; // strtoul will not set errno if valid
    unsigned long int n = strtoul(str.c_str(), &endp, 10);
    if(out) *out = (uint32_t)n;
    if (str.size() > 0 && str[0] == '-') return false; // Reject negative numbers
    return endp && *endp == 0 && !str.empty() &&
           errno == 0 && n <= std::numeric_limits<uint32_t>::max();
}

bool ParseUint64(const std::string& str, uint64_t *out)
{
    char *endp = nullptr;
    errno = 0; // strtoull will not set errno if valid
    unsigned long long int n = strtoull(str.c_str(), &endp, 10);
    if(out) *out = (uint64_t)n;
    if (str.size() > 0 && str[0] == '-') return false; // Reject negative numbers
    return endp && *endp == 0 && !str.empty() &&
           errno == 0 && n <= std::numeric_limits<uint64_t>::max();
}

bool ParseDouble(const std::string& str, double *out)
{
    if (!PrecisionValid(str)) return false;
    char *endp = nullptr;
    errno = 0; // strtod will not set errno if valid
    double n = strtod(str.c_str(), &endp);
    if(out) *out = n;
    return endp && *endp == 0 && !str.empty() && errno == 0;
}

std::string EncodeBase64(const unsigned char* pch, size_t len)
{
    static const char* pary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string strRet;
    strRet.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        int n = pch[i] << 16;
        if (i + 1 < len)
            n |= pch[i + 1] << 8;
        if (i + 2 < len)
            n |= pch[i + 2];

        strRet.push_back(pary[(n >> 18) & 63]);
        strRet.push_back(pary[(n >> 12) & 63]);
        if (i + 1 < len)
            strRet.push_back(pary[(n >> 6) & 63]);
        else
            strRet.push_back('=');
        if (i + 2 < len)
            strRet.push_back(pary[n & 63]);
        else
            strRet.push_back('=');
    }
    return strRet;
}

std::string EncodeBase64(const std::string& str)
{
    return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid)
{
    static const int decode64_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (pfInvalid)
        *pfInvalid = false;

    std::vector<unsigned char> vchRet;
    while (*p)
    {
        int x = decode64_table[(unsigned char)*p++];
        if (x == -1) continue;
        int y = decode64_table[(unsigned char)*p++];
        if (y == -1) break;
        vchRet.push_back((x << 2) | (y >> 4));
        int z = decode64_table[(unsigned char)*p++];
        if (z == -1) break;
        vchRet.push_back(((y << 4) & 0xff) | (z >> 2));
        int w = decode64_table[(unsigned char)*p++];
        if (w == -1) break;
        vchRet.push_back(((z << 6) & 0xff) | w);
    }

    if (*p && pfInvalid)
        *pfInvalid = true;

    return vchRet;
}

std::vector<unsigned char> DecodeBase64(const std::string& str, bool* pfInvalid)
{
    return DecodeBase64(str.c_str(), pfInvalid);
}

std::string EncodeBase32(const unsigned char* pch, size_t len)
{
    static const char* pary = "abcdefghijklmnopqrstuvwxyz234567";
    std::string strRet;
    strRet.reserve(((len + 4) / 5) * 8);
    int mode = 0;
    int left = 0;
    for (size_t i = 0; i < len; i++)
    {
        int ch = pch[i];
        switch (mode)
        {
        case 0:
            strRet.push_back(pary[(ch >> 3) & 31]);
            left = (ch & 7) << 2;
            mode = 1;
            break;
        case 1:
            strRet.push_back(pary[left | (ch >> 6)]);
            strRet.push_back(pary[(ch >> 1) & 31]);
            left = (ch & 1) << 4;
            mode = 2;
            break;
        case 2:
            strRet.push_back(pary[left | (ch >> 4)]);
            left = (ch & 15) << 1;
            mode = 3;
            break;
        case 3:
            strRet.push_back(pary[left | (ch >> 7)]);
            strRet.push_back(pary[(ch >> 2) & 31]);
            left = (ch & 3) << 3;
            mode = 4;
            break;
        case 4:
            strRet.push_back(pary[left | (ch >> 5)]);
            strRet.push_back(pary[ch & 31]);
            mode = 0;
            break;
        }
    }
    static const int nModePadding[] = {0, 6, 4, 3, 1};
    int nPadding = nModePadding[mode];
    left <<= (8 - (5 * mode) % 8) % 5;
    if (mode != 0)
        strRet.push_back(pary[left]);
    for (int n = 0; n < nPadding; n++)
        strRet.push_back('=');
    return strRet;
}

std::string EncodeBase32(const std::string& str)
{
    return EncodeBase32((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid)
{
    static const int decode32_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
        3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (pfInvalid)
        *pfInvalid = false;

    std::vector<unsigned char> vchRet;
    int nBits = 0;
    int nAcc = 0;
    while (*p)
    {
        if (*p == '=') break;
        int x = decode32_table[(unsigned char)*p++];
        if (x == -1) continue;
        nAcc = (nAcc << 5) | x;
        nBits += 5;
        if (nBits >= 8)
        {
            vchRet.push_back(nAcc >> (nBits - 8));
            nBits -= 8;
        }
    }

    if (pfInvalid && (*p == '='))
    {
        while (*p == '=') p++;
        while (*p)
        {
            if (decode32_table[(unsigned char)*p++] != -1)
            {
                *pfInvalid = true;
                break;
            }
        }
    }

    return vchRet;
}

std::vector<unsigned char> DecodeBase32(const std::string& str, bool* pfInvalid)
{
    return DecodeBase32(str.c_str(), pfInvalid);
}

static std::string DoTimingSafeFormat(const std::string& str) {
    return str;
}

std::string TimingSafeFormat(const std::string& str)
{
    return DoTimingSafeFormat(str);
}

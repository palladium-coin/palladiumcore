// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_SCRIPT_PALLADIUMCONSENSUS_H
#define PALLADIUM_SCRIPT_PALLADIUMCONSENSUS_H

#include <stdint.h>

#if defined(BUILD_PALLADIUM_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/palladium-config.h>
  #if defined(_WIN32)
    #if defined(DLL_EXPORT)
      #if defined(HAVE_FUNC_ATTRIBUTE_DLLEXPORT)
        #define EXPORT_SYMBOL __declspec(dllexport)
      #else
        #define EXPORT_SYMBOL
      #endif
    #endif
  #elif defined(HAVE_FUNC_ATTRIBUTE_VISIBILITY)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBPALLADIUMCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PALLADIUMCONSENSUS_API_VER 1

typedef enum palladiumconsensus_error_t
{
    palladiumconsensus_ERR_OK = 0,
    palladiumconsensus_ERR_TX_INDEX,
    palladiumconsensus_ERR_TX_SIZE_MISMATCH,
    palladiumconsensus_ERR_TX_DESERIALIZE,
    palladiumconsensus_ERR_AMOUNT_REQUIRED,
    palladiumconsensus_ERR_INVALID_FLAGS,
} palladiumconsensus_error;

/** Script verification flags */
enum
{
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), // enable CHECKLOCKTIMEVERIFY (BIP65)
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), // enable CHECKSEQUENCEVERIFY (BIP112)
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), // enable WITNESS (BIP141)
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT             = (1U << 17), // enable TAPROOT (BIP341/BIP342)
    palladiumconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = palladiumconsensus_SCRIPT_FLAGS_VERIFY_P2SH | palladiumconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               palladiumconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | palladiumconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                               palladiumconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY | palladiumconsensus_SCRIPT_FLAGS_VERIFY_WITNESS |
                                                               palladiumconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT
};

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not nullptr, err will contain an error/success code for the operation
EXPORT_SYMBOL int palladiumconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                                 const unsigned char *txTo        , unsigned int txToLen,
                                                 unsigned int nIn, unsigned int flags, palladiumconsensus_error* err);

EXPORT_SYMBOL int palladiumconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, palladiumconsensus_error* err);

EXPORT_SYMBOL unsigned int palladiumconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // PALLADIUM_SCRIPT_PALLADIUMCONSENSUS_H

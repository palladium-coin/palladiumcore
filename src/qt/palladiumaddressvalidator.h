// Copyright (c) 2009-2026 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_QT_PALLADIUMADDRESSVALIDATOR_H
#define PALLADIUM_QT_PALLADIUMADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class PalladiumAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit PalladiumAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Palladium address widget validator, checks for a valid palladium address.
 */
class PalladiumAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit PalladiumAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // PALLADIUM_QT_PALLADIUMADDRESSVALIDATOR_H

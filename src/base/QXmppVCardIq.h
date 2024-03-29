/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Author:
 *  Manjeet Dahiya
 *
 * Source:
 *  http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */


#ifndef QXMPPVCARDIQ_H
#define QXMPPVCARDIQ_H

#include "QXmppIq.h"
#include <QDate>
#include <QMap>
#include <QDomElement>

class QXmppVCardAddressPrivate;
class QXmppVCardEmailPrivate;
class QXmppVCardPhonePrivate;
class QXmppVCardIqPrivate;

/// \brief Represent a vCard address.

class QXMPP_EXPORT QXmppVCardAddress
{
public:
    /// \brief Describes e-mail address types.
    enum TypeFlag {
        None        = 0x0,
        Home        = 0x1,
        Work        = 0x2,
        Postal      = 0x4,
        Preferred   = 0x8
    };
    Q_DECLARE_FLAGS(Type, TypeFlag);

    QXmppVCardAddress();
    QXmppVCardAddress(const QXmppVCardAddress &other);
    ~QXmppVCardAddress();

    QXmppVCardAddress& operator=(const QXmppVCardAddress &other);

    QString country() const;
    void setCountry(const QString &country);

    QString locality() const;
    void setLocality(const QString &locality);

    QString postcode() const;
    void setPostcode(const QString &postcode);

    QString region() const;
    void setRegion(const QString &region);

    QString street() const;
    void setStreet(const QString &street);

    Type type() const;
    void setType(Type type);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *stream) const;
    /// \endcond

private:
    QSharedDataPointer<QXmppVCardAddressPrivate> d;
};

/// \brief Represents a vCard e-mail address.

class QXMPP_EXPORT QXmppVCardEmail
{
public:
    /// \brief Describes e-mail address types.
    enum TypeFlag {
        None        = 0x0,
        Home        = 0x1,
        Work        = 0x2,
        Internet    = 0x4,
        Preferred   = 0x8,
        X400        = 0x10
    };
    Q_DECLARE_FLAGS(Type, TypeFlag);

    QXmppVCardEmail();
    QXmppVCardEmail(const QXmppVCardEmail &other);
    ~QXmppVCardEmail();

    QXmppVCardEmail& operator=(const QXmppVCardEmail &other);

    QString address() const;
    void setAddress(const QString &address);

    Type type() const;
    void setType(Type type);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *stream) const;
    /// \endcond

private:
    QSharedDataPointer<QXmppVCardEmailPrivate> d;
};

/// \brief Represents a vCard phone number.

class QXmppVCardPhone
{
public:
    /// \brief Describes phone number types.
    enum TypeFlag {
        None        = 0x0,
        Home        = 0x1,
        Work        = 0x2,
        Voice       = 0x4,
        Fax         = 0x8,
        Pager       = 0x10,
        Messaging   = 0x20,
        Cell        = 0x40,
        Video       = 0x80,
        BBS         = 0x100,
        Modem       = 0x200,
        ISDN        = 0x400,
        PCS         = 0x800,
        Preferred   = 0x1000
    };
    Q_DECLARE_FLAGS(Type, TypeFlag);

    QXmppVCardPhone();
    QXmppVCardPhone(const QXmppVCardPhone &other);
    ~QXmppVCardPhone();

    QXmppVCardPhone& operator=(const QXmppVCardPhone &other);

    QString number() const;
    void setNumber(const QString &number);

    Type type() const;
    void setType(Type type);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *stream) const;
    /// \endcond

private:
    QSharedDataPointer<QXmppVCardPhonePrivate> d;
};

/// \brief Represents the XMPP vCard.
///
/// The functions names are self explanatory.
/// Look at QXmppVCardManager and XEP-0054: vcard-temp for more details.
///
/// There are many field of XMPP vCard which are not present in
/// this class. File a issue for the same. We will add the requested
/// field to this class.
///

class QXMPP_EXPORT QXmppVCardIq : public QXmppIq
{
public:
    QXmppVCardIq(const QString& bareJid = "");
    QXmppVCardIq(const QXmppVCardIq &other);
    ~QXmppVCardIq();

    QXmppVCardIq& operator=(const QXmppVCardIq &other);

    QDate birthday() const;
    void setBirthday(const QDate &birthday);

    QString email() const;
    void setEmail(const QString&);

    QString firstName() const;
    void setFirstName(const QString&);

    QString fullName() const;
    void setFullName(const QString&);

    QString lastName() const;
    void setLastName(const QString&);

    QString middleName() const;
    void setMiddleName(const QString&);

    QString nickName() const;
    void setNickName(const QString&);

    QByteArray photo() const;
    void setPhoto(const QByteArray&);

    QString photoType() const;
    void setPhotoType(const QString &type);

    QString url() const;
    void setUrl(const QString&);

    QList<QXmppVCardAddress> addresses() const;
    void setAddresses(const QList<QXmppVCardAddress> &addresses);

    QList<QXmppVCardEmail> emails() const;
    void setEmails(const QList<QXmppVCardEmail> &emails);

    QList<QXmppVCardPhone> phones() const;
    void setPhones(const QList<QXmppVCardPhone> &phones);

    /// \cond
    static bool isVCard(const QDomElement &element);
    /// \endcond

protected:
    /// \cond
    void parseElementFromChild(const QDomElement&);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;
    /// \endcond

private:
    QSharedDataPointer<QXmppVCardIqPrivate> d;
};

#endif // QXMPPVCARDIQ_H

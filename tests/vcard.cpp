/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
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

#include "QXmppVCardIq.h"

#include "tests.h"
#include "vcard.h"

void tst_QXmppVCardIq::testAddress_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("country");
    QTest::addColumn<QString>("locality");
    QTest::addColumn<QString>("postcode");
    QTest::addColumn<QString>("region");
    QTest::addColumn<QString>("street");

    QTest::newRow("none") << QByteArray("<ADR/>") << int(QXmppVCardAddress::None) << "" << "" << "" << "" << "";
    QTest::newRow("HOME") << QByteArray("<ADR><HOME/></ADR>") << int(QXmppVCardAddress::Home) << "" << "" << "" << "" << "";
    QTest::newRow("WORK") << QByteArray("<ADR><WORK/></ADR>") << int(QXmppVCardAddress::Work) << "" << "" << "" << "" << "";
    QTest::newRow("POSTAL") << QByteArray("<ADR><POSTAL/></ADR>") << int(QXmppVCardAddress::Postal) << "" << "" << "" << "" << "";
    QTest::newRow("PREF") << QByteArray("<ADR><PREF/></ADR>") << int(QXmppVCardAddress::Preferred) << "" << "" << "" << "" << "";

    QTest::newRow("country") << QByteArray("<ADR><CTRY>France</CTRY></ADR>") << int(QXmppVCardAddress::None) << "France" << "" << "" << "" << "";
    QTest::newRow("locality") << QByteArray("<ADR><LOCALITY>Paris</LOCALITY></ADR>") << int(QXmppVCardAddress::None) << "" << "Paris" << "" << "" << "";
    QTest::newRow("postcode") << QByteArray("<ADR><PCODE>75008</PCODE></ADR>") << int(QXmppVCardAddress::None) << "" << "" << "75008" << "" << "";
    QTest::newRow("region") << QByteArray("<ADR><REGION>Ile de France</REGION></ADR>") << int(QXmppVCardAddress::None) << "" << "" << "" << "Ile de France" << "";
    QTest::newRow("street") << QByteArray("<ADR><STREET>55 rue du faubourg Saint-Honoré</STREET></ADR>") << int(QXmppVCardAddress::None) << "" << "" << "" << "" << QString::fromUtf8("55 rue du faubourg Saint-Honoré");
}

void tst_QXmppVCardIq::testAddress()
{
    QFETCH(QByteArray, xml);
    QFETCH(int, type);
    QFETCH(QString, country);
    QFETCH(QString, locality);
    QFETCH(QString, postcode);
    QFETCH(QString, region);
    QFETCH(QString, street);

    QXmppVCardAddress address;
    parsePacket(address, xml);
    QCOMPARE(int(address.type()), type);
    QCOMPARE(address.country(), country);
    QCOMPARE(address.locality(), locality);
    QCOMPARE(address.postcode(), postcode);
    QCOMPARE(address.region(), region);
    QCOMPARE(address.street(), street);
    serializePacket(address, xml);
}

void tst_QXmppVCardIq::testEmail_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("type");

    QTest::newRow("none") << QByteArray("<EMAIL><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::None);
    QTest::newRow("HOME") << QByteArray("<EMAIL><HOME/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::Home);
    QTest::newRow("WORK") << QByteArray("<EMAIL><WORK/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::Work);
    QTest::newRow("INTERNET") << QByteArray("<EMAIL><INTERNET/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::Internet);
    QTest::newRow("X400") << QByteArray("<EMAIL><X400/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::X400);
    QTest::newRow("PREF") << QByteArray("<EMAIL><PREF/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::Preferred);
    QTest::newRow("all") << QByteArray("<EMAIL><HOME/><WORK/><INTERNET/><PREF/><X400/><USERID>foo.bar@example.com</USERID></EMAIL>") << int(QXmppVCardEmail::Home | QXmppVCardEmail::Work | QXmppVCardEmail::Internet | QXmppVCardEmail::Preferred | QXmppVCardEmail::X400);
}

void tst_QXmppVCardIq::testEmail()
{
    QFETCH(QByteArray, xml);
    QFETCH(int, type);

    QXmppVCardEmail email;
    parsePacket(email, xml);
    QCOMPARE(email.address(), QLatin1String("foo.bar@example.com"));
    QCOMPARE(int(email.type()), type);
    serializePacket(email, xml);
}

void tst_QXmppVCardIq::testPhone_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("type");

    QTest::newRow("none") << QByteArray("<PHONE><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::None);
    QTest::newRow("HOME") << QByteArray("<PHONE><HOME/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Home);
    QTest::newRow("WORK") << QByteArray("<PHONE><WORK/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Work);
    QTest::newRow("VOICE") << QByteArray("<PHONE><VOICE/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Voice);
    QTest::newRow("FAX") << QByteArray("<PHONE><FAX/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Fax);
    QTest::newRow("PAGER") << QByteArray("<PHONE><PAGER/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Pager);
    QTest::newRow("MSG") << QByteArray("<PHONE><MSG/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Messaging);
    QTest::newRow("CELL") << QByteArray("<PHONE><CELL/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Cell);
    QTest::newRow("VIDEO") << QByteArray("<PHONE><VIDEO/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Video);
    QTest::newRow("BBS") << QByteArray("<PHONE><BBS/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::BBS);
    QTest::newRow("MODEM") << QByteArray("<PHONE><MODEM/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Modem);
    QTest::newRow("IDSN") << QByteArray("<PHONE><ISDN/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::ISDN);
    QTest::newRow("PCS") << QByteArray("<PHONE><PCS/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::PCS);
    QTest::newRow("PREF") << QByteArray("<PHONE><PREF/><NUMBER>12345</NUMBER></PHONE>") << int(QXmppVCardPhone::Preferred);
}

void tst_QXmppVCardIq::testPhone()
{
    QFETCH(QByteArray, xml);
    QFETCH(int, type);

    QXmppVCardPhone phone;
    parsePacket(phone, xml);
    QCOMPARE(phone.number(), QLatin1String("12345"));
    QCOMPARE(int(phone.type()), type);
    serializePacket(phone, xml);
}


void tst_QXmppVCardIq::testVCard()
{
    const QByteArray xml(
        "<iq id=\"vcard1\" type=\"set\">"
        "<vCard xmlns=\"vcard-temp\">"
        "<ADR><CTRY>France</CTRY></ADR>"
        "<BDAY>1983-09-14</BDAY>"
        "<EMAIL><INTERNET/><USERID>foo.bar@example.com</USERID></EMAIL>"
        "<FN>Foo Bar!</FN>"
        "<NICKNAME>FooBar</NICKNAME>"
        "<N><GIVEN>Foo</GIVEN><FAMILY>Wiz</FAMILY><MIDDLE>Baz</MIDDLE></N>"
        "<PHONE><HOME/><NUMBER>12345</NUMBER></PHONE>"
        "<PHONE><WORK/><NUMBER>67890</NUMBER></PHONE>"
        "<PHOTO>"
            "<TYPE>image/png</TYPE>"
            "<BINVAL>"
            "iVBORw0KGgoAAAANSUhEUgAAAAgAAAAICAIAAABLbSncAAAAAXNSR0IArs4c6QAAAAlwSFlzAAA"
            "UIgAAFCIBjw1HyAAAAAd0SU1FB9oIHQInNvuJovgAAAAiSURBVAjXY2TQ+s/AwMDAwPD/GiMDlP"
            "WfgYGBiQEHGJwSAK2BBQ1f3uvpAAAAAElFTkSuQmCC"
            "</BINVAL>"
        "</PHOTO>"
        "<URL>http://code.google.com/p/qxmpp/</URL>"
        "</vCard>"
        "</iq>");

    QXmppVCardIq vcard;
    parsePacket(vcard, xml);
    QCOMPARE(vcard.addresses().size(), 1);
    QCOMPARE(vcard.addresses()[0].country(), QLatin1String("France"));
    QCOMPARE(int(vcard.addresses()[0].type()), int(QXmppVCardEmail::None));
    QCOMPARE(vcard.birthday(), QDate(1983, 9, 14));
    QCOMPARE(vcard.email(), QLatin1String("foo.bar@example.com"));
    QCOMPARE(vcard.emails().size(), 1);
    QCOMPARE(vcard.emails()[0].address(), QLatin1String("foo.bar@example.com"));
    QCOMPARE(int(vcard.emails()[0].type()), int(QXmppVCardEmail::Internet));
    QCOMPARE(vcard.nickName(), QLatin1String("FooBar"));
    QCOMPARE(vcard.fullName(), QLatin1String("Foo Bar!"));
    QCOMPARE(vcard.firstName(), QLatin1String("Foo"));
    QCOMPARE(vcard.middleName(), QLatin1String("Baz"));
    QCOMPARE(vcard.lastName(), QLatin1String("Wiz"));
    QCOMPARE(vcard.phones().size(), 2);
    QCOMPARE(vcard.phones()[0].number(), QLatin1String("12345"));
    QCOMPARE(int(vcard.phones()[0].type()), int(QXmppVCardEmail::Home));
    QCOMPARE(vcard.phones()[1].number(), QLatin1String("67890"));
    QCOMPARE(int(vcard.phones()[1].type()), int(QXmppVCardEmail::Work));
    QCOMPARE(vcard.photo(), QByteArray::fromBase64(
        "iVBORw0KGgoAAAANSUhEUgAAAAgAAAAICAIAAABLbSncAAAAAXNSR0IArs4c6QAAAAlwSFlzAAA"
        "UIgAAFCIBjw1HyAAAAAd0SU1FB9oIHQInNvuJovgAAAAiSURBVAjXY2TQ+s/AwMDAwPD/GiMDlP"
        "WfgYGBiQEHGJwSAK2BBQ1f3uvpAAAAAElFTkSuQmCC"));
    QCOMPARE(vcard.photoType(), QLatin1String("image/png"));
    QCOMPARE(vcard.url(), QLatin1String("http://code.google.com/p/qxmpp/"));
    serializePacket(vcard, xml);
}

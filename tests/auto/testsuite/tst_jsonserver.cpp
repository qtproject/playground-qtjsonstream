/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtAddOn.JsonStream module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_jsonserver.h"

#include "jsonpidauthority.h"
#include "jsontokenauthority.h"

#include <QtTest/QtTest>

tst_JsonServer::~tst_JsonServer()
{
    qDebug() << Q_FUNC_INFO;

    QCOMPARE(mSpyServerConnectionAdded->count(), 1); // make sure the signal was emitted exactly one time
    QCOMPARE(mSpyServerConnectionRemoved->count(), 1); // make sure the signal was emitted exactly one time
   // QCOMPARE(mSpyMessageReceived->count(), 1); // make sure the signal was emitted exactly one time


    delete mServer;

    delete mSpyServerConnectionAdded;
    delete mSpyServerConnectionRemoved;
    delete mSpyMessageReceived;
}

void tst_JsonServer::connectionAdded(const QString& id)
{
    qDebug() << Q_FUNC_INFO << id;
    QVERIFY(mServer->connections().count() == 1);
}

void tst_JsonServer::connectionRemoved(const QString& id)
{
    qDebug() << Q_FUNC_INFO << id << (id == mId);
    QVERIFY(mServer->connections().count() == 0);
    if (id.isEmpty())
        QFAIL("tst_JsonServer::connectionRemoved: empty id");
    if (id != mId)
        QFAIL("tst_JsonServer::connectionRemoved: unknown id");

    mId = "";
}

void tst_JsonServer::messageReceived(const QString& id, const QJsonObject& msg)
{
    qDebug() << Q_FUNC_INFO << id << msg;
    mId = id;
    if (msg.contains("note"))
    {
        mNote = msg.value("note").toString();

        qDebug() << "Got note=" << note();

        QJsonObject msg_reply;
        msg_reply.insert("note", mNote);
        qDebug() << "Sending" << msg_reply << "to" << mId;
        QVERIFY(mServer->hasConnection(mId));
        QVERIFY(mServer->send(mId, msg_reply));
    }
}

void tst_JsonServer::noAuthTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;

    mServer = new QJsonServer;
    connect(mServer, SIGNAL(connectionAdded(const QString&)), this, SLOT(connectionAdded(const QString&)));
    mSpyServerConnectionAdded = new QSignalSpy(mServer, SIGNAL(connectionAdded(const QString&)));

    connect(mServer, SIGNAL(connectionRemoved(const QString&)), this, SLOT(connectionRemoved(const QString&)));
    mSpyServerConnectionRemoved = new QSignalSpy(mServer, SIGNAL(connectionRemoved(const QString&)));

    connect(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)),
        this, SLOT(messageReceived(const QString&, const QJsonObject&)));
    mSpyMessageReceived = new QSignalSpy(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)));

    QVERIFY(mServer->connections().count() == 0);

    QVERIFY(mServer->listen(mSocketname));
}

void tst_JsonServer::pidAuthorityTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;

#ifdef MMM // not implemented
    QJsonPIDAuthority *auth = new QJsonPIDAuthority(this);

    pid_t clientPid = getpid();
    QString clientId = "pid_auth_tester";

    // should fail with empty pid or identifier
    QVERIFY(auth->authorize(0, "pid_auth_tester") == false);
   // QVERIFY(auth->authorize(pid(), QString::null) == false);

    QVERIFY(auth->authorize(clientPid, clientId));
    // should fail second time with same id
    QVERIFY(auth->authorize(clientPid, clientId) == false);

    QVERIFY(auth->deauthorize(clientPid));
    QVERIFY(auth->deauthorize(clientPid) == false);
#endif

    mServer = new QJsonServer;
    connect(mServer, SIGNAL(connectionAdded(const QString&)), this, SLOT(connectionAdded(const QString&)));
    mSpyServerConnectionAdded = new QSignalSpy(mServer, SIGNAL(connectionAdded(const QString&)));

    connect(mServer, SIGNAL(connectionRemoved(const QString&)), this, SLOT(connectionRemoved(const QString&)));
    mSpyServerConnectionRemoved = new QSignalSpy(mServer, SIGNAL(connectionRemoved(const QString&)));

    connect(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)),
        this, SLOT(messageReceived(const QString&, const QJsonObject&)));
    mSpyMessageReceived = new QSignalSpy(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)));

    QVERIFY(mServer->connections().count() == 0);

    QVERIFY(mServer->listen(mSocketname));
}

void tst_JsonServer::tokenAuthorityTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;

    QJsonTokenAuthority *auth = new QJsonTokenAuthority(this);

    QString clientId = "token_auth_tester";
    QString clientToken = QUuid::createUuid().toString();

    // should fail with empty token or identifier
    QVERIFY(auth->authorize(QString::null, clientId) == false);
    QVERIFY(auth->authorize(clientToken, QString::null) == false);


    QVERIFY(auth->authorize(clientToken, clientId));
    // should fail second time with same id
    QVERIFY(auth->authorize(clientToken, clientId) == false);

    QVERIFY(auth->deauthorize(clientToken));
    QVERIFY(auth->deauthorize(clientToken) == false);

    QVERIFY(auth->authorize(clientToken, clientId));

    mServer = new QJsonServer;
    connect(mServer, SIGNAL(connectionAdded(const QString&)), this, SLOT(connectionAdded(const QString&)));
    mSpyServerConnectionAdded = new QSignalSpy(mServer, SIGNAL(connectionAdded(const QString&)));

    connect(mServer, SIGNAL(connectionRemoved(const QString&)), this, SLOT(connectionRemoved(const QString&)));
    mSpyServerConnectionRemoved = new QSignalSpy(mServer, SIGNAL(connectionRemoved(const QString&)));

    connect(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)),
        this, SLOT(messageReceived(const QString&, const QJsonObject&)));
    mSpyMessageReceived = new QSignalSpy(mServer, SIGNAL(messageReceived(const QString&, const QJsonObject&)));

    QVERIFY(mServer->connections().count() == 0);

    QVERIFY(mServer->listen(mSocketname, auth));
}

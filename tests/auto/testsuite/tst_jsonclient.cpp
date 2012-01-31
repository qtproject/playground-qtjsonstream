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

#include "tst_jsonclient.h"

#include <QtTest/QtTest>

tst_JsonClient::tst_JsonClient(const QString& socketname, const QString& strMsg)
    : mMsg(strMsg)
{
    qDebug() << Q_FUNC_INFO;

    mClient = new JsonClient;
    connect(mClient, SIGNAL(messageReceived(const QJsonObject&)),
        this, SLOT(messageReceived(const QJsonObject&)));
    mSpyMessageReceived = new QSignalSpy(mClient, SIGNAL(messageReceived(const QJsonObject&)));

    qWarning() << "Connecting to " << socketname;
    QVERIFY(mClient->connectLocal(socketname));

    QJsonObject msg;
    msg.insert("note", mMsg);

    qDebug() << "Sending message: " << mMsg;
    mClient->send(msg);
}

tst_JsonClient::~tst_JsonClient()
{
    qDebug() << Q_FUNC_INFO;

    delete mClient;

    delete mSpyMessageReceived;
}


void tst_JsonClient::messageReceived(const QJsonObject& message)
{
    qDebug() << Q_FUNC_INFO;

    QString str = message.value("note").toString();
    qDebug() << "Received" << message << str;

    QVERIFY(str == mMsg);

    deleteLater();
}

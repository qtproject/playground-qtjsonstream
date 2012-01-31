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

#ifndef TST_JSONSERVER_H
#define TST_JSONSERVER_H

#include "jsonserver.h"

class QSignalSpy;

class tst_JsonServer : public QObject
{
    Q_OBJECT

public:
    tst_JsonServer(const QString & socketname)
        : mSocketname(socketname)
    {
    }

    ~tst_JsonServer();

public:
    void noAuthTest();
    void pidAuthorityTest();
    void tokenAuthorityTest();

    QString note() { return mNote; }
    QString id() { return mId; }

protected slots:
    void connectionAdded(const QString& id);
    void connectionRemoved(const QString& id);
    void messageReceived(const QString& id, const QJsonObject& msg);

private:
  //  QProcess *process;
    QString   mSocketname;
    QString   mNote;
    QString   mId;
    bool      mDone;

    JsonServer *mServer;
    QSignalSpy *mSpyServerConnectionAdded;
    QSignalSpy *mSpyServerConnectionRemoved;
    QSignalSpy *mSpyMessageReceived;
};

#endif // TST_JSONSERVER_H

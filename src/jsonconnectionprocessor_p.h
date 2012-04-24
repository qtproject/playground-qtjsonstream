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

#ifndef _JSON_CONNECTION_PROCESSOR_H
#define _JSON_CONNECTION_PROCESSOR_H

#include "jsonstream-global.h"
#include "jsonconnection.h"

#include <QLocalSocket>
#include <QTcpSocket>
class QJsonObject;

QT_BEGIN_NAMESPACE_JSONSTREAM

#include <QLocalSocket>
#include <QTcpSocket>

class JsonEndpoint;
class JsonEndpointManager;

class JsonConnectionProcessorPrivate;
class JsonConnectionProcessor : public QObject
{
    Q_OBJECT
public:
    JsonConnectionProcessor();
    ~JsonConnectionProcessor();

    void setEndpointManager(JsonEndpointManager *);

    void setAutoReconnectEnabled(bool enabled);
    JsonConnection::State state() const;

signals:
    void stateChanged(JsonConnection::State);
    void readyReadMessage();
    void disconnected();
    void bytesWritten(qint64);
    void readBufferOverflow(qint64);
    void error(JsonConnection::Error,int,QString);

public slots:
    bool connectTCP(const QString&,int);
    bool connectLocal(const QString&);
    void setFormat(int);
    void setReadBufferSize(qint64);
    void setWriteBufferSize(qint64);
    bool send(QJsonObject message);
    bool messageAvailable(JsonEndpoint *);
    QJsonObject readMessage(JsonEndpoint *);

protected slots:
    void processMessage(JsonEndpoint* = 0);
    void handleSocketDisconnected();
    void handleReconnect();
    void handleSocketError(QAbstractSocket::SocketError);
    void handleSocketError(QLocalSocket::LocalSocketError);

protected:

private:
    Q_DECLARE_PRIVATE(JsonConnectionProcessor)
    QScopedPointer<JsonConnectionProcessorPrivate> d_ptr;

    // forbid copy constructor
    JsonConnectionProcessor(const JsonConnectionProcessor &);
    void operator=(const JsonConnectionProcessor &);
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_CONNECTION_PROCESSOR_H

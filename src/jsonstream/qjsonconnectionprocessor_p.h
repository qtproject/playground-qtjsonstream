/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef _JSON_CONNECTION_PROCESSOR_H
#define _JSON_CONNECTION_PROCESSOR_H

#include "qjsonstream-global.h"
#include "qjsonconnection.h"

#include <QLocalSocket>
#include <QTcpSocket>
class QJsonObject;

QT_BEGIN_NAMESPACE_JSONSTREAM

#include <QLocalSocket>
#include <QTcpSocket>

class QJsonEndpoint;
class QJsonEndpointManager;

class QJsonConnectionProcessorPrivate;
class QJsonConnectionProcessor : public QObject
{
    Q_OBJECT
public:
    QJsonConnectionProcessor();
    ~QJsonConnectionProcessor();

    void setEndpointManager(QJsonEndpointManager *);

    void setAutoReconnectEnabled(bool enabled);
    QJsonConnection::State state() const;

signals:
    void stateChanged(QJsonConnection::State);
    void readyReadMessage();
    void disconnected();
    void bytesWritten(qint64);
    void readBufferOverflow(qint64);
    void error(QJsonConnection::Error,int,QString);

public slots:
    bool connectTCP(const QString&,int);
    bool connectLocal(const QString&);
    void setFormat(int);
    void setReadBufferSize(qint64);
    void setWriteBufferSize(qint64);
    bool send(QJsonObject message);
    bool messageAvailable(QJsonEndpoint *);
    QJsonObject readMessage(QJsonEndpoint *);

protected slots:
    void processMessage(QJsonEndpoint* = 0);
    void handleSocketDisconnected();
    void handleReconnect();
    void handleSocketError(QAbstractSocket::SocketError);
    void handleSocketError(QLocalSocket::LocalSocketError);

protected:

private:
    Q_DECLARE_PRIVATE(QJsonConnectionProcessor)
    QScopedPointer<QJsonConnectionProcessorPrivate> d_ptr;

    // forbid copy constructor
    QJsonConnectionProcessor(const QJsonConnectionProcessor &);
    void operator=(const QJsonConnectionProcessor &);
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_CONNECTION_PROCESSOR_H

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

#ifndef _JSON_CONNECTION_H
#define _JSON_CONNECTION_H

#include "qjsonstream-global.h"
#include <QObject>

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonEndpoint;
class QJsonConnectionProcessor;

class QJsonConnectionPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(QString localSocketName READ localSocketName WRITE setLocalSocketName)
    Q_PROPERTY(QString tcpHostName READ tcpHostName WRITE setTcpHostName)
    Q_PROPERTY(int tcpHostPort READ tcpHostPort WRITE setTcpHostPort)
    Q_PROPERTY(QString endpointPropertyName READ endpointPropertyName WRITE setEndpointPropertyName)
    Q_PROPERTY(bool autoReconnectEnabled READ autoReconnectEnabled WRITE setAutoReconnectEnabled)
    Q_PROPERTY(bool useSeparateThreadForProcessing READ useSeparateThreadForProcessing WRITE setUseSeparateThreadForProcessing)
    Q_PROPERTY(qint64 readBufferSize READ readBufferSize WRITE setReadBufferSize)
    Q_PROPERTY(qint64 writeBufferSize READ writeBufferSize WRITE setWriteBufferSize)
    Q_PROPERTY(QJsonEndpoint * defaultEndpoint READ defaultEndpoint)
public:
    QJsonConnection(QObject *parent = 0);
    ~QJsonConnection();

    enum State {
        Unconnected = 0,
        Connecting,
        Authenticating,
        Connected
    };
    Q_ENUMS(State)

    State state() const;

    enum Error {
        NoError = 0,
        UnknownError,
        LocalSocketError,
        TcpSocketError
    };
    Q_ENUMS(Error)

    Error error() const;
    int subError() const;
    QString errorString() const;

    QString localSocketName() const;
    void setLocalSocketName(const QString &);

    QString tcpHostName() const;
    void setTcpHostName(const QString &);

    int tcpHostPort() const;
    void setTcpHostPort(int);

    QString endpointPropertyName() const;
    void setEndpointPropertyName(const QString &);

    bool autoReconnectEnabled() const;
    void setAutoReconnectEnabled(bool);

    bool useSeparateThreadForProcessing() const;
    void setUseSeparateThreadForProcessing(bool);

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64);

    qint64 writeBufferSize() const;
    void setWriteBufferSize(qint64 sz);

    Q_INVOKABLE bool connectTCP(const QString& hostname = QString::null, int port = 0);
    Q_INVOKABLE bool connectLocal(const QString& socketname = QString::null);

    void addEndpoint(QJsonEndpoint *);
    QJsonEndpoint * defaultEndpoint();
    void removeEndpoint(QJsonEndpoint *);

    void setFormat( EncodingFormat format );

    QJsonConnectionProcessor *processor() const;

signals:
    void bytesWritten(qint64);
    void readBufferOverflow(qint64);
    void stateChanged(QJsonConnection::State);
    void disconnected();
    void error(QJsonConnection::Error, int);

private slots:
    void handleError(QJsonConnection::Error, int, QString);

private:
    Q_DECLARE_PRIVATE(QJsonConnection)
    QScopedPointer<QJsonConnectionPrivate> d_ptr;

    // forbid copy constructor
    QJsonConnection(const QJsonConnection &);
    void operator=(const QJsonConnection &);
};

QT_END_NAMESPACE_JSONSTREAM

Q_DECLARE_METATYPE(::QtAddOn::QtJsonStream::QJsonConnection::State)
Q_DECLARE_METATYPE(::QtAddOn::QtJsonStream::QJsonConnection::Error)

#endif // _JSON_CONNECTION_H

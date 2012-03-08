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

#include <QtCore>
#include <QtNetwork>

#include "jsonserverclient.h"
#include "jsonauthority.h"
#include "jsonstream.h"

QT_BEGIN_NAMESPACE_JSONSTREAM


/*!
  \internal
*/
class JsonServerClientPrivate
{
public:
    JsonServerClientPrivate()
        : m_socket(0)
        , m_stream(0) {}

    QString        m_identifier;
    QLocalSocket  *m_socket;
    JsonStream    *m_stream;
    QWeakPointer<JsonAuthority> m_authority;
};

/****************************************************************************/

JsonServerClient::JsonServerClient(QObject *parent)
    : QObject(parent)
    , d_ptr(new JsonServerClientPrivate())
{
}

JsonServerClient::~JsonServerClient()
{
}

/*!
    Sets the authority manager of the client to \a authority.

    Does not take ownership of the object.
*/
void JsonServerClient::setAuthority(JsonAuthority *authority)
{
    Q_D(JsonServerClient);
    d->m_authority = authority;
}

const QLocalSocket *JsonServerClient::socket() const
{
    Q_D(const JsonServerClient);
    return d->m_socket;
}

void JsonServerClient::setSocket(QLocalSocket *socket)
{
    Q_D(JsonServerClient);
    d->m_socket = socket;
    d->m_socket->setParent(this);

    if (socket) {
        d->m_stream = new JsonStream(socket);
        d->m_stream->setParent(this);
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
        connect(d->m_stream, SIGNAL(messageReceived(const QJsonObject&)),
                this, SLOT(received(const QJsonObject&)));
    }
}

QString JsonServerClient::identifier() const
{
    Q_D(const JsonServerClient);
    return d->m_identifier;
}

void JsonServerClient::start()
{
    Q_D(JsonServerClient);
    if (!d->m_authority) {
        d->m_identifier = QUuid::createUuid().toString();
        emit authorized(d->m_identifier);
    } else {
        AuthorizationRecord authRecord = d->m_authority.data()->clientConnected(d->m_stream);
        switch (authRecord.state) {
        case JsonAuthority::StateAuthorized:
            d->m_identifier = authRecord.identifier;
            emit authorized(d->m_identifier);
            break;
        case JsonAuthority::StateNotAuthorized:
            emit authorizationFailed();
            stop();
            break;
        default:
            break;
        }
    }
}

/*!
    Force the connection to stop
    Once the socket disconnects, a "disconnected" signal will be emitted
*/
void JsonServerClient::stop()
{
    // qDebug() << Q_FUNC_INFO;
    Q_D(JsonServerClient);
    disconnect(d->m_stream, SIGNAL(messageReceived(const QJsonObject&)),
               this, SLOT(received(const QJsonObject&)));
    d->m_socket->disconnectFromServer();
    // qDebug() << Q_FUNC_INFO << "done";
}

void JsonServerClient::received(const QJsonObject& message)
{
    Q_D(JsonServerClient);
    if (d->m_identifier.isEmpty()) {
        if (d->m_authority.isNull()) {
            emit authorizationFailed();
            stop();
            return;
        } else {
            AuthorizationRecord authRecord = d->m_authority.data()->messageReceived(d->m_stream, message);
            switch (authRecord.state) {
            case JsonAuthority::StateAuthorized:
                d->m_identifier = authRecord.identifier;
                emit authorized(d->m_identifier);
                break;
            case JsonAuthority::StateNotAuthorized:
                emit authorizationFailed();
                stop();
                break;
            default:
                break;
            }
        }
    }
    else {
        emit messageReceived(d->m_identifier, message);
    }
}

void JsonServerClient::send(const QJsonObject &message)
{
    Q_D(JsonServerClient);
    if (d->m_stream) {
        // qDebug() << "Sending message" << message;
        d->m_stream->send(message);
    }
}

void JsonServerClient::handleDisconnect()
{
    // qDebug() << Q_FUNC_INFO;
    Q_D(JsonServerClient);
    d->m_stream->setDevice(0);
    if (d->m_authority.data())
        d->m_authority.data()->clientDisconnected(d->m_stream);
    if (!d->m_identifier.isEmpty())
        emit disconnected(d->m_identifier);
    // qDebug() << Q_FUNC_INFO << "done";
}

#include "moc_jsonserverclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

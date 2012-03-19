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

/*!
  \class JsonServerClient
  \brief The JsonServerClient class wraps an individual connection to the JsonServer.
 */

/*!
  Construct a JsonServerClient object with the given \a parent
 */

JsonServerClient::JsonServerClient(QObject *parent)
    : QObject(parent)
    , d_ptr(new JsonServerClientPrivate())
{
}

/*!
  Destructs the JsonServerClient object.
 */

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

/*!
  Return the internal socket object
*/

const QLocalSocket *JsonServerClient::socket() const
{
    Q_D(const JsonServerClient);
    return d->m_socket;
}

/*!
  Set the internal socket object to \a socket.
 */

void JsonServerClient::setSocket(QLocalSocket *socket)
{
    Q_D(JsonServerClient);
    d->m_socket = socket;
    d->m_socket->setParent(this);

    if (socket) {
        d->m_stream = new JsonStream(socket);
        d->m_stream->setParent(this);
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
        connect(d->m_stream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    }
}

/*!
  Return the client identifier
 */

QString JsonServerClient::identifier() const
{
    Q_D(const JsonServerClient);
    return d->m_identifier;
}

/*!
  Start processing messages from this client.  This function should
  only be called by the \l{JsonServer}
 */

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
    disconnect(d->m_stream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
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

/*!
  Send a \a message to the client.
  Returns true if the entire message was send/buffered or false otherwise.
 */

bool JsonServerClient::send(const QJsonObject &message)
{
    bool ret = false;
    Q_D(JsonServerClient);
    if (d->m_stream) {
        // qDebug() << "Sending message" << message;
        ret = d->m_stream->send(message);
    }
    return ret;
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

/*!
  \internal
 */
void JsonServerClient::processMessages()
{
    Q_D(JsonServerClient);
    while (d->m_stream->messageAvailable()) {
        QJsonObject obj = d->m_stream->readMessage();
        if (!obj.isEmpty())
            received(obj);
    }
}

/*!
  \fn JsonServerClient::disconnected(const QString& identifier)
  This signal is emitted when the client has been disconnected.  The
  \a identifier property is included for convenience.
 */

/*!
  \fn JsonServerClient::messageReceived(const QString& identifier, const QJsonObject& message)

  This signal is emitted when a \a message has been received by the client. The
  \a identifier property is included for convenience.
 */

/*!
  \fn JsonServerClient::authorized(const QString& identifier)

  This signal is emitted when the client has been authorized. The
  \a identifier property is included for convenience.
 */

/*!
  \fn JsonServerClient::authorizationFailed()

  This signal is emitted when the client has failed authorization.
 */

#include "moc_jsonserverclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

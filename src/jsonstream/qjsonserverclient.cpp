/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtNetwork>

#include "qjsonserverclient.h"
#include "qjsonauthority.h"
#include "qjsonstream.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonServerClientPrivate
{
public:
    QJsonServerClientPrivate()
        : m_socket(0)
        , m_stream(0) {}

    QString        m_identifier;
    QLocalSocket  *m_socket;
    QJsonStream    *m_stream;
    QPointer<QJsonAuthority> m_authority;
};

/****************************************************************************/

/*!
  \class QJsonServerClient
  \inmodule QtJsonStream
  \brief The QJsonServerClient class wraps an individual connection to the QJsonServer.
 */

/*!
  Construct a QJsonServerClient object with the given \a parent
 */

QJsonServerClient::QJsonServerClient(QObject *parent)
    : QObject(parent)
    , d_ptr(new QJsonServerClientPrivate())
{
}

/*!
  Destructs the QJsonServerClient object.
 */

QJsonServerClient::~QJsonServerClient()
{
}

/*!
    Sets the authority manager of the client to \a authority.

    Does not take ownership of the object.
*/
void QJsonServerClient::setAuthority(QJsonAuthority *authority)
{
    Q_D(QJsonServerClient);
    d->m_authority = authority;
}

/*!
  Return the internal socket object
*/

const QLocalSocket *QJsonServerClient::socket() const
{
    Q_D(const QJsonServerClient);
    return d->m_socket;
}

/*!
  Set the internal socket object to \a socket.
 */

void QJsonServerClient::setSocket(QLocalSocket *socket)
{
    Q_D(QJsonServerClient);
    d->m_socket = socket;
    d->m_socket->setParent(this);

    if (socket) {
        d->m_stream = new QJsonStream(socket);
        d->m_stream->setParent(this);
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
        connect(d->m_stream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    }
}

/*!
  Return the client identifier
 */

QString QJsonServerClient::identifier() const
{
    Q_D(const QJsonServerClient);
    return d->m_identifier;
}

/*!
  Start processing messages from this client.  This function should
  only be called by the \l{QJsonServer}
 */

void QJsonServerClient::start()
{
    Q_D(QJsonServerClient);
    if (!d->m_authority) {
        d->m_identifier = QUuid::createUuid().toString();
        emit authorized(d->m_identifier);
    } else {
        AuthorizationRecord authRecord = d->m_authority.data()->clientConnected(d->m_stream);
        switch (authRecord.state) {
        case QJsonAuthority::StateAuthorized:
            d->m_identifier = authRecord.identifier;
            emit authorized(d->m_identifier);
            break;
        case QJsonAuthority::StateNotAuthorized:
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
void QJsonServerClient::stop()
{
    // qDebug() << Q_FUNC_INFO;
    Q_D(QJsonServerClient);
    disconnect(d->m_stream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    d->m_socket->disconnectFromServer();
    // qDebug() << Q_FUNC_INFO << "done";
}

void QJsonServerClient::received(const QJsonObject& message)
{
    Q_D(QJsonServerClient);
    if (d->m_identifier.isEmpty()) {
        if (d->m_authority.isNull()) {
            emit authorizationFailed();
            stop();
            return;
        } else {
            AuthorizationRecord authRecord = d->m_authority.data()->messageReceived(d->m_stream, message);
            switch (authRecord.state) {
            case QJsonAuthority::StateAuthorized:
                d->m_identifier = authRecord.identifier;
                emit authorized(d->m_identifier);
                break;
            case QJsonAuthority::StateNotAuthorized:
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

bool QJsonServerClient::send(const QJsonObject &message)
{
    bool ret = false;
    Q_D(QJsonServerClient);
    if (d->m_stream) {
        // qDebug() << "Sending message" << message;
        ret = d->m_stream->send(message);
    }
    return ret;
}

void QJsonServerClient::handleDisconnect()
{
    // qDebug() << Q_FUNC_INFO;
    Q_D(QJsonServerClient);
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
void QJsonServerClient::processMessages()
{
    Q_D(QJsonServerClient);
    while (d->m_stream->messageAvailable()) {
        QJsonObject obj = d->m_stream->readMessage();
        if (!obj.isEmpty())
            received(obj);
    }
}

/*!
  \fn QJsonServerClient::disconnected(const QString& identifier)
  This signal is emitted when the client has been disconnected.  The
  \a identifier property is included for convenience.
 */

/*!
  \fn QJsonServerClient::messageReceived(const QString& identifier, const QJsonObject& message)

  This signal is emitted when a \a message has been received by the client. The
  \a identifier property is included for convenience.
 */

/*!
  \fn QJsonServerClient::authorized(const QString& identifier)

  This signal is emitted when the client has been authorized. The
  \a identifier property is included for convenience.
 */

/*!
  \fn QJsonServerClient::authorizationFailed()

  This signal is emitted when the client has failed authorization.
 */

#include "moc_qjsonserverclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

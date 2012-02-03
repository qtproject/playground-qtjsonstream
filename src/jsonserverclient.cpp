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

JsonServerClient::JsonServerClient(QObject *parent)
    : QObject(parent)
    , m_socket(0)
    , m_stream(0)
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
    m_authority = authority;
}

const QLocalSocket *JsonServerClient::socket() const
{
    return m_socket;
}

void JsonServerClient::setSocket(QLocalSocket *socket)
{
    m_socket = socket;
    m_socket->setParent(this);

    if (socket) {
        m_stream = new JsonStream(socket);
        m_stream->setParent(this);
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
        connect(m_stream, SIGNAL(messageReceived(const QJsonObject&)),
                this, SLOT(received(const QJsonObject&)));
    }
}

QString JsonServerClient::identifier() const
{
    return m_identifier;
}

void JsonServerClient::start()
{
    if (!m_authority) {
        m_identifier = QUuid::createUuid().toString();
        emit authorized(m_identifier);
    } else {
        AuthorizationRecord authRecord = m_authority.data()->clientConnected(m_stream);
        switch (authRecord.state) {
        case JsonAuthority::StateAuthorized:
            m_identifier = authRecord.identifier;
            emit authorized(m_identifier);
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
    disconnect(m_stream, SIGNAL(messageReceived(const QJsonObject&)),
               this, SLOT(received(const QJsonObject&)));
    m_socket->disconnectFromServer();
    // qDebug() << Q_FUNC_INFO << "done";
}

void JsonServerClient::received(const QJsonObject& message)
{
    if (m_identifier.isEmpty()) {
        if (m_authority.isNull()) {
            emit authorizationFailed();
            stop();
            return;
        } else {
            AuthorizationRecord authRecord = m_authority.data()->messageReceived(m_stream, message);
            switch (authRecord.state) {
            case JsonAuthority::StateAuthorized:
                m_identifier = authRecord.identifier;
                emit authorized(m_identifier);
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
        emit messageReceived(m_identifier, message);
    }
}

void JsonServerClient::send(const QJsonObject &message)
{
    if (m_stream) {
        // qDebug() << "Sending message" << message;
        m_stream->send(message);
    }
}

void JsonServerClient::handleDisconnect()
{
    // qDebug() << Q_FUNC_INFO;
    m_stream->setDevice(0);
    if (m_authority.data())
        m_authority.data()->clientDisconnected(m_stream);
    if (!m_identifier.isEmpty())
        emit disconnected(m_identifier);
    // qDebug() << Q_FUNC_INFO << "done";
}

#include "moc_jsonserverclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

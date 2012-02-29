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

#include "jsonclient.h"

#include <QLocalSocket>
#include <QTcpSocket>
#include <QCoreApplication>
#include <QFile>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
    \class JsonClient
    \brief The JsonClient class is used to send jsons to the JsonServer.

    Note: The JsonClient is not thread safe.
*/

/*!
  Construct a new JsonClient instance with \a registration token and optional \a parent.
  The token will be passed in \c{{"token": registration}}.  This constructor
  is designed to work with JsonTokenAuthority.
 */
JsonClient::JsonClient(const QString& registration, QObject* parent)
    : QObject(parent),
      mStream(0)
{
    mRegistrationMessage.insert(QStringLiteral("token"), registration);
}

/*!
  Construct a new JsonClient instance with registration \a message and optional \a parent.
 */
JsonClient::JsonClient(const QJsonObject& message, QObject *parent)
    : QObject(parent),
      mRegistrationMessage(message),
      mStream(0)
{
}

/*!
  Construct a new JsonClient instance with an optional \a parent.
 */
JsonClient::JsonClient(QObject *parent)
    : QObject(parent),
      mStream(0)
{
}

/*!
  Destroy the JsonClient and shut down any active stream.
 */
JsonClient::~JsonClient()
{
    // Variant streams don't own the socket
    QIODevice *device = mStream.device();
    mStream.setDevice(0);
    if (device)
        delete device;
}

/*!
  Connect to the JsonServer over a TCP socket at \a hostname and \a port. Send the initial registration message.
  Return true if the connection is successful.
*/

bool JsonClient::connectTCP(const QString& hostname, int port)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->connectToHost(hostname, port);

    if (socket->waitForConnected()) {
        connect(socket, SIGNAL(disconnected()), SLOT(handleSocketDisconnected()));
        mStream.setDevice(socket);
        connect(&mStream, SIGNAL(receive(const QJsonObject&)),
                this, SIGNAL(receive(const QJsonObject&)));

        mStream.send(mRegistrationMessage);
        return true;
    }

    return false;
}

/*!
  Connect to the JsonServer over a Unix local socket to \a socketname and send the initial registration message.
  Return true if the connection is successful.
 */
bool JsonClient::connectLocal(const QString& socketname)
{
    if (!QFile::exists(socketname)) {
        qWarning() << Q_FUNC_INFO << "socket does not exist";
        return false;
    }

    QLocalSocket *socket = new QLocalSocket(this);
    socket->setReadBufferSize(64*1024);
    socket->connectToServer(socketname);

    if (socket->waitForConnected()) {
        connect(socket, SIGNAL(disconnected()), SLOT(handleSocketDisconnected()));
        mStream.setDevice(socket);
        connect(&mStream, SIGNAL(messageReceived(const QJsonObject&)),
                this, SIGNAL(messageReceived(const QJsonObject&)));
        // qDebug() << "Sending local socket registration message" << mRegistrationMessage;
        mStream.send(mRegistrationMessage);
        return true;
    }
    delete socket;
    return false;
}

/*!
  Send a \a message over the socket.
*/

void JsonClient::send(const QJsonObject &message)
{
    if (mStream.isOpen()) {
        mStream << message;
    } else {
        qCritical() << Q_FUNC_INFO << "stream socket is not available";
    }
}

/*!
  Set the current stream encoding \a format.
  This controls how messages will be sent
*/

void JsonClient::setFormat( EncodingFormat format )
{
    mStream.setFormat(format);
}

/*!
  \internal
*/
void JsonClient::handleSocketDisconnected()
{
    QIODevice *device = mStream.device();
    if (!device)
        return;

    mStream.setDevice(0);
    device->deleteLater();
    emit disconnected();
}


/*!
    \fn void JsonClient::messageReceived(const QJsonObject &message)
    This signal is emitted when a \a message is received from the server.
*/

/*!
    \fn void JsonClient::disconnected()
    This signal is emitted when the client socket is disconnected.
*/

#include "moc_jsonclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

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

#include "qjsonclient.h"
#include "qjsonstream.h"

#include <QLocalSocket>
#include <QTcpSocket>
#include <QCoreApplication>
#include <QFile>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE_JSONSTREAM

/****************************************************************************/

class QJsonClientPrivate
{
public:
    QJsonClientPrivate()
        : mStream(0) {}

    QJsonClientPrivate(const QJsonObject& message)
        : mRegistrationMessage(message)
        , mStream(0) {}

    QJsonObject  mRegistrationMessage;
    QJsonStream   mStream;
};

/****************************************************************************/

/*!
    \class QJsonClient
    \inmodule QtJsonStream
    \brief The QJsonClient class is used to send jsons to the QJsonServer.

    Note: The QJsonClient is not thread safe.
*/

/*!
  Construct a new QJsonClient instance with \a registration token and optional \a parent.
  The token will be passed in \c{{"token": registration}}.  This constructor
  is designed to work with QJsonTokenAuthority.
 */
QJsonClient::QJsonClient(const QString& registration, QObject* parent)
    : QObject(parent),
      d_ptr(new QJsonClientPrivate())
{
    Q_D(QJsonClient);
    d->mRegistrationMessage.insert(QStringLiteral("token"), registration);
}

/*!
  Construct a new QJsonClient instance with registration \a message and optional \a parent.
 */
QJsonClient::QJsonClient(const QJsonObject& message, QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonClientPrivate(message))
{
}

/*!
  Construct a new QJsonClient instance with an optional \a parent.
 */
QJsonClient::QJsonClient(QObject *parent)
    : QObject(parent),
      d_ptr(new QJsonClientPrivate())
{
}

/*!
  Destroy the QJsonClient and shut down any active stream.
 */
QJsonClient::~QJsonClient()
{
    // Variant streams don't own the socket
    Q_D(QJsonClient);
    QIODevice *device = d->mStream.device();
    d->mStream.setDevice(0);
    if (device)
        delete device;
}

/*!
  Connect to the QJsonServer over a TCP socket at \a hostname and \a port. Send the initial registration message.
  Return true if the connection is successful.
*/

bool QJsonClient::connectTCP(const QString& hostname, int port)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->connectToHost(hostname, port);

    if (socket->waitForConnected()) {
        connect(socket, SIGNAL(disconnected()), SLOT(handleSocketDisconnected()));
        Q_D(QJsonClient);
        d->mStream.setDevice(socket);
        connect(&d->mStream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));

        return d->mStream.send(d->mRegistrationMessage);
    }

    return false;
}

/*!
  Connect to the QJsonServer over a Unix local socket to \a socketname and send the initial registration message.
  Return true if the connection is successful.
 */
bool QJsonClient::connectLocal(const QString& socketname)
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
        Q_D(QJsonClient);
        d->mStream.setDevice(socket);
        connect(&d->mStream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
        // qDebug() << "Sending local socket registration message" << mRegistrationMessage;
        return d->mStream.send(d->mRegistrationMessage);
    }
    delete socket;
    return false;
}

/*!
  Send a \a message over the socket.
  Returns true if the entire message was send/buffered or false otherwise.
*/

bool QJsonClient::send(const QJsonObject &message)
{
    bool ret = false;
    Q_D(QJsonClient);
    if (d->mStream.isOpen()) {
        ret = d->mStream.send(message);
    } else {
        qCritical() << Q_FUNC_INFO << "stream socket is not available";
    }
    return ret;
}

/*!
  Set the current stream encoding \a format.
  This controls how messages will be sent
*/

void QJsonClient::setFormat( EncodingFormat format )
{
    Q_D(QJsonClient);
    d->mStream.setFormat(format);
}

/*!
  \internal
*/
void QJsonClient::handleSocketDisconnected()
{
    Q_D(QJsonClient);
    QIODevice *device = d->mStream.device();
    if (!device)
        return;

    d->mStream.setDevice(0);
    device->deleteLater();
    emit disconnected();
}


/*!
  \internal
*/
void QJsonClient::processMessages()
{
    Q_D(QJsonClient);
    while (d->mStream.messageAvailable()) {
        QJsonObject obj = d->mStream.readMessage();
        if (!obj.isEmpty())
            emit messageReceived(obj);
    }
}

/*!
    \fn void QJsonClient::messageReceived(const QJsonObject &message)
    This signal is emitted when a \a message is received from the server.
*/

/*!
    \fn void QJsonClient::disconnected()
    This signal is emitted when the client socket is disconnected.
*/

#include "moc_qjsonclient.cpp"

QT_END_NAMESPACE_JSONSTREAM

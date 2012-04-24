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

#include "jsonconnectionprocessor_p.h"
#include "jsonendpointmanager_p.h"
#include "jsonclient.h"
#include "jsonstream.h"
#include "jsonendpoint.h"
#include "jsonbuffer_p.h"

#include <QMap>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QTimer>

const int knAUTO_RECONNECTION_TIMEOUT = 5000;
const int knSOCKET_READ_BUFFER_SIZE = 64*1024;

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonConnectionProcessorPrivate
{
public:
    JsonConnectionProcessorPrivate()
        : mState(JsonConnection::Unconnected)
        , mManager(0)
        , mDestinationEndpoint(0)
        , mAutoReconnectEnabled(false)
        , mExplicitDisconnect(false)
        , mReconnectionTimer(0)
    {}

    JsonConnection::State mState;
    JsonEndpointManager *mManager;
    JsonStream mStream;
    QMutex          mutex;
    QJsonObject     mObject; // buffer for single message
    JsonEndpoint   *mDestinationEndpoint;

    QString mServerName;
    int mPort;

    // auto reconnection
    bool mAutoReconnectEnabled;
    bool mExplicitDisconnect;
    QTimer *mReconnectionTimer;
};

/****************************************************************************/

/*!
    \class JsonConnectionProcessor
    \brief The JsonConnectionProcessor class ...

*/

/*!
  Constructs a \c JsonConnectionProcessor object.
 */

JsonConnectionProcessor::JsonConnectionProcessor()
    : QObject(0)
    , d_ptr(new JsonConnectionProcessorPrivate())
{
    Q_D(JsonConnectionProcessor);
    d->mStream.setParent(this);
    d->mStream.setThreadProtection(true);
}

/*!
  Deletes the \c JsonConnectionProcessor object.
 */

JsonConnectionProcessor::~JsonConnectionProcessor()
{
    // Variant streams don't own the socket
    Q_D(JsonConnectionProcessor);
    QIODevice *device = d->mStream.device();
    if (device) {
        device->disconnect(this);
        d->mStream.setDevice(0);
        delete device;
    }
}

/*!
  Returns the state of the connection.
*/
JsonConnection::State JsonConnectionProcessor::state() const
{
    return d_func()->mState;
}

/*!
  Sets whether to reconnect when server connection is lost.
*/
void JsonConnectionProcessor::setAutoReconnectEnabled(bool enabled)
{
    Q_D(JsonConnectionProcessor);
    d->mAutoReconnectEnabled = enabled;
}

void JsonConnectionProcessor::setEndpointManager(JsonEndpointManager *manager)
{
    Q_D(JsonConnectionProcessor);
    QMutexLocker locker(&d->mutex);
    d->mManager = manager;
}

/*!
  Connect to the JsonServer over a TCP socket at \a hostname and \a port.
  Return true if the connection is successful.
*/

bool JsonConnectionProcessor::connectTCP(const QString& hostname, int port)
{
    Q_D(JsonConnectionProcessor);
    if (JsonConnection::Connecting != d->mState)
        emit stateChanged(d->mState = JsonConnection::Connecting);

    QTcpSocket *socket = new QTcpSocket(this);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(handleSocketError(QAbstractSocket::SocketError)));
    socket->connectToHost(hostname, port);

    if (socket->waitForConnected()) {
        connect(socket, SIGNAL(disconnected()), SLOT(handleSocketDisconnected()));
        d->mStream.setDevice(socket);
        connect(&d->mStream, SIGNAL(readyReadMessage()), this, SLOT(processMessage()));
        connect(&d->mStream, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
        connect(&d->mStream, SIGNAL(readBufferOverflow(qint64)), this, SIGNAL(readBufferOverflow(qint64)));
        d->mServerName = hostname;
        d->mPort = port;
        d->mState = JsonConnection::Connected;
        emit stateChanged(d->mState);
        return true;
    }

    d->mState = JsonConnection::Unconnected;
    emit stateChanged(d->mState);
    delete socket;
    return false;
}

/*!
  Connect to the JsonServer over a Unix local socket to \a socketname.
  Return true if the connection is successful.
 */
bool JsonConnectionProcessor::connectLocal(const QString& socketname)
{
    QString socketPath(socketname);
#if defined(Q_OS_UNIX)
    if (!socketPath.startsWith(QLatin1Char('/')))
        socketPath.prepend(QDir::tempPath() + QLatin1Char('/'));
#endif

    if (!QFile::exists(socketPath)) {
        qWarning() << Q_FUNC_INFO << "socket does not exist" << socketPath;
        return false;
    }

    Q_D(JsonConnectionProcessor);
    if (JsonConnection::Connecting != d->mState)
        emit stateChanged(d->mState = JsonConnection::Connecting);

    QLocalSocket *socket = new QLocalSocket(this);
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            SLOT(handleSocketError(QLocalSocket::LocalSocketError)));
    socket->setReadBufferSize(knSOCKET_READ_BUFFER_SIZE);
    socket->connectToServer(socketPath);

    if (socket->waitForConnected()) {
        connect(socket, SIGNAL(disconnected()), SLOT(handleSocketDisconnected()));
        d->mStream.setDevice(socket);
        connect(&d->mStream, SIGNAL(readyReadMessage()), this, SLOT(processMessage()));
        connect(&d->mStream, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
        connect(&d->mStream, SIGNAL(readBufferOverflow(qint64)), this, SIGNAL(readBufferOverflow(qint64)));
        d->mServerName = socketname;
        d->mPort = -1; // local socket
        d->mState = JsonConnection::Connected;
        emit stateChanged(d->mState);
        return true;
    }

    d->mState = JsonConnection::Unconnected;
    emit stateChanged(d->mState);
    delete socket;
    return false;
}

/*!
  \internal
*/
void JsonConnectionProcessor::handleSocketDisconnected()
{
    Q_D(JsonConnectionProcessor);
    QIODevice *device = d->mStream.device();
    if (!device)
        return;

    device->disconnect(this);
    d->mStream.setDevice(0);
    device->deleteLater();

    if (d->mAutoReconnectEnabled && !d->mExplicitDisconnect)
    {
        if (!d->mReconnectionTimer || !d->mReconnectionTimer->isActive())
        {
            if (!d->mReconnectionTimer) {
                // create timer
                d->mReconnectionTimer = new QTimer(this);
                d->mReconnectionTimer->setInterval(knAUTO_RECONNECTION_TIMEOUT);
                connect(d->mReconnectionTimer, SIGNAL(timeout()), SLOT(handleReconnect()));
            }
            d->mState = JsonConnection::Connecting;
            emit stateChanged(d->mState);
            d->mReconnectionTimer->start();
        }
        return;
    }

    d->mState = JsonConnection::Unconnected;
    emit disconnected();
    emit stateChanged(d->mState);
}

/*!
  \internal
*/
void JsonConnectionProcessor::handleSocketError(QAbstractSocket::SocketError _error)
{
    if (QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(sender())) {
        emit error(JsonConnection::TcpSocketError, _error, socket->errorString());
    }
}

/*!
  \internal
*/
void JsonConnectionProcessor::handleSocketError(QLocalSocket::LocalSocketError _error)
{
    if (QLocalSocket *socket = qobject_cast<QLocalSocket*>(sender())) {
        emit error(JsonConnection::LocalSocketError, _error, socket->errorString());
    }
}

/*!
  \internal
*/
void JsonConnectionProcessor::handleReconnect()
{
    Q_D(JsonConnectionProcessor);
    d->mReconnectionTimer->stop();

    if (JsonConnection::Connecting == d->mState) {
        if (d->mPort < 0)
            connectLocal(d->mServerName);
        else
            connectTCP(d->mServerName, d->mPort);
    }
}

/*!
  \internal
  Handle a received readyReadMessage signal and emit the correct signals
*/

void JsonConnectionProcessor::processMessage(JsonEndpoint *destination)
{
    Q_D(JsonConnectionProcessor);
    if (!destination)
        d->mutex.lock();
    if (!d->mDestinationEndpoint  && d->mManager) { // no message available
        if (d->mStream.messageAvailable()) {
            QJsonObject obj = d->mStream.readMessage();
            if (!obj.isEmpty()) {
                JsonEndpoint *endpoint = d->mManager->endpoint(obj);
                if (endpoint) {
                    // find a corresponding endpoint and notify it
                    d->mDestinationEndpoint = endpoint;
                    d->mObject = obj;

                    if (destination != endpoint) {
                        // do not emit a signal if destination endpoint is already processing messages
                        if (!destination)
                            d->mutex.unlock();
                        // use a queued signal if we process messages in one endpoint and need to notify another
                        QMetaObject::invokeMethod(endpoint,
                                                  "slotReadyReadMessage",
                                                  !destination ? Qt::AutoConnection : Qt::QueuedConnection,
                                                  QGenericReturnArgument());
                        return;
                    }
                }
            }
        }
    }
    if (!destination)
        d->mutex.unlock();
}

/*!
  Returns \b true if a message is available for \a endpoint to be read via \l{readMessage()}
  or \b false otherwise.
 */

bool JsonConnectionProcessor::messageAvailable(JsonEndpoint *endpoint)
{
    bool ret = false;
    if (endpoint) {
        Q_D(JsonConnectionProcessor);
        QMutexLocker locker(&d->mutex);
        if (!(ret = (d->mDestinationEndpoint == endpoint))) {
            if (!d->mDestinationEndpoint) {
                // check stream for more if no messages available
                processMessage(endpoint);
                ret = (d->mDestinationEndpoint == endpoint);
            }
        }
    }
    return ret;
}

/*!
  Returns a JSON object that has been received for \a endpoint.  If no message is
  available, an empty JSON object is returned.
 */
QJsonObject JsonConnectionProcessor::readMessage(JsonEndpoint *endpoint)
{
    QJsonObject obj;
    if (endpoint) {
        Q_D(JsonConnectionProcessor);
        QMutexLocker locker(&d->mutex);
        if (!d->mDestinationEndpoint) {
            // check stream for more if no messages available
            processMessage(endpoint);
        }

        if (d->mDestinationEndpoint == endpoint) {
            obj = d->mObject;
            d->mObject = QJsonObject();
            d->mDestinationEndpoint = 0;
        }
    }
    return obj;
}

/*!
  Sets a maximum size of the inbound message buffer to \a sz thus capping a size
  of an inbound message.
 */
void JsonConnectionProcessor::setReadBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonConnectionProcessor);
        d->mStream.setReadBufferSize(sz);
    }
}

/*!
  Sets a maximum size of the outbound message buffer to \a sz thus capping a size
  of an outbound message.  A value of 0 means the buffer size is unlimited.
 */
void JsonConnectionProcessor::setWriteBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonConnectionProcessor);
        d->mStream.setWriteBufferSize(sz);
    }
}

/*!
  Set the current stream encoding \a format.
  This controls how messages will be sent
*/

void JsonConnectionProcessor::setFormat( int format )
{
    Q_D(JsonConnectionProcessor);
    d->mStream.setFormat((EncodingFormat)format); // EncodingFormat
}

/*!
  Send a \a message over the socket.
  Returns true if the entire message was send/buffered or false otherwise.
*/

bool JsonConnectionProcessor::send(QJsonObject message)
{
    Q_D(JsonConnectionProcessor);
    return d->mStream.send(message);
}

#include "moc_jsonconnectionprocessor_p.cpp"

QT_END_NAMESPACE_JSONSTREAM


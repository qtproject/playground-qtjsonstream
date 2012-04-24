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

#include "jsonconnection.h"
#include "jsonconnectionprocessor_p.h"
#include "jsonendpoint.h"
#include "jsonendpointmanager_p.h"
#include <QThread>
#include <QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonConnectionPrivate
{
public:
    JsonConnectionPrivate()
        : mTcpHostPort(0)
        , mAutoReconnectEnabled(false)
        , mUseSeparateThread(false)
        , mReadBufferSize(0)
        , mWriteBufferSize(0)
        , mManager(0)
        , mConnected(0)
        , mProcessorThread(0)
        , mError(JsonConnection::NoError)
        , mSubError(0) {}

    QString     mLocalSocketName;
    QString     mTcpHostName;
    int         mTcpHostPort;
    bool        mAutoReconnectEnabled;
    bool        mUseSeparateThread;
    qint64      mReadBufferSize;
    qint64      mWriteBufferSize;
    JsonEndpointManager *mManager;
    JsonConnectionProcessor *mProcessor;
    bool mConnected;
    QThread *mProcessorThread;

    JsonConnection::Error mError;
    int                   mSubError;
    QString               mErrorStr;

    /*!
      \internal
     */
    void createProcessorThread()
    {
        if (!mUseSeparateThread)
            return;

        mProcessorThread = new QThread();
        QObject::connect(mProcessorThread, SIGNAL(finished()), mProcessor, SLOT(deleteLater()));
        QObject::connect(mProcessorThread, SIGNAL(finished()), mProcessorThread, SLOT(deleteLater()));

        mProcessor->moveToThread(mProcessorThread);
        mProcessorThread->start();
    }
};

/****************************************************************************/

/*!
    \class JsonConnection
    \brief The JsonConnection class ...

*/

/*!
    \enum JsonConnection::State

    This enum describes the different states in which a connection can be.

    \value Unconnected No connected.
    \value Connecting Started establishing a connection.
    \value Authenticating Started authentication process.
    \value Connected A connection is established.

    \sa JsonConnection::state()
*/

/*!
  Constructs a \c JsonConnection object.
 */

JsonConnection::JsonConnection(QObject *parent)
    : QObject(parent)
    , d_ptr(new JsonConnectionPrivate())
{
    Q_D(JsonConnection);
    d->mManager = new JsonEndpointManager(this);
    d->mProcessor = new JsonConnectionProcessor();
    d->mProcessor->setEndpointManager(d->mManager);
    connect(d->mProcessor, SIGNAL(disconnected()), SIGNAL(disconnected()));
    qRegisterMetaType<JsonConnection::State>("JsonConnection::State");
    connect(d->mProcessor, SIGNAL(stateChanged(JsonConnection::State)), SIGNAL(stateChanged(JsonConnection::State)));
    qRegisterMetaType<JsonConnection::State>("JsonConnection::Error");
    connect(d->mProcessor, SIGNAL(error(JsonConnection::Error,int,QString)), SLOT(handleError(JsonConnection::Error,int,QString)));
}

/*!
  Deletes the \c JsonConnection object.
 */

JsonConnection::~JsonConnection()
{
    Q_D(JsonConnection);
    d->mProcessor->setEndpointManager(0);
    if (d->mProcessorThread)
        d->mProcessorThread->quit();
    if (!d->mUseSeparateThread)
        delete d->mProcessor;
}

/*!
  Returns the state of the connection.
*/
JsonConnection::State JsonConnection::state() const
{
    Q_D(const JsonConnection);
    return d->mProcessor->state();
}

/*!
    Returns the type of error that last occurred.

    \sa state(), errorString(), subError()
*/

JsonConnection::Error JsonConnection::error() const
{
    Q_D(const JsonConnection);
    return d->mError;
}

/*!
  Returns the additional error code of error that last occurred. This code depends on an error
  type. It could be casted to QLocalSocket::LocalSocketError for LocalSocketError error and to
  QAbstractSocket::SocketError for TcpSocketError error.

  \sa error()
*/

int JsonConnection::subError() const
{
    Q_D(const JsonConnection);
    return d->mSubError;
}

/*!
  Returns a human-readable description of the last device error that occurred.

  \sa error()
*/

QString JsonConnection::errorString() const
{
    Q_D(const JsonConnection);
    return d->mErrorStr;
}

/*!
  Returns a socket name to be used for Unix local socket connection.
*/
QString JsonConnection::localSocketName() const
{
    Q_D(const JsonConnection);
    return d->mLocalSocketName;
}

/*!
  Sets a socket name to be used for Unix local socket connection.
*/
void JsonConnection::setLocalSocketName(const QString &name)
{
    Q_D(JsonConnection);
    d->mLocalSocketName = name;
}

/*!
  Returns a host name to be used for TCP socket connection.
*/
QString JsonConnection::tcpHostName() const
{
    Q_D(const JsonConnection);
    return d->mTcpHostName;
}

/*!
  Sets a host name to be used for TCP socket connection.
*/
void JsonConnection::setTcpHostName(const QString &name)
{
    Q_D(JsonConnection);
    d->mTcpHostName = name;
}

/*!
  Returns a port number to be used for TCP socket connection.
*/
int JsonConnection::tcpHostPort() const
{
    Q_D(const JsonConnection);
    return d->mTcpHostPort;
}

/*!
  Sets a port number to be used for TCP socket connection.
*/
void JsonConnection::setTcpHostPort(int port)
{
    Q_D(JsonConnection);
    if (port >= 0)
        d->mTcpHostPort = port;
}

/*!
  Specifies whether to reconnect when server connection is lost.
*/
bool JsonConnection::autoReconnectEnabled() const
{
    Q_D(const JsonConnection);
    return d->mAutoReconnectEnabled;
}

/*!
  Sets whether to reconnect when server connection is lost.
*/
void JsonConnection::setAutoReconnectEnabled(bool enabled)
{
    Q_D(JsonConnection);
    d->mAutoReconnectEnabled = enabled;
    d->mProcessor->setAutoReconnectEnabled(enabled);
}

/*!
    Returns a property which value in message object will be used as an endpoint name.
*/
QString JsonConnection::endpointPropertyName() const
{
    Q_D(const JsonConnection);
    return d->mManager->endpointPropertyName();
}

/*!
    Sets a property which value in message object will be used as an endpoint name.
*/
void JsonConnection::setEndpointPropertyName(const QString &property)
{
    Q_D(JsonConnection);
    if (d->mManager) {
        d->mManager->setEndpointPropertyName(property);
    }
}

/*!
  Returns a maximum size of the inbound message buffer.
 */
qint64 JsonConnection::readBufferSize() const
{
    Q_D(const JsonConnection);
    return d->mReadBufferSize;
}

/*!
  Sets a maximum size of the inbound message buffer to \a sz thus capping a size
  of an inbound message.
 */
void JsonConnection::setReadBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonConnection);
        d->mReadBufferSize = sz;

        if (!d->mUseSeparateThread || !d->mConnected)
            d->mProcessor->setReadBufferSize(sz);
        else
            QMetaObject::invokeMethod(d->mProcessor,
                                  "setReadBufferSize",
                                  Qt::QueuedConnection,
                                  QGenericReturnArgument(),
                                  Q_ARG(qint64, sz));
    }
}

/*!
  Returns a maximum size of the outbound message buffer.  A value of 0
  means the buffer size is unlimited.
 */
qint64 JsonConnection::writeBufferSize() const
{
    Q_D(const JsonConnection);
    return d->mWriteBufferSize;
}

/*!
  Sets a maximum size of the outbound message buffer to \a sz thus capping a size
  of an outbound message.  A value of 0 means the buffer size is unlimited.
 */
void JsonConnection::setWriteBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonConnection);
        d->mWriteBufferSize = sz;

        if (!d->mUseSeparateThread || !d->mConnected)
            d->mProcessor->setWriteBufferSize(sz);
        else
            QMetaObject::invokeMethod(d->mProcessor,
                                  "setWriteBufferSize",
                                  Qt::QueuedConnection,
                                  QGenericReturnArgument(),
                                  Q_ARG(qint64, sz));
    }
}

/*!
  Adds endpoint to a connection.
 */
void JsonConnection::addEndpoint(JsonEndpoint *endpoint)
{
    Q_D(JsonConnection);
    d->mManager->addEndpoint(endpoint);
    if (endpoint->connection() != this)
        endpoint->setConnection(this);
}

/*!
  Removes endpoint from a connection.
 */
void JsonConnection::removeEndpoint(JsonEndpoint *endpoint)
{
    Q_D(JsonConnection);
    d->mManager->removeEndpoint(endpoint);
}

/*!
  Sets whether to create a separate worker thread for a connection
 */
void JsonConnection::setUseSeparateThreadForProcessing(bool use)
{
    Q_D(JsonConnection);

    Q_ASSERT(!d->mConnected);
    if (!d->mConnected)
        d->mUseSeparateThread = use;
}

/*!
  Returns whether a separate worker thread for a connection required
*/

bool JsonConnection::useSeparateThreadForProcessing() const
{
    Q_D(const JsonConnection);
    return d->mUseSeparateThread;
}

/*!
  Returns a default endpoint without name.
*/

JsonEndpoint * JsonConnection::defaultEndpoint()
{
    Q_D(JsonConnection);
    return d->mManager->defaultEndpoint();
}

/*!
  Connect to the JsonServer over a TCP socket at \a hostname and \a port.
  Return true if the connection is successful.
*/

bool JsonConnection::connectTCP(const QString& hostname, int port)
{
    bool bRet = false;
    Q_D(JsonConnection);

    if (!d->mConnected) {
        d->mConnected = true;
        if (d->mUseSeparateThread) {
            d->createProcessorThread();
        }
        connect(d->mProcessor, SIGNAL(readBufferOverflow(qint64)), SIGNAL(readBufferOverflow(qint64)),
                d->mUseSeparateThread ? Qt::BlockingQueuedConnection : Qt::AutoConnection);
    }

    if (!hostname.isEmpty())
        setTcpHostName(hostname);
    if (port > 0)
        setTcpHostPort(port);

    if (!d->mUseSeparateThread)
        bRet = d->mProcessor->connectTCP(tcpHostName(), tcpHostPort());
    else
        QMetaObject::invokeMethod(d->mProcessor,
                              "connectTCP",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, bRet),
                              Q_ARG(QString, tcpHostName()),
                              Q_ARG(int, tcpHostPort()));
    return bRet;
}

/*!
  Connect to the JsonServer over a Unix local socket to \a socketname.
  Return true if the connection is successful.
 */
bool JsonConnection::connectLocal(const QString& socketname)
{
    bool bRet = false;
    Q_D(JsonConnection);

    if (!d->mConnected) {
        d->mConnected = true;
        if (d->mUseSeparateThread) {
            d->createProcessorThread();
        }
        connect(d->mProcessor, SIGNAL(readBufferOverflow(qint64)), SIGNAL(readBufferOverflow(qint64)),
                d->mUseSeparateThread ? Qt::BlockingQueuedConnection : Qt::AutoConnection);
    }

    if (!socketname.isEmpty())
        setLocalSocketName(socketname);

    if (!d->mUseSeparateThread)
        bRet = d->mProcessor->connectLocal(localSocketName());
    else
        QMetaObject::invokeMethod(d->mProcessor,
                              "connectLocal",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, bRet),
                              Q_ARG(QString, localSocketName()));
    return bRet;
}

/*!
  Set the current stream encoding \a format.
  This controls how messages will be sent
*/

void JsonConnection::setFormat( EncodingFormat format )
{
    Q_D(JsonConnection);
    if (!d->mUseSeparateThread || !d->mConnected)
        d->mProcessor->setFormat(format);
    else
        QMetaObject::invokeMethod(d->mProcessor,
                              "setFormat",
                              Qt::QueuedConnection,
                              QGenericReturnArgument(),
                              Q_ARG(int, format));
}

/*!
  \internal
*/
JsonConnectionProcessor *JsonConnection::processor() const
{
    Q_D(const JsonConnection);
    return d->mProcessor;
}

/*!
  \internal
*/
void JsonConnection::handleError(JsonConnection::Error err, int suberr, QString str)
{
    Q_D(JsonConnection);
    d->mError = err;
    d->mSubError = suberr;
    d->mErrorStr = str;
    emit error(d->mError, d->mSubError);
}

/*! \fn JsonConnection::bytesWritten(qint64 bytes)

    This signal is emitted every time a payload of data has been
    written to the device. The \a bytes argument is set to the number
    of bytes that were written in this payload.
*/

/*! \fn JsonConnection::readBufferOverflow(qint64 bytes)

  This signal is emitted when the read buffer is full of data that has been read
  from the \l{device()}, \a bytes additional bytes are available on the device,
  but the message is not complete.  The \l{readBufferSize()} may be increased
  to a sufficient size in a slot connected to this signal, in which case more
  data will be read into the read buffer.  If the buffer size  is not increased,
  the connection is closed.
*/

/*!
    \fn void JsonConnection::stateChanged(JsonConnection::State state)

    This signal is emitted whenever JsonConnection's state changes.
    The \a state parameter is the new state.

    \sa state()
*/

/*!
    \fn void JsonConnection::error(JsonConnection::Error error, int subError)

    This signal is emitted after an error occurred. The \a error
    parameter describes the type of error that occurred and \a subError
    contains the additional error code.

    \sa error(), subError(), errorString()
*/

/*!
    \fn void JsonConnection::disconnected()

    This signal is emitted when the connection has been disconnected.

    \warning If you need to delete the sender() of this signal in a slot connected
    to it, use the \l{QObject::deleteLater()}{deleteLater()} function.
*/

#include "moc_jsonconnection.cpp"

QT_END_NAMESPACE_JSONSTREAM

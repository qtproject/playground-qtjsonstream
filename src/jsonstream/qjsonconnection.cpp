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

#include "qjsonconnection.h"
#include "qjsonconnectionprocessor_p.h"
#include "qjsonendpoint.h"
#include "qjsonendpointmanager_p.h"
#include <QThread>
#include <QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonConnectionPrivate
{
public:
    QJsonConnectionPrivate()
        : mTcpHostPort(0)
        , mAutoReconnectEnabled(false)
        , mUseSeparateThread(false)
        , mReadBufferSize(0)
        , mWriteBufferSize(0)
        , mManager(0)
        , mConnected(0)
        , mProcessorThread(0)
        , mError(QJsonConnection::NoError)
        , mSubError(0) {}

    QString     mLocalSocketName;
    QString     mTcpHostName;
    int         mTcpHostPort;
    bool        mAutoReconnectEnabled;
    bool        mUseSeparateThread;
    qint64      mReadBufferSize;
    qint64      mWriteBufferSize;
    QJsonEndpointManager *mManager;
    QJsonConnectionProcessor *mProcessor;
    bool mConnected;
    QThread *mProcessorThread;

    QJsonConnection::Error mError;
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
    \class QJsonConnection
    \inmodule QtJsonStream
    \brief The QJsonConnection class is a thread-safe client connection object that can be
           used by multiple endpoint objects.

    QJsonConnection implements a thread-safe client connection object.
    Multiple QJsonEndpoint objects (possibly from different threads) can use a
    QJsonConnection, providing a type of multiplexing on a single connection.  The
    endpointPropertyName() property specifies the property that controls which endpoint
    an inbound message should be delivered to.  A default endpoint is provided via
    defaultEndpoint().

    The processing of the connection can take place in the connection object's
    affined thread or a separate thread, depending on the value of
    useSeparateThreadForProcessing().
*/

/*!
    \enum QJsonConnection::State

    This enumuration describes the different states in which a connection can be.

    \value Unconnected Not connected.
    \value Connecting Started establishing a connection.
    \value Authenticating Started authentication process.
    \value Connected Connection is established.
*/

/*!
    \enum QJsonConnection::Error

    This enumuration describes error conditions for QJsonConnection.

    \value NoError No error.
    \value UnknownError Unknown error.
    \value LocalSocketError subError() is a local socket error code.
    \value TcpSocketError subError() is a TCP socket error code.
*/

/*!
  Constructs a \c QJsonConnection object with \a parent.
 */

QJsonConnection::QJsonConnection(QObject *parent)
    : QObject(parent)
    , d_ptr(new QJsonConnectionPrivate())
{
    Q_D(QJsonConnection);
    d->mManager = new QJsonEndpointManager(this);
    d->mProcessor = new QJsonConnectionProcessor();
    d->mProcessor->setEndpointManager(d->mManager);
    connect(d->mProcessor, SIGNAL(disconnected()), SIGNAL(disconnected()));
    qRegisterMetaType<QJsonConnection::State>("QJsonConnection::State");
    connect(d->mProcessor, SIGNAL(stateChanged(QJsonConnection::State)), SIGNAL(stateChanged(QJsonConnection::State)));
    qRegisterMetaType<QJsonConnection::Error>("QJsonConnection::Error");
    connect(d->mProcessor, SIGNAL(error(QJsonConnection::Error,int,QString)), SLOT(handleError(QJsonConnection::Error,int,QString)));
}

/*!
  Deletes the \c QJsonConnection object.
 */

QJsonConnection::~QJsonConnection()
{
    Q_D(QJsonConnection);
    d->mProcessor->setEndpointManager(0);
    if (d->mProcessorThread)
        d->mProcessorThread->quit();
    if (!d->mUseSeparateThread)
        delete d->mProcessor;
}

/*!
  Returns the state of the connection.
*/
QJsonConnection::State QJsonConnection::state() const
{
    Q_D(const QJsonConnection);
    return d->mProcessor->state();
}

/*!
    Returns the type of error that last occurred.

    \sa state(), errorString(), subError()
*/

QJsonConnection::Error QJsonConnection::error() const
{
    Q_D(const QJsonConnection);
    return d->mError;
}

/*!
  Returns the additional error code of error that last occurred. This code depends on an error
  type. It could be casted to QLocalSocket::LocalSocketError for LocalSocketError error and to
  QAbstractSocket::SocketError for TcpSocketError error.

  \sa error()
*/

int QJsonConnection::subError() const
{
    Q_D(const QJsonConnection);
    return d->mSubError;
}

/*!
  Returns a human-readable description of the last device error that occurred.

  \sa error()
*/

QString QJsonConnection::errorString() const
{
    Q_D(const QJsonConnection);
    return d->mErrorStr;
}

/*!
  Returns the socket name to be used for Unix local socket connection.
*/
QString QJsonConnection::localSocketName() const
{
    Q_D(const QJsonConnection);
    return d->mLocalSocketName;
}

/*!
  Sets a socket name to be used for Unix local socket connection.
*/
void QJsonConnection::setLocalSocketName(const QString &name)
{
    Q_D(QJsonConnection);
    d->mLocalSocketName = name;
}

/*!
  Returns the host name to be used for TCP socket connection.
*/
QString QJsonConnection::tcpHostName() const
{
    Q_D(const QJsonConnection);
    return d->mTcpHostName;
}

/*!
  Sets a host name to be used for TCP socket connection.
*/
void QJsonConnection::setTcpHostName(const QString &name)
{
    Q_D(QJsonConnection);
    d->mTcpHostName = name;
}

/*!
  Returns the port number to be used for TCP socket connection.
*/
int QJsonConnection::tcpHostPort() const
{
    Q_D(const QJsonConnection);
    return d->mTcpHostPort;
}

/*!
  Sets a port number to be used for TCP socket connection.
*/
void QJsonConnection::setTcpHostPort(int port)
{
    Q_D(QJsonConnection);
    if (port >= 0)
        d->mTcpHostPort = port;
}

/*!
  Specifies whether to reconnect when server connection is lost.
*/
bool QJsonConnection::autoReconnectEnabled() const
{
    Q_D(const QJsonConnection);
    return d->mAutoReconnectEnabled;
}

/*!
  Sets whether to reconnect when server connection is lost.
*/
void QJsonConnection::setAutoReconnectEnabled(bool enabled)
{
    Q_D(QJsonConnection);
    d->mAutoReconnectEnabled = enabled;
    d->mProcessor->setAutoReconnectEnabled(enabled);
}

/*!
    Returns the property which value in message object will be used as an endpoint name.
*/
QString QJsonConnection::endpointPropertyName() const
{
    Q_D(const QJsonConnection);
    return d->mManager->endpointPropertyName();
}

/*!
    Sets a property which value in message object will be used as an endpoint name.
*/
void QJsonConnection::setEndpointPropertyName(const QString &property)
{
    Q_D(QJsonConnection);
    if (d->mManager) {
        d->mManager->setEndpointPropertyName(property);
    }
}

/*!
  Returns the maximum size of the inbound message buffer.
 */
qint64 QJsonConnection::readBufferSize() const
{
    Q_D(const QJsonConnection);
    return d->mReadBufferSize;
}

/*!
  Sets a maximum size of the inbound message buffer to \a sz thus capping a size
  of an inbound message.
 */
void QJsonConnection::setReadBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(QJsonConnection);
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
  Returns the maximum size of the outbound message buffer.  A value of 0
  means the buffer size is unlimited.
 */
qint64 QJsonConnection::writeBufferSize() const
{
    Q_D(const QJsonConnection);
    return d->mWriteBufferSize;
}

/*!
  Sets a maximum size of the outbound message buffer to \a sz thus capping a size
  of an outbound message.  A value of 0 means the buffer size is unlimited.
 */
void QJsonConnection::setWriteBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(QJsonConnection);
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
  Adds \a endpoint to the connection.  Messages routed for the endpoint's
  \l{QJsonEndpoint::name()}{name()} will be delivered to the endpoint.
 */
void QJsonConnection::addEndpoint(QJsonEndpoint *endpoint)
{
    Q_D(QJsonConnection);
    d->mManager->addEndpoint(endpoint);
    if (endpoint->connection() != this)
        endpoint->setConnection(this);
}

/*!
  Removes \a endpoint from the connection.  No further messages will be delivered
  to the endpoint.
 */
void QJsonConnection::removeEndpoint(QJsonEndpoint *endpoint)
{
    Q_D(QJsonConnection);
    d->mManager->removeEndpoint(endpoint);
}

/*!
  Sets whether a separate connection processing thread should be used.
 */
void QJsonConnection::setUseSeparateThreadForProcessing(bool use)
{
    Q_D(QJsonConnection);

    Q_ASSERT(!d->mConnected);
    if (!d->mConnected)
        d->mUseSeparateThread = use;
}

/*!
  Returns whether a separate connection processing thread should be used.
*/
bool QJsonConnection::useSeparateThreadForProcessing() const
{
    Q_D(const QJsonConnection);
    return d->mUseSeparateThread;
}

/*!
  Returns the default endpoint.  This endpoint will be used to process any
  messages that cannot be directed to a named endpoint.  You must not change
  the name of the default endpoint.
*/

QJsonEndpoint * QJsonConnection::defaultEndpoint()
{
    Q_D(QJsonConnection);
    return d->mManager->defaultEndpoint();
}

/*!
  Connect to the QJsonServer over a TCP socket at \a hostname and \a port.
  Return true if the connection is successful.
*/

bool QJsonConnection::connectTCP(const QString& hostname, int port)
{
    bool bRet = false;
    Q_D(QJsonConnection);

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
  Connect to the QJsonServer over a Unix local socket to \a socketname.
  Return true if the connection is successful.
 */
bool QJsonConnection::connectLocal(const QString& socketname)
{
    bool bRet = false;
    Q_D(QJsonConnection);

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

void QJsonConnection::setFormat( EncodingFormat format )
{
    Q_D(QJsonConnection);
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
QJsonConnectionProcessor *QJsonConnection::processor() const
{
    Q_D(const QJsonConnection);
    return d->mProcessor;
}

/*!
  \internal
*/
void QJsonConnection::handleError(QJsonConnection::Error err, int suberr, QString str)
{
    Q_D(QJsonConnection);
    d->mError = err;
    d->mSubError = suberr;
    d->mErrorStr = str;
    emit error(d->mError, d->mSubError);
}

/*! \property QJsonConnection::localSocketName
  File path of a local socket to be used for local socket connections.
*/

/*! \property QJsonConnection::tcpHostName
  Host name to be used for TCP socket connections.
*/

/*! \property QJsonConnection::tcpHostPort
  Port number to be used for TCP socket connections.
*/

/*! \property QJsonConnection::state
  The state of the connection.
 */

/*! \property QJsonConnection::autoReconnectEnabled
  Specifies if automatic reconnection to the server should be attempted if the connection is lost.
*/

/*! \property QJsonConnection::endpointPropertyName
  Specifies a property in inbound JSON messages whose value will be used to determine
  which endpoint the message should be delivered to.
*/

/*! \property QJsonConnection::readBufferSize
  The maximum size of the read buffer.  Messages that are larger than the read buffer
  size will be rejected and the connection will be closed.  A value of 0 means
  the read buffer is unlimited.
 */

/*! \property QJsonConnection::writeBufferSize
  The maximum size of the outbound message buffer.  Messages that are larger than the
  write buffer size will not be sent.  A value of 0 means the buffer size is unlimited.
 */

/*! \property QJsonConnection::useSeparateThreadForProcessing
  Specifies whether the connection should be processed in a separate thread (created and
  controlled by QJsonConnection) or in the object's affined thread.
*/

/*! \property QJsonConnection::defaultEndpoint
  Specifies the default endpoint.  This endpoint will be used to process any
  messages that cannot be directed to a named endpoint.  You must not change
  the name of the default endpoint.
*/

/*! \fn QJsonConnection::bytesWritten(qint64 bytes)

    This signal is emitted every time a payload of data has been
    written to the device. The \a bytes argument is set to the number
    of bytes that were written in this payload.
*/

/*! \fn QJsonConnection::readBufferOverflow(qint64 bytes)

  This signal is emitted when the read buffer is full of data that has been read
  from the stream, \a bytes additional bytes are available on the stream,
  but the message is not complete.  The \l{readBufferSize()} may be increased
  to a sufficient size in a slot connected to this signal, in which case more
  data will be read into the read buffer.  If the read buffer size is not increased,
  the connection is closed.
*/

/*!
    \fn void QJsonConnection::stateChanged(QJsonConnection::State state)

    This signal is emitted whenever QJsonConnection's state changes.
    The \a state parameter is the new state.
*/

/*!
    \fn void QJsonConnection::error(QJsonConnection::Error error, int subError)

    This signal is emitted after an error occurred. The \a error
    parameter describes the type of error that occurred and \a subError
    contains the additional error code.

    \sa subError(), errorString()
*/

/*!
    \fn void QJsonConnection::disconnected()

    This signal is emitted when the connection has been disconnected.

    \warning If you need to delete the sender() of this signal in a slot connected
    to it, use the \l{QObject::deleteLater()}{deleteLater()} function.
*/

#include "moc_qjsonconnection.cpp"

QT_END_NAMESPACE_JSONSTREAM

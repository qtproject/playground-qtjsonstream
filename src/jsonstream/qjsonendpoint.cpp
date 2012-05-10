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

#include "qjsonendpoint.h"
#include "qjsonconnection.h"
#include "qjsonconnectionprocessor_p.h"
#include <qjsonobject.h>
#include <QVariant>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonEndpointPrivate
{
public:
    QJsonEndpointPrivate()
        : mConnection(0)
        , mEmittedReadyRead(false)
        , mMessageReady(false)
    {
    }

    QString             mName;
    QJsonConnection     *mConnection;
    bool                mEmittedReadyRead;
    bool                mMessageReady;
};

/****************************************************************************/

/*!
    \class QJsonEndpoint
    \brief The QJsonEndpoint class ...

*/

/*!
  Constructs a \c QJsonEndpoint object.
 */

QJsonEndpoint::QJsonEndpoint(const QString & name, QJsonConnection *connection)
    : QObject(0)
    , d_ptr(new QJsonEndpointPrivate())
{
    Q_D(QJsonEndpoint);
    d->mName = name;

    setConnection(connection);
}

/*!
  Deletes the \c QJsonEndpoint object.
 */

QJsonEndpoint::~QJsonEndpoint()
{
    Q_D(QJsonEndpoint);
    if (d->mConnection)
        d->mConnection->removeEndpoint(this);
}

/*!
  Returns a name used for message multiplexing. A default endpoint should not
  have a name
 */
QString QJsonEndpoint::name() const
{
    Q_D(const QJsonEndpoint);
    return d->mName;
}

/*!
  Sets a \a name used for message multiplexing. A default endpoint should not
  have a name
 */
void QJsonEndpoint::setName( const QString & name )
{
    Q_D(QJsonEndpoint);
    d->mName = name;
    emit nameChanged();
}

/*!
  Returns a connection that is used by endpoint
 */
QJsonConnection *QJsonEndpoint::connection() const
{
    Q_D(const QJsonEndpoint);
    return d->mConnection;
}

/*!
  Sets a \a connection to be used by endpoint
 */
void QJsonEndpoint::setConnection(QJsonConnection *connection)
{
    Q_D(QJsonEndpoint);
    d->mConnection = connection;
    if (d->mConnection)
        d->mConnection->addEndpoint(this);

    emit connectionChanged();
}

/*!
  Send a QVariantMap \a message over the connection.
  Returns \b true if the entire message was sent or buffered or \b false otherwise.
*/
bool QJsonEndpoint::send(const QVariantMap& message)
{
    return send(QJsonObject::fromVariantMap(message));
}

/*!
  Send a QJsonObject \a message over the connection.
  Returns \b true if the entire message was sent or buffered or \b false otherwise.
*/
bool QJsonEndpoint::send(const QJsonObject& message)
{
    bool ret = false;
    Q_D(const QJsonEndpoint);
    if (d->mConnection) {
        if (!d->mConnection->useSeparateThreadForProcessing()) {
            ret = d->mConnection->processor()->send(message);
        }
        else {
            QMetaObject::invokeMethod(d->mConnection->processor(),
                                      "send",
                                      Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(bool, ret),
                                      Q_ARG(QJsonObject, message));
        }
    }
    return ret;
}

/*!
  \internal
  Handle a notification from connection processor and emit the correct signals
*/
void QJsonEndpoint::slotReadyReadMessage()
{
    Q_D(QJsonEndpoint);
    d->mMessageReady = true;
    if (!d->mEmittedReadyRead) {
        d->mEmittedReadyRead = true;
        emit readyReadMessage();
        d->mEmittedReadyRead = false;
    }
}


/*!
  Returns \b true if a message is available to be read via \l{readMessage()}
  or \b false otherwise.
 */
bool QJsonEndpoint::messageAvailable()
{
    Q_D(QJsonEndpoint);
    bool ret = d->mMessageReady;
    if (!d->mMessageReady) {
        // check again
        if (d->mConnection) {
            ret = d->mConnection->processor()->messageAvailable(this);
            d->mMessageReady = ret;
        }
    }
    return ret;
}

/*!
  Returns a JSON object that has been received as a variant map.  If no message is
  available, an empty JSON object is returned.
 */
QVariantMap QJsonEndpoint::readMessageMap()
{
    return readMessage().toVariantMap();
}

/*!
  Returns a JSON object that has been received.  If no message is
  available, an empty JSON object is returned.
 */
QJsonObject QJsonEndpoint::readMessage()
{
    QJsonObject obj;
    Q_D(QJsonEndpoint);
    if (d->mConnection) {
        obj = d->mConnection->processor()->readMessage(this);
        d->mMessageReady = false;
    }
    return obj;
}

/*!
    \fn void QJsonEndpoint::readyReadMessage()

    This signal is emitted once every time new data arrives on the device
    and a message is ready. \l{readMessage()} should be used to retrieve a message
    and \l{messageAvailable()} to check for next available messages.
    The client code may look like this:

    \code
    ...
    connect(endpoint, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    ...

    void Foo::processMessages()
    {
        while (endpoint->messageAvailable()) {
            QJsonObject obj = endpoint->readMessage();
            <process message>
        }
    }
    \endcode

    \b readyReadMessage() is not emitted recursively; if you reenter the event loop
    inside a slot connected to the \b readyReadMessage() signal, the signal will not
    be reemitted.

    \sa readMessage(), messageAvailable()
*/

#include "moc_qjsonendpoint.cpp"

QT_END_NAMESPACE_JSONSTREAM

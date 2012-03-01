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

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>

#include "jsonstream.h"
#include "jsonserver.h"
#include "jsonauthority.h"
#include "jsonserverclient.h"

#include "schemavalidator.h"

#include <QtCore>
#include <QtNetwork>

QT_BEGIN_NAMESPACE_JSONSTREAM


/*!
  \internal
*/
inline bool canValidate(JsonServer::ValidatorFlags flags, SchemaValidator *validator)
{
    return flags != JsonServer::NoValidation && validator && !validator->isEmpty();
}


/**************************************************************************************************/

/*!
    \class JsonServer
    \brief The JsonServer class is the server endpoint for JSON communications.

    JSON communications is a interprocess communication (IPC) method between clients and a server.
    It consists of the following components:

    \list
       \o JsonClient, a client-side class which sends and receives JSON messages
       \o JsonServer, the server class which sends and receives JSON messages
       \o JsonAuthority, an abstract authorization class which
          approves connections from clients and assigns client names.
    \endlist

    In most cases, authorization of the client is required.  This
    authorization step determines whether or not the client is
    approved for communication and associates an "identifier" string
    with the client for disambiguating received messages.
    Authorization can occur implicitly by looking at the properties of
    the connected socket (for example, authorize all connections from
    a particular TCP address) or explicitly by receiving a JSON
    message with appropriate authorization data.

    Once a client is authorized, a \l connectionAdded() signal will be
    fired.  When an authorized client disconnects, the \l
    connectionRemoved() signal fires.  No JsonServer signals are sent
    until a client is authorized.

    After authorization, each message received will result in a \l messageReceived() signal.
    The JsonServer sends messages to clients with the \l send() slot.

    In some cases, it is desireable to queue up messages for a client
    that has not yet connected.  Calling \c enableQueueing(identifier) will
    enable queueing of messages for that identifier.  Each client must
    be enabled separately; there is no general "queue for everyone" setting.
*/

/*!
  \property JsonServer::validatorFlags
  The current ValidatorFlags set on this server
*/

/*!
  Constructs a new JsonServer instance with \a parent.
*/
JsonServer::JsonServer(QObject *parent)
    : QObject(parent)
    , m_inboundValidator(0)
    , m_outboundValidator(0)
{
    initSchemaValidation(); // initialize validation if defined by environment
}

/*!
    Destroy this JsonServer.  All connections will be immediately dropped, with no more signals.
    We destroy all of the JsonServerClient objects and remove the QLocalServer objects.
    We do not destroy the JsonAuthority objects.
*/
JsonServer::~JsonServer()
{
    qDeleteAll(m_identifierToClient);
    foreach (QLocalServer *server, m_localServers.keys())
        delete server;
}

/*!
  Configure the JsonServer to listen on a new TCP socket and \a port using JsonAuthority file \a authority.
  If the \a authority is omitted or NULL, all connections will be automatically authorized and
  a unique identifier generated for each new connection.  Returns true if the socket could be opened.
 */
bool JsonServer::listen( int port, JsonAuthority *authority )
{
    Q_UNUSED(port);
    Q_UNUSED(authority);
    return false;
}

/*!
  Configure the JsonServer to listen on a new Unix local socket \a socketname using JsonAuthority file \a authority.
  If the \a authority is omitted or NULL, all connections will be automatically authorized and
  a unique identifier generated for each new connection. Returns true if the socket could be opened.

  Does \bold{not} take ownership of the \a authority object.
 */
bool JsonServer::listen(const QString &socketname, JsonAuthority *authority)
{
    QLocalServer::removeServer(socketname);
    QLocalServer *server = new QLocalServer(this);
    m_localServers.insert(server, authority);
    QObject::connect(server, SIGNAL(newConnection()), this, SLOT(handleLocalConnection()));
    if (!server->listen(socketname)) {
        qCritical() << Q_FUNC_INFO << "Unable to listen on socket:" << socketname;
        m_localServers.remove(server);
        return false;
    }
    return true;
}

/*!
    \internal
    Called with an incoming connection
*/
void JsonServer::handleLocalConnection()
{
    QLocalServer *server = qobject_cast<QLocalServer *>(sender());
    if (!server)
        return;

    if (QLocalSocket *socket = server->nextPendingConnection()) {
        socket->setReadBufferSize(64*1024);
        JsonAuthority *authority = m_localServers.value(server);
        JsonServerClient *client = new JsonServerClient(this);
        client->setAuthority(authority);
        client->setSocket(socket);
        connect(client, SIGNAL(authorized(const QString&)),
                this, SLOT(handleClientAuthorized(const QString&)));
        connect(client, SIGNAL(disconnected(const QString&)),
                this, SLOT(clientDisconnected(const QString&)));
        connect(client, SIGNAL(messageReceived(const QString&, const QJsonObject&)),
                this, SLOT(receiveMessage(const QString&, const QJsonObject&)));
        connect(client, SIGNAL(authorizationFailed()),
                this, SLOT(handleAuthorizationFailed()));
        client->start();
    }
}

/*!
  Received when a new client has been authorized by a \c JsonAuthority.
  The \a identifier must be unique.

  Be sure to call this function if you override it in a subclass.  This function
  will emit \c connectionAdded with the \a identifier of the client.
  t
 */
void JsonServer::handleClientAuthorized(const QString &identifier)
{
    JsonServerClient *client = qobject_cast<JsonServerClient *>(sender());
    bool exists = m_identifierToClient.contains(identifier);

    if (exists && !m_multipleConnections.contains(identifier)) {
        qWarning() << "Error: Multiple disallowed connections for" << identifier;
        client->stop();
    }
    else {
        m_identifierToClient.insert(identifier, client);
        QList<QJsonObject> messageQueue = m_messageQueues.take(identifier);
        foreach (const QJsonObject& message, messageQueue)
            client->send(message);
        if (!exists)
            emit connectionAdded(identifier);
    }
}

/*!
  Called when an identified client has disconnected.
  This function will emit \c connectionRemoved with the \a identifier of
  the client.
 */

void JsonServer::clientDisconnected(const QString &identifier) {
    JsonServerClient *client = qobject_cast<JsonServerClient *>(sender());
    if (!client)
        return;

    // Only emit the connectionRemoved signal if this was a valid connection
    if (!m_identifierToClient.remove(identifier, client))
        qWarning() << "Error: Mismatched client for" << identifier;
    else
        emit connectionRemoved(identifier);

    client->deleteLater();
}

/*!
    Called when authorization fails for a client.
*/
void JsonServer::handleAuthorizationFailed()
{
    emit authorizationFailed();
}

/*!
    Process received \a message originating from application identified by \a identifier.

    The messageReceived signal is emitted when this method is called.
*/
void JsonServer::receiveMessage(const QString &identifier, const QJsonObject &message)
{
    // do JSON schema validation if required
    if (canValidate(validatorFlags(), m_inboundValidator)) {
        if (!m_inboundValidator->validateSchema(message))
        {
            if (validatorFlags().testFlag(WarnIfInvalid)) {
                emit inboundMessageValidationFailed(message, m_inboundValidator->getLastError());
            }
            if (validatorFlags().testFlag(DropIfInvalid)) {
                return;
            }
        }
    }

    emit messageReceived(identifier, message);
}

/*!
    Returns whether there is a connected client with \a identifier.
*/
bool JsonServer::hasConnection(const QString &identifier) const
{
    return m_identifierToClient.contains(identifier);
}

/*!
  Returns a list of identifiers for all current connections.
*/

QStringList JsonServer::connections() const
{
    return m_identifierToClient.uniqueKeys();
}

/*!
    Enables queuing of messages to client identified by \a identifier.

    The queue will be flushed to the client when it connects to the server,
    and all subsequent messages will be sent directly to the client.
*/
void JsonServer::enableQueuing(const QString &identifier)
{
    if (identifier.isEmpty())
        return;

    if (!m_messageQueues.contains(identifier))
        m_messageQueues.insert(identifier, QList<QJsonObject>());
}

/*!
    Disables queuing of messages for client identified by \a identifier.
*/
void JsonServer::disableQueuing(const QString &identifier)
{
    if (identifier.isEmpty())
        return;

    m_messageQueues.remove(identifier);
}

/*!
    Returns whether queuing of messages is enabled for client identified by \a identifier.
*/
bool JsonServer::isQueuingEnabled(const QString &identifier)
{
    return m_messageQueues.contains(identifier);
}

/*!
    Clears the message queue for client \a identifier.

    Does not disable or enable message queuing for the client.
*/
void JsonServer::clearQueue(const QString &identifier)
{
    if (identifier.isEmpty())
        return;

    if (m_messageQueues.contains(identifier))
        m_messageQueues.insert(identifier, QList<QJsonObject>());
}

/*!
  Enable multiple clients that match \a identifier.
  Any outgoing messages will be sent to each connection with this
  \a identifier.
*/

void JsonServer::enableMultipleConnections(const QString& identifier)
{
    m_multipleConnections.insert(identifier);
}

/*!
  Disable multiple clients that match \a identifier.
*/

void JsonServer::disableMultipleConnections(const QString& identifier)
{
    m_multipleConnections.remove(identifier);
}



/*!
    Sends an arbitrary \a message to the client application identified by \a identifier and returns true when
    the client was found and the message could be sent. The return value does \bold{not} indicate that
    the client actually received the message and reacted to it, only that the JsonServer had a connection
    open to that client, and was able to dispatch the message.

*/
bool JsonServer::send(const QString &identifier, const QJsonObject &message)
{
    // do JSON schema validation if required
    if (canValidate(validatorFlags(), m_outboundValidator)) {
        if (!m_outboundValidator->validateSchema(message))
        {
            if (validatorFlags().testFlag(WarnIfInvalid)) {
                emit outboundMessageValidationFailed(message, m_outboundValidator->getLastError());
            }
            if (validatorFlags().testFlag(DropIfInvalid)) {
                return false;
            }
        }
    }

    if (isQueuingEnabled(identifier)) {
        QList<QJsonObject> queue = m_messageQueues.value(identifier);
        queue << message;
        m_messageQueues.insert(identifier, queue);
        return true;
    }

    QList<JsonServerClient*> clients = m_identifierToClient.values(identifier);
    foreach (JsonServerClient *client, clients) {
        Q_ASSERT(client);
        client->send(message);
    }
    return (clients.size() > 0);
}

/*!
  Broadcast \a message to all connected clients.
*/

void JsonServer::broadcast(const QJsonObject &message)
{
    // do JSON schema validation if required
    if (canValidate(validatorFlags(), m_outboundValidator)) {
        if (!m_outboundValidator->validateSchema(message))
        {
            if (validatorFlags().testFlag(WarnIfInvalid)) {
                emit outboundMessageValidationFailed(message, m_outboundValidator->getLastError());
            }
            if (validatorFlags().testFlag(DropIfInvalid)) {
                return;
            }
        }
    }

    // ### No JsonServerClient should be repeated in this list
    QList<JsonServerClient*> clients = m_identifierToClient.values();
    foreach (JsonServerClient *client, clients) {
        Q_ASSERT(client);
        client->send(message);
    }
}

/*!
    Removes all connections created by the created by the application identified by \a identifier.
    If no connections are found, this method does nothing.
    Note that calling this function does not change the JsonAuthority permissions, so it is
    possible that a new client will connect using the same authorization.

    Calling this function will result in the \l connectionRemoved() signal being raised.
*/
void JsonServer::removeConnection(const QString &identifier)
{
    QList<JsonServerClient*> clients = m_identifierToClient.values(identifier);
    while (!clients.isEmpty()) {
        JsonServerClient *client = clients.takeFirst();
        Q_ASSERT(client);
        client->stop();
    }
}

/*!
     \enum JsonServer::ValidatorFlag
     This enum determines what the validators do with packets.  The value is
     specified by combining values from the following list using the bitwise
     OR operator:

     \value DropIfInvalid
         Validate and drop invalid messages.
     \value WarnIfInvalid
         Validate and warn about invalid messages.
     \value ApplyDefaultValues
         If a value is missing then use a default attribute's value fron JSON schema.

     \omitvalue NoValidation
*/

/*!
  \fn ValidatorFlags JsonServer::validatorFlags() const
  Return the current ValidatorFlags
*/

/*!
  Sets validatorFlags property to \a flags.
*/
void JsonServer::setValidatorFlags(ValidatorFlags flags)
{
    m_validatorFlags = flags;
}

/*!
  Returns an inbound JSON schema validator object.
*/
SchemaValidator *JsonServer::inboundValidator()
{
    if (!m_inboundValidator) {
        m_inboundValidator = new SchemaValidator(this);
    }
    return m_inboundValidator;
}

/*!
  Returns an outbound JSON schema validator object.
*/
SchemaValidator *JsonServer::outboundValidator()
{
    if (!m_outboundValidator) {
        m_outboundValidator = new SchemaValidator(this);
    }
    return m_outboundValidator;
}

/*!
  \internal
  Initialize validation if defined by environment
*/
void JsonServer::initSchemaValidation()
{
    QString szInboundPath = QString::fromLocal8Bit(qgetenv("JSONSERVER_SCHEMA_INBOUND_PATH"));
    QString szOutboundPath = QString::fromLocal8Bit(qgetenv("JSONSERVER_SCHEMA_OUTBOUND_PATH"));
    QString strSchemaControl = QString::fromLocal8Bit(qgetenv("JSONSERVER_SCHEMA_CONTROL")); // "warn","drop" or "warn":"drop"

    if (!strSchemaControl.isEmpty())
    {
        ValidatorFlags flags(NoValidation);
        foreach (QString str, strSchemaControl.toLower().split(QRegExp(QStringLiteral("[,:]")),
                                                               QString::SkipEmptyParts)) {
            if (str == QLatin1String("warn"))
                flags |= WarnIfInvalid;
            else if (str == QLatin1String("drop"))
                flags |= DropIfInvalid;
        }
        setValidatorFlags(flags);
    }

    if (!szInboundPath.isEmpty()) {
        QFileInfo fi(szInboundPath);
        if (fi.exists() && fi.isDir()) {
            inboundValidator()->loadFromFolder(szInboundPath);
        }
    }

    if (!szOutboundPath.isEmpty()) {
        QFileInfo fi(szOutboundPath);
        if (fi.exists() && fi.isDir()) {
            outboundValidator()->loadFromFolder(szOutboundPath);
        }
    }
}

/*!
    \fn void JsonServer::connectionAdded(const QString &identifier)
    This signal is emitted when a connection identified by the \a identifier has connected and
    been authorized.
*/

/*!
    \fn void JsonServer::connectionRemoved(const QString &identifier)
    This signal is emitted when a connection identified by \a identifier disconnects.

    Disconnection can happen if the connection is lost, or when the client process is stopped.
*/

/*!
    \fn void JsonServer::messageReceived(const QString &identifier, const QJsonObject &message)
    This signal is emitted when a message is received from a connected client identified by the application identifier \a identifier.
    \a message contains the JSON message received.
*/

/*!
    \fn void JsonServer::authorizationFailed()
    This signal is emitted when a client fails to authorize.
*/

/*!
    \fn void JsonServer::inboundMessageValidationFailed(const QJsonObject &message, const QtAddOn::JsonStream::SchemaError &error)
    This signal is emitted when an inbound \a message message fails a JSON schema validation.  The validation
    error is reported in \a error.
*/

/*!
    \fn void JsonServer::outboundMessageValidationFailed(const QJsonObject &message, const QtAddOn::JsonStream::SchemaError &error)
    This signal is emitted when an outbound \a message message fails a JSON schema validation.  The validation
    error is reported in \a error.
*/

#include "moc_jsonserver.cpp"

QT_END_NAMESPACE_JSONSTREAM

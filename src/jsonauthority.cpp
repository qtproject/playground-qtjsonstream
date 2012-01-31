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

#include "jsonauthority.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/**************************************************************************************************/
/*!
  \class JsonAuthority
  \brief The JsonAuthority class is an abstract class used to authorize new Json client connections.

  The JsonAuthority class authorizes Json client connections.

  Note: Do I need an asynchronous way so that a JsonAuthority can drop a client connection
  after a timeout?
 */

/*!
  \enum JsonAuthority::AuthorizationState

  This enum is returned from clientConnected() and messageReceived() calls to change the
  authorization state of the connection.

  \value StateAuthorized    The connection is authorized.  All future messages are send to the \c JsonServer clients.
  \value StateNotAuthorized The connection is not authorized.  The \c JsonServer must shut down the connection.
  \value StatePending       The connection has not yet been authorized.  New messages will be send to the \c JsonAuthority
                            until the connection is authorized by the JsonAuthority.
*/

/*!
  Constructs a \c JsonAuthority object with the given \a parent.
 */

JsonAuthority::JsonAuthority(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destructs the JsonAuthority object.
*/
JsonAuthority::~JsonAuthority()
{

}

/*!
  Will only be called if a non-authorized client disconnects.
  The \a stream object should not be used to send messages.
 */
void JsonAuthority::clientDisconnected( JsonStream *stream )
{
    Q_UNUSED(stream);
}

/*!
  Returns an AuthorizationRecord based on initial client connection.  If the authorization state is \c Authorized,
  the record's identifier must be initialized to a unique value.  The \a stream object may be used to send
  messages; for example, the \c JsonAuthority object may participate in a challenge-response exchange before
  authorizing the connection.
 */

AuthorizationRecord JsonAuthority::clientConnected( JsonStream *stream )
{
    Q_UNUSED(stream);
    return AuthorizationRecord( StateNotAuthorized );
}

/*!
  Returns an AuthorizationState based on a received \a message.  If the authorization state is \c Authorized,
  the record's identifier must be initialized to a unique value.  The \a stream may be used to send additional
  messages to the client.
 */

AuthorizationRecord JsonAuthority::messageReceived(JsonStream *stream, const QJsonObject& message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    return AuthorizationRecord( StateNotAuthorized );
}

#include "moc_jsonauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

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

#include "qjsonauthority.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/**************************************************************************************************/
/*!
  \class QJsonAuthority
  \inmodule QtJsonStream
  \brief The QJsonAuthority class is an abstract class used to authorize new QJson client connections.

  The QJsonAuthority class authorizes QJson client connections.

  Note: Do I need an asynchronous way so that a QJsonAuthority can drop a client connection
  after a timeout?
 */

/*!
  \enum QJsonAuthority::AuthorizationState

  This enum is returned from clientConnected() and messageReceived() calls to change the
  authorization state of the connection.

  \value StateAuthorized    The connection is authorized.  All future messages are send to the \c QJsonServer clients.
  \value StateNotAuthorized The connection is not authorized.  The \c QJsonServer must shut down the connection.
  \value StatePending       The connection has not yet been authorized.  New messages will be send to the \c QJsonAuthority
                            until the connection is authorized by the QJsonAuthority.
*/

/*!
  Constructs a \c QJsonAuthority object with the given \a parent.
 */

QJsonAuthority::QJsonAuthority(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destructs the QJsonAuthority object.
*/
QJsonAuthority::~QJsonAuthority()
{

}

/*!
  Will only be called if a non-authorized client disconnects.
  The \a stream object should not be used to send messages.
 */
void QJsonAuthority::clientDisconnected( QJsonStream *stream )
{
    Q_UNUSED(stream);
}

/*!
  Returns an AuthorizationRecord based on initial client connection.  If the authorization state is \c Authorized,
  the record's identifier must be initialized to a unique value.  The \a stream object may be used to send
  messages; for example, the \c QJsonAuthority object may participate in a challenge-response exchange before
  authorizing the connection.
 */

AuthorizationRecord QJsonAuthority::clientConnected( QJsonStream *stream )
{
    Q_UNUSED(stream);
    return AuthorizationRecord( StateNotAuthorized );
}

/*!
  Returns an AuthorizationState based on a received \a message.  If the authorization state is \c Authorized,
  the record's identifier must be initialized to a unique value.  The \a stream may be used to send additional
  messages to the client.
 */

AuthorizationRecord QJsonAuthority::messageReceived(QJsonStream *stream, const QJsonObject& message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    return AuthorizationRecord( StateNotAuthorized );
}

#include "moc_qjsonauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

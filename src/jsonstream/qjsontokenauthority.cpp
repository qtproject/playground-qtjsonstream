/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QUuid>
#include <QDebug>
#include "qjsontokenauthority.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class QJsonTokenAuthority
  \inmodule QtJsonStream
  \brief The QJsonTokenAuthority class implements a token-based authentication scheme for QJsonServer.

  The QJsonTokenAuthority class authorizes QJson client connections based on tokens received from the first
  message sent by the client to the server. The expectation is that the first message sent by the client
  to the authority will be of the form:

  \code
    { "token": AUTHORIZATION_TOKEN }
  \endcode

 */

/*!
  Construct a \c QJsonTokenAuthority with the given \a parent.
 */

QJsonTokenAuthority::QJsonTokenAuthority(QObject *parent)
    : QJsonAuthority(parent)
{
}

/*!
    Add a \a token, \a identifier pair to the valid hash table.
    Return false if the token was already in the table.
*/
bool QJsonTokenAuthority::authorize(const QByteArray &token, const QString &identifier)
{
    if (identifier.isEmpty() || token.isEmpty())
        return false;

    if (m_identifierForToken.contains(token))
        return false;

    m_identifierForToken.insert(token, identifier);
    return true;
}

/*!
    Remove a \a token from the valid hash table.  Calling this function does
    \b{not} disconnect any existing clients.  Return true if the token was in the table, false if it was not.
*/
bool QJsonTokenAuthority::deauthorize(const QByteArray &token)
{
    return m_identifierForToken.remove(token);
}

/*!
    Wait for the first received message on \a stream.
 */
AuthorizationRecord QJsonTokenAuthority::clientConnected(QJsonStream *stream)
{
    Q_UNUSED(stream);
    return AuthorizationRecord( QJsonAuthority::StatePending );
}

/*!
    The first \a message received from \a stream must contain a "token" field which contains an
    authorized identifier. If it does not, the connection is not authorized.

    \sa authorize()
 */
AuthorizationRecord QJsonTokenAuthority::messageReceived(QJsonStream *stream,
                                                        const QJsonObject &message)
{
    Q_UNUSED(stream);

    const QByteArray token = message.value(QLatin1String("token")).toString().toLatin1();
    QString identifier = m_identifierForToken.value(token);

    if (!identifier.isEmpty())
        return AuthorizationRecord( QJsonAuthority::StateAuthorized, identifier );
    return AuthorizationRecord( QJsonAuthority::StateNotAuthorized );
}

#include "moc_qjsontokenauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

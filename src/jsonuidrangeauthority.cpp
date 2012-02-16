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

#include "jsonuidrangeauthority.h"
#include "jsonstream.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pwd.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class JsonUIDRangeAuthority
  \brief The JsonUIDRangeAuthority class authorizes clients that fall within a range of UID values.

  The JsonUIDRangeAuthority class authorizes Json client connections based on the user ID of the
  client.  This user ID is read directly from the connected socket.  The authorized range
  is set with a minimum and maximum value.  The range is inclusive; that is, to be authorized the client
  must satisfy "minimum <= uid <= maximum".  Initially the range is not set; you must set both
  minimum and maximum, or all clients will be refused.

  The name returned by the JsonUIDRangeAuthority class is the name entry in the /etc/passwd
  database.  If a name entry does not exist, the JsonUIDRangeAuthority class sets the name equal
  to the string form of the uid.

  Please note that actual Posix uid_t values are unsigned integers.  In theory this means that
  we will get into trouble if someone has a UID value greater than about 2000000000.
 */

/*!
  \property JsonUIDRangeAuthority::minimum
  The minimum valid UID.  This must be set to at least 0, or all connections will be refused.
*/

/*!
  \property JsonUIDRangeAuthority::maximum
  The maximum valid UID.  This must be set to at least 0, or all connections will be refused.
*/

/*!
  Construct a JsonUIDRangeAuthority with optional \a parent.
 */

JsonUIDRangeAuthority::JsonUIDRangeAuthority(QObject *parent)
    : JsonAuthority(parent)
    , m_minimum(-1)
    , m_maximum(-1)
{
}

/*!
  Return the minimum value
 */

int JsonUIDRangeAuthority::minimum() const
{
    return m_minimum;
}

/*!
  Set the minimum value to \a minimum.
 */

void JsonUIDRangeAuthority::setMinimum(int minimum)
{
    if (m_minimum != minimum) {
        m_minimum = minimum;
        emit minimumChanged();
    }
}

/*!
  Return the maximum value
 */

int JsonUIDRangeAuthority::maximum() const
{
    return m_maximum;
}

/*!
  Set the maximum value to \a maximum.
 */

void JsonUIDRangeAuthority::setMaximum(int maximum)
{
    if (m_maximum != maximum) {
        m_maximum = maximum;
        emit maximumChanged();
    }
}

/*!
    Authorizes the connection from \a stream if and only if the uid
    fall within the value range.
    Returns an AuthorizationRecord.
*/

AuthorizationRecord JsonUIDRangeAuthority::clientConnected(JsonStream *stream)
{
    AuthorizationRecord authRecord;
    authRecord.state = JsonAuthority::StateNotAuthorized;

    if (!stream)
        return authRecord;

    QLocalSocket *socket = qobject_cast<QLocalSocket*>(stream->device());

    if (!socket)
        return authRecord;

    if (socket->socketDescriptor() == (quintptr)-1) {
        qWarning() << Q_FUNC_INFO << "no socket descriptor available for connection" << socket;
        return authRecord;
    }

    uid_t euid;
#if defined(Q_OS_MAC)
    gid_t egid;
    if (::getpeereid(socket->socketDescriptor(), &euid, &egid) != 0) {
        qWarning() << "getpeereid failed with errcode" << errno << socket->socketDescriptor();
        return authRecord;
    }
#else
    // Check the UIDRange table and return Authorized if appropriate.
    struct ucred cr;
    socklen_t len = sizeof(struct ucred);
    if (::getsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_PEERCRED, &cr, &len) != 0) {
        qWarning() << "getsockopt failed with errcode" << errno << socket->socketDescriptor();
        return authRecord;
    }
    euid = cr.uid;
#endif

    if (m_minimum >= 0 && m_maximum >= 0 && euid >= (uid_t) m_minimum && euid <= (uid_t) m_maximum) {
        struct passwd *passwd = getpwuid(euid);
        if (passwd)
            authRecord.identifier = QString::fromLatin1(passwd->pw_name);
        else
            authRecord.identifier = QString::number(euid);
        authRecord.state = StateAuthorized;
    }

    return authRecord;
}

/*!
  Ignore any \a message received from \a stream.
 */
AuthorizationRecord JsonUIDRangeAuthority::messageReceived(JsonStream *stream,
                                                           const QJsonObject &message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    AuthorizationRecord authRecord;
    authRecord.state = JsonAuthority::StateNotAuthorized;
    return authRecord;
}

/*!
  \fn void JsonUIDRangeAuthority::minimumChanged()
  Signal emitted when the minimum value is changed.
 */

/*!
  \fn void JsonUIDRangeAuthority::maximumChanged()
  Signal emitted when the maximum value is changed.
 */

#include "moc_jsonuidrangeauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

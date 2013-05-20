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

#include "qjsonuidrangeauthority.h"
#include "qjsonstream.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pwd.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class QJsonUIDRangeAuthority
  \inmodule QtJsonStream
  \brief The QJsonUIDRangeAuthority class authorizes clients that fall within a range of UID values.

  The QJsonUIDRangeAuthority class authorizes QJson client connections based on the user ID of the
  client.  This user ID is read directly from the connected socket.  The authorized range
  is set with a minimum and maximum value.  The range is inclusive; that is, to be authorized the client
  must satisfy "minimum <= uid <= maximum".  Initially the range is not set; you must set both
  minimum and maximum, or all clients will be refused.

  The name returned by the QJsonUIDRangeAuthority class is the name entry in the /etc/passwd
  database.  If a name entry does not exist, the QJsonUIDRangeAuthority class sets the name equal
  to the string form of the uid.

  Please note that actual Posix uid_t values are unsigned integers.  In theory this means that
  we will get into trouble if someone has a UID value greater than about 2000000000.
 */

/*!
  \property QJsonUIDRangeAuthority::minimum
  The minimum valid UID.  This must be set to at least 0, or all connections will be refused.
*/

/*!
  \property QJsonUIDRangeAuthority::maximum
  The maximum valid UID.  This must be set to at least 0, or all connections will be refused.
*/

/*!
  Construct a QJsonUIDRangeAuthority with optional \a parent.
 */

QJsonUIDRangeAuthority::QJsonUIDRangeAuthority(QObject *parent)
    : QJsonAuthority(parent)
    , m_minimum(-1)
    , m_maximum(-1)
{
}

/*!
  Return the minimum value
 */

int QJsonUIDRangeAuthority::minimum() const
{
    return m_minimum;
}

/*!
  Set the minimum value to \a minimum.
 */

void QJsonUIDRangeAuthority::setMinimum(int minimum)
{
    if (m_minimum != minimum) {
        m_minimum = minimum;
        emit minimumChanged();
    }
}

/*!
  Return the maximum value
 */

int QJsonUIDRangeAuthority::maximum() const
{
    return m_maximum;
}

/*!
  Set the maximum value to \a maximum.
 */

void QJsonUIDRangeAuthority::setMaximum(int maximum)
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

AuthorizationRecord QJsonUIDRangeAuthority::clientConnected(QJsonStream *stream)
{
    AuthorizationRecord authRecord;
    authRecord.state = QJsonAuthority::StateNotAuthorized;

    if (!stream)
        return authRecord;

    QLocalSocket *socket = qobject_cast<QLocalSocket*>(stream->device());

    if (!socket)
        return authRecord;

    if (socket->socketDescriptor() == (qintptr)-1) {
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
AuthorizationRecord QJsonUIDRangeAuthority::messageReceived(QJsonStream *stream,
                                                           const QJsonObject &message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    AuthorizationRecord authRecord;
    authRecord.state = QJsonAuthority::StateNotAuthorized;
    return authRecord;
}

/*!
  \fn void QJsonUIDRangeAuthority::minimumChanged()
  Signal emitted when the minimum value is changed.
 */

/*!
  \fn void QJsonUIDRangeAuthority::maximumChanged()
  Signal emitted when the maximum value is changed.
 */

#include "moc_qjsonuidrangeauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

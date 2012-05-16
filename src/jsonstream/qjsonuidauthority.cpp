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

#include "qjsonuidauthority.h"
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
  \class QJsonUIDAuthority
  \inmodule QtJsonStream
  \brief The QJsonUIDAuthority class implements a UID-based authentication scheme for QJsonServer.

  The QJsonUIDAuthority class authorizes QJson client connections based on the user ID of the
  client.  This user ID is read directly from the connected socket.

  The name returned by the QJsonUIDAuthority class is the name entry in the /etc/passwd
  database.  If a name entry does not exist, the QJsonUIDAuthority class sets the name equal
  to the string form of the uid.
 */

/*!
  Construct a QJsonUIDAuthority with optional \a parent.
 */

QJsonUIDAuthority::QJsonUIDAuthority(QObject *parent)
    : QJsonAuthority(parent)
{
}

/*!
    Adds a user id \a uid to the list of authorized processes.
    Returns true if the uid was non-zero and could be added.

    Before a process can successfully connect to the server with
    its user id must be added with this method if authorization is required.
*/
bool QJsonUIDAuthority::authorize(qint64 uid)
{
    if (m_nameForUid.contains(uid))
        return false;

    errno = 0;
    struct passwd *passwd = getpwuid(uid);
    QString name;
    if (passwd) {
        name = QString::fromLatin1(passwd->pw_name);
    }
    else {
        if (errno) {
            qWarning() << "getpwuid failed with errcode" << errno << uid;
            return false;
        }
        name = QString::number(uid);
    }
    m_nameForUid.insert(uid, name);
    return true;
}


/*!
    Adds a user \a name to the list of authorized processes.
    Returns true if the name was found in the /etc/passwd file
    and could be added.
*/
bool QJsonUIDAuthority::authorize(const QString& name)
{
    errno = 0;
    struct passwd *passwd = getpwnam(name.toLatin1().data());
    if (!passwd) {
        if (errno)
            qWarning() << "getpwnam failed with errno=" << errno << name;
        else
            qWarning() << "Unable to authorize" << name;
        return false;
    }

    if (m_nameForUid.contains(passwd->pw_uid))
        return false;

    m_nameForUid.insert(passwd->pw_uid, name);
    return true;
}

/*!
    Removes a user id \a uid from the list of authorized users.
    Returns true if a process with a matching UID was found.
*/
bool QJsonUIDAuthority::deauthorize(qint64 uid)
{
    return m_nameForUid.remove(uid) != 0;
}

/*!
    Removes a user \a name from the list of authorized users.
    Returns true if a process with a matching name was found.
*/
bool QJsonUIDAuthority::deauthorize(const QString& name)
{
    QList<qint64> keylist = m_nameForUid.keys(name);
    if (!keylist.length())
        return false;

    foreach (qint64 key, keylist)
        m_nameForUid.remove(key);
    return true;
}

/*!
    Return true if the \a uid is authorized.
 */

bool QJsonUIDAuthority::isAuthorized(qint64 uid) const
{
    return m_nameForUid.contains(uid);
}

/*!
    Returns the identify of the client with UID \a uid.
    If no such name is authorized, returns an empty string.
*/

QString QJsonUIDAuthority::name(qint64 uid) const
{
    return m_nameForUid.value(uid);
}

/*!
    Authorizes the connection from \a stream if and only if it matches a valid UID.
    Returns an AuthorizationRecord.
*/

AuthorizationRecord QJsonUIDAuthority::clientConnected(QJsonStream *stream)
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
    // Check the UID table and return Authorized if appropriate.
    struct ucred cr;
    socklen_t len = sizeof(struct ucred);
    if (::getsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_PEERCRED, &cr, &len) != 0) {
        qWarning() << "getsockopt failed with errcode" << errno << socket->socketDescriptor();
        return authRecord;
    }
    euid = cr.uid;
#endif

    if (m_nameForUid.contains(euid)) {
        authRecord.identifier = m_nameForUid.value(euid);
        authRecord.state      = StateAuthorized;
    }

    return authRecord;
}

/*!
  Ignore any \a message received from \a stream.
 */
AuthorizationRecord QJsonUIDAuthority::messageReceived(QJsonStream *stream,
                                                      const QJsonObject &message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    AuthorizationRecord authRecord;
    authRecord.state = QJsonAuthority::StateNotAuthorized;
    return authRecord;
}

#include "moc_qjsonuidauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

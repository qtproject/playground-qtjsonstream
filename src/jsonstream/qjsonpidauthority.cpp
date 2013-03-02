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

#include "qjsonpidauthority.h"
#include "qjsonstream.h"

#include <errno.h>
#include <sys/socket.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class QJsonPIDAuthority
  \inmodule QtJsonStream
  \brief The QJsonPIDAuthority class implements a PID-based authentication scheme for QJsonServer.

  The QJsonPIDAuthority class authorizes QJson client connections based on the process ID of the
  client.  This process ID is read directly from the connected socket.

  This class functions only on Linux machines.  The BSD kernel used in Macintosh systems does
  not currently support sending PID information across a socket in a secure fashion.
 */

/*!
  Construct a QJsonPIDAuthority with optional \a parent.
 */

QJsonPIDAuthority::QJsonPIDAuthority(QObject *parent)
    : QJsonAuthority(parent)
{
}

/*!
    Adds a process id \a pid to the list of authorized processes
    with the string \a identifier.
    Returns true if the PID was non-zero and could be added.

    Before a process can successfully connect to the server with
    its process id must be added with this method if authorization is required.
*/
bool QJsonPIDAuthority::authorize(qint64 pid, const QString &identifier)
{
    if (pid == 0) {
        qWarning() << "PID 0 is invalid";
        return false;
    }
    if (m_identifierForPid.contains(pid))
        return false;

    m_identifierForPid.insert(pid, identifier);
    return true;
}

/*!
    Removes a process id \a pid from the list of authorized processes.
    Returns true if a process with a matching PID was found.
*/
bool QJsonPIDAuthority::deauthorize(qint64 pid)
{
    return m_identifierForPid.remove(pid) != 0;
}

/*!
    Return true if the \a pid is authorized.
 */

bool QJsonPIDAuthority::isAuthorized(qint64 pid) const
{
    return m_identifierForPid.contains(pid);
}

/*!
    Returns the identify of the client with PID \a pid.
    If no such identifier is authorized, returns an empty string.
*/

QString QJsonPIDAuthority::identifier(qint64 pid) const
{
    return m_identifierForPid.value(pid);
}

/*!
    Authorizes the connection from \a stream if and only if it matches a valid PID.
    Returns an AuthorizationRecord.
*/

AuthorizationRecord QJsonPIDAuthority::clientConnected(QJsonStream *stream)
{
    AuthorizationRecord authRecord;
    authRecord.state = QJsonAuthority::StateNotAuthorized;

    if (!stream)
        return authRecord;

    QLocalSocket *socket = qobject_cast<QLocalSocket*>(stream->device());

    if (!socket)
        return authRecord;

    if (socket->socketDescriptor() == -1) {
        qWarning() << Q_FUNC_INFO << "no socket descriptor available for connection" << socket;
        return authRecord;
    }

    // Check the PID table and return Authorized if appropriate.
    struct ucred cr;
    socklen_t len = sizeof(struct ucred);
    int r = ::getsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_PEERCRED, &cr, &len);
    if (r == 0) {
        if (m_identifierForPid.contains(cr.pid)) {
            authRecord.identifier = m_identifierForPid.value(cr.pid);
            authRecord.state = StateAuthorized;
        }
    } else {
        qWarning() << "getsockopt failed with errcode" << errno << socket->socketDescriptor();
        authRecord.state = StateNotAuthorized;
    }

    return authRecord;
}

/*!
  Ignore any \a message received from \a stream.
 */
AuthorizationRecord QJsonPIDAuthority::messageReceived(QJsonStream *stream,
                                                      const QJsonObject &message)
{
    Q_UNUSED(stream);
    Q_UNUSED(message);
    AuthorizationRecord authRecord;
    authRecord.state = QJsonAuthority::StateNotAuthorized;
    return authRecord;
}

#include "moc_qjsonpidauthority.cpp"

QT_END_NAMESPACE_JSONSTREAM

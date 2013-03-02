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

#ifndef JSONPIDAUTHORITYPROVIDER_H
#define JSONPIDAUTHORITYPROVIDER_H

#include <QHash>
#include <QLocalSocket>

#include "qjsonauthority.h"
#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT QJsonPIDAuthority : public QJsonAuthority
{
    Q_OBJECT
public:
    QJsonPIDAuthority(QObject *parent = 0);

    bool authorize(qint64 pid, const QString &applicationIdentifier);
    bool deauthorize(qint64 pid);
    bool isAuthorized(qint64 pid) const;
    QString identifier(qint64 pid) const;

    virtual AuthorizationRecord clientConnected(QJsonStream *stream);
    virtual AuthorizationRecord messageReceived(QJsonStream *stream, const QJsonObject &message);

private:
    QHash<qint64, QString> m_identifierForPid;
};

QT_END_NAMESPACE_JSONSTREAM

QT_JSONSTREAM_DECLARE_METATYPE_PTR(QJsonPIDAuthority)

#endif // JSONPIDAUTHORITYPROVIDER_H

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

#ifndef JSONUIDAUTHORITYPROVIDER_H
#define JSONUIDAUTHORITYPROVIDER_H

#include <QHash>
#include <QLocalSocket>

#include "jsonauthority.h"
#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT JsonUIDAuthority : public JsonAuthority
{
    Q_OBJECT
public:
    JsonUIDAuthority(QObject *parent = 0);

    bool authorize(qint64 uid);
    bool authorize(const QString& name);

    bool deauthorize(qint64 uid);
    bool deauthorize(const QString& name);

    bool isAuthorized(qint64 uid) const;
    QString name(qint64 uid) const;

    virtual AuthorizationRecord clientConnected(JsonStream *stream);
    virtual AuthorizationRecord messageReceived(JsonStream *stream, const QJsonObject &message);

private:
    QHash<qint64, QString> m_nameForUid;
};

QT_END_NAMESPACE_JSONSTREAM

QT_JSONSTREAM_DECLARE_METATYPE_PTR(JsonUIDAuthority)

#endif // JSONUIDAUTHORITYPROVIDER_H

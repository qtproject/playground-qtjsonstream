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

#ifndef JSONUIDAUTHORITYPROVIDER_H
#define JSONUIDAUTHORITYPROVIDER_H

#include <QHash>
#include <QLocalSocket>

#include "qjsonauthority.h"
#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT QJsonUIDAuthority : public QJsonAuthority
{
    Q_OBJECT
public:
    QJsonUIDAuthority(QObject *parent = 0);

    bool authorize(qint64 uid);
    bool authorize(const QString& name);

    bool deauthorize(qint64 uid);
    bool deauthorize(const QString& name);

    bool isAuthorized(qint64 uid) const;
    QString name(qint64 uid) const;

    virtual AuthorizationRecord clientConnected(QJsonStream *stream);
    virtual AuthorizationRecord messageReceived(QJsonStream *stream, const QJsonObject &message);

private:
    QHash<qint64, QString> m_nameForUid;
};

QT_END_NAMESPACE_JSONSTREAM

QT_JSONSTREAM_DECLARE_METATYPE_PTR(QJsonUIDAuthority)

#endif // JSONUIDAUTHORITYPROVIDER_H

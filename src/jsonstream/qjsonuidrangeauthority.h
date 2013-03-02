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

#ifndef JSON_UID_RANGE_AUTHORITY_H
#define JSON_UID_RANGE_AUTHORITY_H

#include <QHash>
#include <QLocalSocket>

#include "qjsonauthority.h"
#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT QJsonUIDRangeAuthority : public QJsonAuthority
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)

public:
    QJsonUIDRangeAuthority(QObject *parent = 0);

    int  minimum() const;
    void setMinimum(int);

    int  maximum() const;
    void setMaximum(int);

    virtual AuthorizationRecord clientConnected(QJsonStream *stream);
    virtual AuthorizationRecord messageReceived(QJsonStream *stream, const QJsonObject &message);

signals:
    void minimumChanged();
    void maximumChanged();

private:
    int m_minimum;
    int m_maximum;
};

QT_END_NAMESPACE_JSONSTREAM

QT_JSONSTREAM_DECLARE_METATYPE_PTR(QJsonUIDRangeAuthority)

#endif // JSON_UID_RANGE_AUTHORITY_H

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

#ifndef _JSON_ENDPOINT_H
#define _JSON_ENDPOINT_H

#include <QObject>
#include <QJsonObject>
#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonConnection;

class QJsonEndpointPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonEndpoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QJsonConnection* connection READ connection WRITE setConnection NOTIFY connectionChanged)
public:
    QJsonEndpoint(const QString & = QString::null, QJsonConnection * = 0);
    virtual ~QJsonEndpoint();

    QString name() const;
    void    setName( const QString & name );

    QJsonConnection *connection() const;
    void setConnection(QJsonConnection *);

    Q_INVOKABLE bool send(const QVariantMap& message);
    Q_INVOKABLE bool send(const QJsonObject& message);

    Q_INVOKABLE bool messageAvailable();

    Q_INVOKABLE QJsonObject readMessage();
    Q_INVOKABLE QVariantMap readMessageMap();

signals:
    void nameChanged();
    void connectionChanged();

    void readyReadMessage();

protected slots:
    void slotReadyReadMessage();

private:
    Q_DECLARE_PRIVATE(QJsonEndpoint)
    QScopedPointer<QJsonEndpointPrivate> d_ptr;

    // forbid copy constructor
    QJsonEndpoint(const QJsonEndpoint &);
    void operator=(const QJsonEndpoint &);
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_ENDPOINT_H

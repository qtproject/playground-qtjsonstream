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

#ifndef _JSON_ENDPOINT_H
#define _JSON_ENDPOINT_H

#include <QObject>
#include <QJsonObject>
#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonConnection;

class JsonEndpointPrivate;
class Q_ADDON_JSONSTREAM_EXPORT JsonEndpoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(JsonConnection* connection READ connection WRITE setConnection)
public:
    JsonEndpoint(const QString & = QString::null, JsonConnection * = 0);
    virtual ~JsonEndpoint();

    QString name() const;
    void    setName( const QString & name );

    JsonConnection *connection() const;
    void setConnection(JsonConnection *);

    Q_INVOKABLE bool send(const QVariantMap& message);
    Q_INVOKABLE bool send(const QJsonObject& message);

    Q_INVOKABLE bool messageAvailable();

    Q_INVOKABLE QJsonObject readMessage();
    Q_INVOKABLE QVariantMap readMessageMap();

signals:
    void readyReadMessage();

protected slots:
    void slotReadyReadMessage();

private:
    Q_DECLARE_PRIVATE(JsonEndpoint)
    QScopedPointer<JsonEndpointPrivate> d_ptr;

    // forbid copy constructor
    JsonEndpoint(const JsonEndpoint &);
    void operator=(const JsonEndpoint &);
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_ENDPOINT_H

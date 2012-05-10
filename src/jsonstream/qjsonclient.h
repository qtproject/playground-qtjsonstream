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

#ifndef JSON_CLIENT_H
#define JSON_CLIENT_H

#include <QObject>
#include <QVariant>
#include <QJsonObject>

#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonClientPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonClient : public QObject
{
    Q_OBJECT
public:
    QJsonClient(const QString& registration, QObject *parent = 0);
    QJsonClient(const QJsonObject& message, QObject *parent = 0);
    QJsonClient(QObject *parent = 0);
    ~QJsonClient();

    bool connectTCP(const QString& hostname, int port);
    bool connectLocal(const QString& socketname);

    bool send(const QJsonObject&);
    void setFormat( EncodingFormat format );

    // Do we really need a "connect with delay or error" facility?
    // All singleton information will be put in other classes...

signals:
    void messageReceived(const QJsonObject&);
    void disconnected();

private slots:
    void handleSocketDisconnected();
    void processMessages();

private:
    Q_DECLARE_PRIVATE(QJsonClient)
    QScopedPointer<QJsonClientPrivate> d_ptr;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // JSONCLIENT_H

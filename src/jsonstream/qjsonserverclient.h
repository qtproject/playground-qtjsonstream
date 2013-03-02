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

#ifndef _JSON_SERVER_CLIENT_H
#define _JSON_SERVER_CLIENT_H

#include <QObject>
#include <QJsonObject>

class QLocalSocket;

#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonAuthority;

class QJsonServerClientPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonServerClient : public QObject
{
    Q_OBJECT
public:
    QJsonServerClient(QObject *parent = 0);
    ~QJsonServerClient();

    void start();
    void stop();

    bool send(const QJsonObject &message);

    void setAuthority(QJsonAuthority *authority);

    const QLocalSocket *socket() const;
    void setSocket(QLocalSocket *socket);

    QString identifier() const;

signals:
    void disconnected(const QString& identifier);
    void messageReceived(const QString& identifier, const QJsonObject& message);

    void authorized(const QString& identifier);
    void authorizationFailed();

private slots:
    void received(const QJsonObject& message);
    void handleDisconnect();
    void processMessages();

private:
    Q_DECLARE_PRIVATE(QJsonServerClient)
    QScopedPointer<QJsonServerClientPrivate> d_ptr;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_SERVER_CLIENT_H

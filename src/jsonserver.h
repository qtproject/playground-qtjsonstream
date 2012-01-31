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

#ifndef _JSON_SERVER_H
#define _JSON_SERVER_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QSet>

class QLocalServer;

#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonStream;
class JsonAuthority;
class JsonServerClient;

class Q_ADDON_JSONSTREAM_EXPORT JsonServer : public QObject
{
    Q_OBJECT

public:
    JsonServer(QObject *parent = 0);
    virtual ~JsonServer();

    bool listen(int port, JsonAuthority *authority = 0);
    bool listen(const QString& socketname, JsonAuthority *authority = 0);

    QStringList connections() const;

    void enableQueuing(const QString &identifier);
    void disableQueuing(const QString &identifier);
    bool isQueuingEnabled(const QString &identifier);
    void clearQueue(const QString &identifier);

    void enableMultipleConnections(const QString& identifier);
    void disableMultipleConnections(const QString& identifier);

public slots:
    bool hasConnection(const QString &identifier) const;
    bool send(const QString &identifier, const QJsonObject& message);
    void broadcast(const QJsonObject& message);
    void removeConnection(const QString &identifier);

signals:
    void connectionAdded(const QString &identifier);
    void connectionRemoved(const QString &identifier);
    void messageReceived(const QString &identifier, const QJsonObject &message);
    void authorizationFailed();

protected slots:
    virtual void handleClientAuthorized(const QString &identifier);
    virtual void handleAuthorizationFailed();
    virtual void receiveMessage(const QString &identifier, const QJsonObject &message);
    virtual void clientDisconnected(const QString& identifier);

private slots:
    void handleLocalConnection();

private:
    QMap<QLocalServer *, JsonAuthority *>  m_localServers;
    QMultiMap<QString, JsonServerClient *> m_identifierToClient;
    QMap<QString, QList<QJsonObject> >     m_messageQueues;
    QSet<QString>                          m_multipleConnections;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_SERVER_H

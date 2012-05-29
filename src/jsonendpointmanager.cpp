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

#include "jsonendpointmanager_p.h"
#include "jsonendpoint.h"
#include "jsonconnection.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

static const QLatin1String kstrEndpointKey("endpoint");

/****************************************************************************/

/*!
    \class JsonEndpointManager
    \brief The JsonEndpointManager class ...

*/

/*!
  Constructs a \c JsonEndpointManager object.
 */

JsonEndpointManager::JsonEndpointManager(JsonConnection *parent)
    : QObject(parent), mInit(false), mEndpointPropertyName(kstrEndpointKey), mDefaultEndpoint(0)
{
}

/*!
  Deletes the \c JsonEndpointManager object.
 */

JsonEndpointManager::~JsonEndpointManager()
{
    clear();
}

JsonEndpoint *JsonEndpointManager::defaultEndpoint()
{
    JsonEndpoint *endpoint;
    endpoints();
    if (mDefaultEndpoint) {
        endpoint = mDefaultEndpoint;
    }
    else {
        endpoint = new JsonEndpoint();
        JsonConnection *connection = qobject_cast<JsonConnection *>(parent());
        if (connection) {
            connection->addEndpoint(endpoint);
            // XXX hack - i don't want the default endpoint in the endpoints list
            mEndpoints.remove(QString::number((ulong)endpoint));
        }
        mDefaultEndpoint = endpoint;
    }
    return endpoint;
}

void JsonEndpointManager::addEndpoint(JsonEndpoint *endpoint)
{
    if (mEndpoints.key(endpoint).isEmpty()) {
        mInit = false; // rehashing required
        mEndpoints.insert(QString::number((ulong)endpoint), endpoint);
    }
}

void JsonEndpointManager::removeEndpoint(JsonEndpoint *endpoint)
{
    mEndpoints.remove(endpoint->name());
    endpoint->setConnection(0);
}

QHash<QString, JsonEndpoint*> & JsonEndpointManager::endpoints()
{
    if (!mInit) {
        // rehash with real names
        QList<JsonEndpoint *> lst = mEndpoints.values();
        mEndpoints.clear();
        foreach (JsonEndpoint *e, lst) {
            mEndpoints.insert(e->name(), e);
        }
        mInit = true;
    }
    return mEndpoints;
}

JsonEndpoint *JsonEndpointManager::endpoint(const QJsonObject &message)
{
    return endpoints().value(message.value(mEndpointPropertyName).toString(), defaultEndpoint());
}

void JsonEndpointManager::clear()
{
    QList<JsonEndpoint *> lst = mEndpoints.values();
    foreach (JsonEndpoint *endpoint, lst) {
        endpoint->setConnection(0);
    }
    mEndpoints.clear();
}

#include "moc_jsonendpointmanager_p.cpp"

QT_END_NAMESPACE_JSONSTREAM


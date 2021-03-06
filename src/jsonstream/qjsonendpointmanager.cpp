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

#include "qjsonendpointmanager_p.h"
#include "qjsonendpoint.h"
#include "qjsonconnection.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

static const QLatin1String kstrEndpointKey("endpoint");

/****************************************************************************/

/*!
    \class QJsonEndpointManager
    \brief The QJsonEndpointManager class maintains a list of endpoints
    \internal

    and determines which endpoint should be used to process a given JSON message.
*/

/*!
  Constructs a \c QJsonEndpointManager object with \a parent.
 */

QJsonEndpointManager::QJsonEndpointManager(QJsonConnection *parent)
    : QObject(parent), mInit(false), mEndpointPropertyName(kstrEndpointKey), mDefaultEndpoint(0)
{
}

/*!
  Deletes the \c QJsonEndpointManager object.
 */

QJsonEndpointManager::~QJsonEndpointManager()
{
    clear();
}

/*!
  Returns the default endpoint.  This endpoint is used when no matching named
  endpoint could be found.

  \sa endpoint()
*/
QJsonEndpoint *QJsonEndpointManager::defaultEndpoint()
{
    QJsonEndpoint *endpoint;
    endpoints();
    if (mDefaultEndpoint) {
        endpoint = mDefaultEndpoint;
    }
    else {
        endpoint = new QJsonEndpoint();
        QJsonConnection *connection = qobject_cast<QJsonConnection *>(parent());
        if (connection) {
            connection->addEndpoint(endpoint);
            // XXX hack - i don't want the default endpoint in the endpoints list
            mEndpoints.remove(QString::number((ulong)endpoint));
        }
        mDefaultEndpoint = endpoint;
    }
    return endpoint;
}

/*!
    Add \a endpoint to the list
*/
void QJsonEndpointManager::addEndpoint(QJsonEndpoint *endpoint)
{
    if (mEndpoints.key(endpoint).isEmpty()) {
        mInit = false; // rehashing required
        mEndpoints.insert(QString::number((ulong)endpoint), endpoint);
        connect(endpoint, SIGNAL(nameChanged()), SLOT(handleNameChange()));
    }
}

/*!
    Remove \a endpoint from the list
*/
void QJsonEndpointManager::removeEndpoint(QJsonEndpoint *endpoint)
{
    mEndpoints.remove(endpoint->name());
    endpoint->setConnection(0);
}

/*!
    Return (a copy of) the list of endpoints.
*/
QHash<QString, QJsonEndpoint*> & QJsonEndpointManager::endpoints()
{
    if (!mInit) {
        // rehash with real names
        QList<QJsonEndpoint *> lst = mEndpoints.values();
        mEndpoints.clear();
        foreach (QJsonEndpoint *e, lst) {
            mEndpoints.insert(e->name(), e);
        }
        mInit = true;
    }
    return mEndpoints;
}

/*!
   Determines the endpoint that should be used to process \a message and returns it.

   The endpoint is determined by retrieving the value of the property named in
   endpointPropertyName() from \a message.  If that value matches one of the
   endpoints' \l{QJsonEndpoint::name()}{name()} properties, that endpoint is returned.
   If no matching endpoint could be found, the defaultEndpoint() is returned.
*/
QJsonEndpoint *QJsonEndpointManager::endpoint(const QJsonObject &message)
{
    return endpoints().value(message.value(mEndpointPropertyName).toString(), defaultEndpoint());
}

/*!
  Clears the endpoint list.
 */
void QJsonEndpointManager::clear()
{
    QList<QJsonEndpoint *> lst = mEndpoints.values();
    foreach (QJsonEndpoint *endpoint, lst) {
        endpoint->setConnection(0);
    }
    mEndpoints.clear();
}

void QJsonEndpointManager::handleNameChange()
{
    mInit = false;  // next call to endpoints() will rehash everything
}

/*! \property QJsonEndpointManager::endpointPropertyName
  Property name that will be used to retrieve a value from a message to determine
  what endpoint should be used to process that message.

  \sa endpoint()
*/

#include "moc_qjsonendpointmanager_p.cpp"

QT_END_NAMESPACE_JSONSTREAM


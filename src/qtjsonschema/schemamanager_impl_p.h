/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef SCHEMAMANAGER_IMPL_P_H
#define SCHEMAMANAGER_IMPL_P_H

#include "schemamanager_p.h"
#include "schemaobject_p.h"

#include "jsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

template<class T, class TT>
bool SchemaManager<T,TT>::contains(const QString &name) const
{
    return m_schemas.contains(name);
}

template<class T, class TT>
T SchemaManager<T,TT>::value(const QString &name) const
{
    return m_schemas.value(name).first;
}

template<class T, class TT>
SchemaValidation::Schema<TT> SchemaManager<T,TT>::schema(const QString &schemaName, TypesService *service)
{
    MapSchemaPair schemaPair = m_schemas.value(schemaName);
    ensureCompiled(schemaName, &schemaPair, service);
    return schemaPair.second;
}

template<class T, class TT>
T SchemaManager<T,TT>::take(const QString &name)
{
    return m_schemas.take(name).first;
}

template<class T, class TT>
T SchemaManager<T,TT>::insert(const QString &name, T &schema)
{
    m_schemas.insert(name, qMakePair(schema, SchemaValidation::Schema<TT>()));
    return T();
}

template<class T, class TT>
inline T SchemaManager<T,TT>::ensureCompiled(const QString &schemaName, MapSchemaPair *pair, TypesService *callbacks)
{
    SchemaValidation::Schema<TT> schema(pair->second);
    if (!schema.isValid()) {
        // Try to compile schema
        typename TT::Object schemaObject(pair->first);
        SchemaValidation::Schema<TT> compiledSchema(schemaObject, callbacks);
        pair->second = compiledSchema;
        m_schemas.insert(schemaName, *pair);
        return callbacks->error();
    }
    return T();
}

template<class T, class TT>
inline T SchemaManager<T,TT>::validate(const QString &schemaName, T object)
{
    if (!contains(schemaName))
        return T();

    TypesService callbacks(this);
    MapSchemaPair schemaPair = m_schemas.value(schemaName);
    ensureCompiled(schemaName, &schemaPair, &callbacks);
    SchemaValidation::Schema<TT> schema(schemaPair.second);
    typename TT::Value rootObject(QString(), object);
    /*bool result = */ schema.check(rootObject, &callbacks);
    return callbacks.error();
}

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAMANAGER_IMPL_P_H

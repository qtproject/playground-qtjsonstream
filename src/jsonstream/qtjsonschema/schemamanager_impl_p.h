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

#ifndef SCHEMAMANAGER_IMPL_P_H
#define SCHEMAMANAGER_IMPL_P_H

#include "schemamanager_p.h"
#include "schemaobject_p.h"

#include "qjsonschema-global.h"

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
    if (schema.hasErrors())
    {
        callbacks->setLoadError(QStringLiteral("Schema errors found. Schema can not be loaded properly."));
        return callbacks->error();
    }
    else if (!schema.isValid()) {
        // Try to compile schema
        typename TT::Object schemaObject(pair->first);
        SchemaValidation::Schema<TT> compiledSchema = SchemaValidation::Schema<TT>::compile(schemaObject, callbacks);
        pair->second = compiledSchema;
        m_schemas.insert(schemaName, *pair);
        return callbacks->error();
    }
    return T();
}

template<class T, class TT>
inline T SchemaManager<T,TT>::validate(const QString &schemaName, T object)
{
    TypesService callbacks(this);
    if (!contains(schemaName)) {
        callbacks.setValidationError(QString::fromLatin1("Schema '%1' not found.").arg(schemaName));
        return callbacks.error();
    }

    MapSchemaPair schemaPair = m_schemas.value(schemaName);
    if (ensureCompiled(schemaName, &schemaPair, &callbacks).isEmpty()) {
        // schema compiled successfully can validate an object
        SchemaValidation::Schema<TT> schema(schemaPair.second);
        typename TT::Value rootObject(QString(), object);
        schema.check(rootObject, &callbacks);
    }
    return callbacks.error();
}

template<class T, class TT>
inline QMap<QString, T> SchemaManager<T,TT>::schemas() const
{
    QMap<QString, T> map;
    typename QMap<QString, MapSchemaPair>::const_iterator it(m_schemas.constBegin());
    while (it != m_schemas.constEnd()) {
        map.insert(it.key(), it.value().first);
        ++it;
    }
    return map;
}

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAMANAGER_IMPL_P_H

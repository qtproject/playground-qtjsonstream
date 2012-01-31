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

#ifndef VAROBJECTTYPES_IMPL_P_H
#define VAROBJECTTYPES_IMPL_P_H

#include "varobjecttypes_p.h"
#include "schemamanager_p.h"

#include "jsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

inline VarObjectTypes::Value::Value(const QVariant &var)
    : QVariant(var)
{}

inline VarObjectTypes::Value::Value(Key, const QVariantMap &map)
    : QVariant(map)
{}

inline VarObjectTypes::Value::Value(const int, const QVariantList &list)
    : QVariant(list)
{}

inline int VarObjectTypes::Value::toInt(bool *ok) const
{
    if (type() == QVariant::Int || type() == QVariant::UInt)
    {
        *ok = true;
        return value<int>();
    }

    *ok = false;
    return -1;
}

inline double VarObjectTypes::Value::toDouble(bool *ok) const
{
    if (type() == QVariant::Double)
    {
        *ok = true;
        return value<double>();
    }

    *ok = false;
    return -1;
}

inline VarObjectTypes::ValueList VarObjectTypes::Value::toList(bool *ok) const
{
    if (type() == QVariant::List)
    {
        *ok = true;
        return value<QVariantList>();
    }

    *ok = false;
    return ValueList();
}

inline QString VarObjectTypes::Value::toString(bool *ok) const
{
    if (type() == QVariant::String)
    {
        *ok = true;
        return value<QString>();
    }

    *ok = false;
    return QString();
}

inline bool VarObjectTypes::Value::toBoolean(bool *ok) const
{
    if (type() == QVariant::Bool)
    {
        *ok = true;
        return value<bool>();
    }

    *ok = false;
    return false;
}

inline void VarObjectTypes::Value::toNull(bool *ok) const
{
    *ok = (type() == QVariant::Invalid);
}

inline VarObjectTypes::Object VarObjectTypes::Value::toObject(bool *ok) const
{
    if (type() == QVariant::Map)
    {
        *ok = true;
        return value<QVariantMap>();
    }

    *ok = false;
    return QVariantMap();
}

inline VarObjectTypes::Object::Object()
{}

inline VarObjectTypes::Object::Object(const QVariantMap &map)
    : QVariantMap(map)
{}

inline VarObjectTypes::Value VarObjectTypes::Object::property(const VarObjectTypes::Key& name) const
{
    qDebug() << "Object::property " << name << "[" << value(name) << "]";
    return value(name);
}

inline QList<VarObjectTypes::Key> VarObjectTypes::Object::propertyNames() const { return keys(); }

inline VarObjectTypes::Service::Service(SchemaManagerBase *schemas)
    : m_schemas(schemas)
{}

inline QVariantMap VarObjectTypes::Service::error() const
{
    return m_errorMap;
}

inline void VarObjectTypes::Service::setError(const QString &message)
{
    m_errorMap.insert("code", FailedSchemaValidation);
    m_errorMap.insert("message", message);
}

inline SchemaValidation::Schema<VarObjectTypes> VarObjectTypes::Service::loadSchema(const QString &schemaName)
{
    return reinterpret_cast< SchemaManager<QVariantMap, VarObjectTypes> *>(m_schemas)->schema(schemaName, this);
}

QT_END_NAMESPACE_JSONSTREAM

#endif // VAROBJECTTYPES_IMPL_P_H

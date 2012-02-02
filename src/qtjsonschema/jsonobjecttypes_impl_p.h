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

#ifndef JsonObjectTypes_IMPL_P_H
#define JsonObjectTypes_IMPL_P_H

#include "jsonobjecttypes_p.h"
#include "schemamanager_p.h"

#include "jsonschema-global.h"
#include "schemaerror.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

inline JsonObjectTypes::Value::Value(const QJsonValue &v)
    : QJsonValue(v)
{}

inline JsonObjectTypes::Value::Value(Key, const QJsonObject &v)
    : QJsonValue(v)
{}

inline int JsonObjectTypes::Value::toInt(bool *ok) const
{
    *ok = false;
    return -1;
}

inline double JsonObjectTypes::Value::toDouble(bool *ok) const
{
    if (isDouble())
    {
        *ok = true;
        return QJsonValue::toDouble();
    }

    *ok = false;
    return -1;
}

inline JsonObjectTypes::ValueList JsonObjectTypes::Value::toList(bool *ok) const
{
    if (isArray())
    {
        *ok = true;
        return QJsonValue::toArray();
    }

    *ok = false;
    return ValueList();
}

inline QString JsonObjectTypes::Value::toString(bool *ok) const
{
    if (isString())
    {
        *ok = true;
        return QJsonValue::toString();
    }

    *ok = false;
    return QString();
}

inline bool JsonObjectTypes::Value::toBoolean(bool *ok) const
{
    if (isBool())
    {
        *ok = true;
        return QJsonValue::toBool();
    }

    *ok = false;
    return false;
}

inline void JsonObjectTypes::Value::toNull(bool *ok) const
{
    *ok = (type() == Null);
}

inline JsonObjectTypes::Object JsonObjectTypes::Value::toObject(bool *ok) const
{
    if (isObject())
    {
        *ok = true;
        return QJsonValue::toObject();
    }

    *ok = false;
    return JsonObjectTypes::Object();
}

inline JsonObjectTypes::Object::Object()
{}

inline JsonObjectTypes::Object::Object(const QJsonObject &object)
    : QJsonObject(object)
{}

inline JsonObjectTypes::Value JsonObjectTypes::Object::property(const JsonObjectTypes::Key& name) const
{
    qDebug() << "Object::property " << name << "[" << value(name) << "]";
    return value(name);
}

inline QList<JsonObjectTypes::Key> JsonObjectTypes::Object::propertyNames() const { return keys(); }

inline JsonObjectTypes::Service::Service(SchemaManagerBase *schemas)
    : m_schemas(schemas)
{}

inline QJsonObject JsonObjectTypes::Service::error() const
{
    return m_errorMap;
}

inline void JsonObjectTypes::Service::setError(const QString &message)
{
    m_errorMap.insert("code", SchemaError::FailedSchemaValidation);
    m_errorMap.insert("message", message);
}

inline SchemaValidation::Schema<JsonObjectTypes> JsonObjectTypes::Service::loadSchema(const QString &schemaName)
{
    return reinterpret_cast< SchemaManager<QJsonObject, JsonObjectTypes> *>(m_schemas)->schema(schemaName, this);
}

QT_END_NAMESPACE_JSONSTREAM

#endif // JsonObjectTypes_IMPL_P_H

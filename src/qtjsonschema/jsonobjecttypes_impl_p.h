/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef JSONOBJECTTYPES_IMPL_P_H
#define JSONOBJECTTYPES_IMPL_P_H

#include "jsonobjecttypes_p.h"
#include "schemamanager_p.h"

#include "jsonschema-global.h"
#include "schemaerror.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

inline JsonObjectTypes::ValueList::ValueList(const QJsonArray &list) : QJsonArray(list)
{}

inline uint JsonObjectTypes::ValueList::count() const
{
    return QJsonArray::size();
}

inline JsonObjectTypes::ValueList::const_iterator JsonObjectTypes::ValueList::constBegin() const
{
    return const_iterator(0, this);
}

inline JsonObjectTypes::ValueList::const_iterator JsonObjectTypes::ValueList::constEnd() const
{
    return const_iterator(size(), this);
}

inline JsonObjectTypes::ValueList::const_iterator::const_iterator()
    : m_index(-1)
    , m_list(0)
{}

inline JsonObjectTypes::Value JsonObjectTypes::ValueList::const_iterator::operator *() const
{
    Q_ASSERT(isValid());
    return Value(m_index, *m_list);
}

inline bool JsonObjectTypes::ValueList::const_iterator::operator !=(const const_iterator &other) const
{
    return m_index != other.m_index || m_list != other.m_list;
}

inline JsonObjectTypes::ValueList::const_iterator& JsonObjectTypes::ValueList::const_iterator::operator ++()
{
    m_index++;
    return *this;
}

inline JsonObjectTypes::ValueList::const_iterator::const_iterator(int begin, const QJsonArray *list)
    : m_index(begin)
    , m_list(list)
{}

inline bool JsonObjectTypes::ValueList::const_iterator::isValid() const
{
    return m_index != -1 && m_index < m_list->size();
}

inline JsonObjectTypes::Value::Value(Key propertyName, const QJsonObject &map)
    : m_index(-1)
    , m_property(propertyName)
    , m_value(map)
    , m_type(propertyName.isEmpty() ? RootMap : Map)
{}

inline JsonObjectTypes::Value::Value(const int index, const QJsonArray &list)
    : m_index(index)
    , m_value(list)
    , m_type(List)
{}

inline int JsonObjectTypes::Value::toInt(bool *ok) const
{
    if (m_intCache.isValid()) {
        *ok = true;
        return m_intCache.value();
    }
    int result = 0;
    QJsonValue::Type type;
    switch (m_type) {
    case Map:
        type = typeMap();
        if (type == QJsonValue::Double) {
            QJsonValue v = map().value(m_property);
            double doubleResult = v.toDouble();
            int intResult = (int)doubleResult;
            if ((double)intResult == doubleResult) {
                *ok = true;
                result = intResult;
            } else {
                *ok = false;
            }
        } else {
            *ok = false;
        }
        m_intCache.set(*ok, result);
        return result;
    case List:
        type = typeList();
        if (type == QJsonValue::Double) {
            QJsonValue v = list().at(m_index);
            double doubleResult = v.toDouble();
            int intResult = (int)doubleResult;
            if ((double)intResult == doubleResult) {
                *ok = true;
                result = intResult;
            } else {
                *ok = false;
            }
        } else {
            *ok = false;
        }
        m_intCache.set(*ok, result);
        return result;
    case RootMap:
        break;
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return -1;
}

inline double JsonObjectTypes::Value::toDouble(bool *ok) const
{
    if (m_doubleCache.isValid()) {
        *ok = true;
        return m_doubleCache.value();
    }
    double result;
    QJsonValue::Type type;
    switch (m_type) {
    case Map:
        type = typeMap();
        *ok = type == QJsonValue::Double;
        result = map().value(m_property).toDouble();
        m_doubleCache.set(*ok, result);
        return result;
    case List:
        type = typeList();
        *ok = type == QJsonValue::Double;
        result = list().at(m_index).toDouble();
        m_doubleCache.set(*ok, result);
        return result;
    case RootMap:
        break;
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return -1;
}

inline JsonObjectTypes::ValueList JsonObjectTypes::Value::toList(bool *ok) const
{
    *ok = true;
    switch (m_type) {
    case Map:
        *ok = typeMap() == QJsonValue::Array;
        return map().value(m_property).toArray();
    case List:
        *ok = typeList() == QJsonValue::Array;
        return list().at(m_index).toArray();
    case RootMap:
        return m_value.toArray();
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return ValueList(QJsonArray());
}

inline QString JsonObjectTypes::Value::toString(bool *ok) const
{
    switch (m_type) {
    case Map:
        *ok = typeMap() == QJsonValue::String;
        return map().value(m_property).toString();
    case List:
        *ok = typeList() == QJsonValue::String;
        return list().at(m_index).toString();
    case RootMap:
        *ok = true;
        return QString(); // useful for debugging
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return QString();
}

inline bool JsonObjectTypes::Value::toBoolean(bool *ok) const
{
    switch (m_type) {
    case Map:
        *ok = typeMap() == QJsonValue::Bool;
        return map().value(m_property).toBool();
    case List:
        *ok = typeList() == QJsonValue::Bool;
        return list().at(m_index).toBool();
    case RootMap:
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return false;
}

inline void JsonObjectTypes::Value::toNull(bool *ok) const
{
    switch (m_type) {
    case Map:
        *ok = typeMap() == QJsonValue::Null;
    case List:
        *ok = typeList() == QJsonValue::Null;
    case RootMap:
    default:
        Q_ASSERT(false);
    }
    *ok = false;
}

inline JsonObjectTypes::Object JsonObjectTypes::Value::toObject(bool *ok) const
{
    switch (m_type) {
    case Map:
        *ok = typeMap() == QJsonValue::Object;
        return map().value(m_property).toObject();
    case List:
        *ok = typeList() == QJsonValue::Object;
        return list().at(m_index).toObject();
    case RootMap:
        *ok =  true;
        return static_cast<Object>(m_value.toObject());
    default:
        Q_ASSERT(false);
    }
    *ok = false;
    return Object(QJsonObject());
}

inline bool JsonObjectTypes::Value::compare(const Value & val) const
{
    QJsonValue v0(m_type == Map ? map().value(m_property) : (m_type == List ? list().at(m_index) : QJsonValue()));
    QJsonValue v1(val.m_type == Map ? val.map().value(val.m_property) : (val.m_type == List ? val.list().at(val.m_index) : QJsonValue()));
    return v0 == v1;
}

inline QJsonValue JsonObjectTypes::Value::value() const
{
    switch (m_type) {
    case Map:
        return map().value(m_property);
    case List:
        return list().at(m_index);
    case RootMap:
        return m_value;
    default:
        Q_ASSERT(false);
    }
    return QJsonValue();
}

inline const QJsonObject JsonObjectTypes::Value::map() const
{
    Q_ASSERT(m_type == Map);
    Q_ASSERT(!m_property.isEmpty());
    return m_value.toObject();
}

inline const QJsonArray JsonObjectTypes::Value::list() const
{
    Q_ASSERT(m_type == List);
    Q_ASSERT(m_index >= 0);
    return m_value.toArray();
}

inline QJsonValue::Type JsonObjectTypes::Value::typeMap() const
{
    if (m_jsonTypeCache.isValid())
        return m_jsonTypeCache.value();
    QJsonValue::Type result = map().value(m_property).type();
    m_jsonTypeCache.set(true, result);
    return result;
}

inline QJsonValue::Type JsonObjectTypes::Value::typeList() const
{
    if (m_jsonTypeCache.isValid())
        return m_jsonTypeCache.value();
    QJsonValue::Type result = list().at(m_index).type();
    m_jsonTypeCache.set(true, result);
    return result;
}

inline JsonObjectTypes::Object::Object()
{}

inline JsonObjectTypes::Object::Object(const QJsonObject &object)
    : QJsonObject(object)
{}

inline JsonObjectTypes::Value JsonObjectTypes::Object::property(const JsonObjectTypes::Key& name) const
{
    return Value(name, *this);
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
    m_errorMap.insert(SchemaError::kCodeStr, SchemaError::FailedSchemaValidation);
    m_errorMap.insert(SchemaError::kMessageStr, message);
}

inline SchemaValidation::Schema<JsonObjectTypes> JsonObjectTypes::Service::loadSchema(const QString &schemaName)
{
    return reinterpret_cast< SchemaManager<QJsonObject, JsonObjectTypes> *>(m_schemas)->schema(schemaName, this);
}

QT_END_NAMESPACE_JSONSTREAM

#endif // JSONOBJECTTYPES_IMPL_P_H

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

#ifndef JSONOBJECTTYPES_P_H
#define JSONOBJECTTYPES_P_H

#include "schemaobject_p.h"

#include <QPair>
#include <QJsonObject>
#include <QJsonArray>

#include "qjsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class SchemaManagerBase;

/**
  \internal
  This is type definition for schema validation framework.
  Essentially schema validation is independent from
  data representation. The performance cost of this indirection is about 0. We can consider removing
  it in future or leave it and check different data representation (think about QJSValue).

  These types define the simplest types in JSON.
  */
class JsonObjectTypes {
public:
    typedef QString Key;

    class Value;
    class ValueList : protected QJsonArray
    {
    public:
        ValueList() {};
        inline ValueList(const QJsonArray &list);

        // interface
        class const_iterator;
        inline uint count() const;
        inline const_iterator constBegin() const;
        inline const_iterator constEnd() const;
        class const_iterator
        {
            friend class ValueList;
        public:
            inline const_iterator();
            inline Value operator *() const;
            inline bool operator !=(const const_iterator &other) const;
            inline const_iterator& operator ++();

        private:
            inline const_iterator(int begin, const QJsonArray *list);
            inline bool isValid() const;

            int m_index;
            const QJsonArray *m_list;
        };
    };

    class Object;
    class Value
    {
        enum Type {List, Map, RootMap};

        template<class T>
        class Cache : protected QPair<bool, T>
        {
        public:
            Cache()
                : QPair<bool, T>(false, T())
            {}
            bool isValid() const { return QPair<bool, T>::first; }
            T value() const { Q_ASSERT(isValid()); return QPair<bool, T>::second; }
            void set(const bool ok, const T &value) { QPair<bool, T>::first = ok; QPair<bool, T>::second = value; }
        };

    public:
        inline Value(Key propertyName, const QJsonObject &);
        inline Value(const int index, const QJsonArray &list);

        // interface
        inline int toInt(bool *ok) const;
        inline double toDouble(bool *ok) const;
        inline ValueList toList(bool *ok) const;
        inline QString toString(bool *ok) const;
        inline bool toBoolean(bool *ok) const;
        inline void toNull(bool *ok) const;
        inline Object toObject(bool *ok) const;

        inline bool compare(const Value &) const;
        inline QJsonValue value() const;
        inline QString data() const; // human-readable format

    private:
        inline const QJsonObject map() const;
        inline const QJsonArray list() const;
        inline QJsonValue::Type typeMap() const;
        inline QJsonValue::Type typeList() const;

        const int m_index;
        const Key m_property;

        const QJsonValue m_value;
        const Type m_type;

        mutable Cache<QJsonValue::Type> m_jsonTypeCache;
        mutable Cache<int> m_intCache;
        mutable Cache<double> m_doubleCache;
    };

    class Object : public QJsonObject
    {
    public:
        inline Object(const QJsonObject &);

        // interface
        inline Object();
        inline Value property(const Key& name) const;
        inline QList<Key> propertyNames() const;
    };

    class Service {
    public:
        inline Service(SchemaManagerBase *schemas);
        inline QJsonObject error() const;

        void setRootSchema(const SchemaValidation::Schema<JsonObjectTypes> & _schema) { m_rootSchema = _schema; }
        const SchemaValidation::Schema<JsonObjectTypes> &rootSchema() const { return m_rootSchema; }

        // interface
        inline void setValidationError(const QString &message);
        inline void setLoadError(const QString &message);
        inline void setSubError(const QString &message, int errorCode);
        inline SchemaValidation::Schema<JsonObjectTypes> loadSchema(const QString &schemaName);

    private:
        SchemaManagerBase *m_schemas;
        QJsonObject m_errorMap;
        SchemaValidation::Schema<JsonObjectTypes> m_rootSchema;
    };
};

QT_END_NAMESPACE_JSONSTREAM

#endif // JSONOBJECTTYPES_P_H

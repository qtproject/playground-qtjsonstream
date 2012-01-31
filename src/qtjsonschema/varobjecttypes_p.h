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

#ifndef VAROBJECTTYPES_P_H
#define VAROBJECTTYPES_P_H

#include "schemaobject_p.h"

#include <QPair>

#include "jsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class SchemaManagerBase;

/**
  \internal
  This is type definition for schema validation framework. It was created because of planed change
  of data representation in jsondb (Bson -> Qson). Essentially schema validation is independent from
  data representation. The performance cost of this indirection is about 0. We can consider removing
  it in future or leave it and check different data representation (think about QJSValue).

  These types define the simplest types in JSON.
  */
class VarObjectTypes {
public:
    typedef QString Key;
    typedef QVariantList ValueList;

    class Object;
    class Value : QVariant
    {
    public:
        inline Value(const QVariant &var);
        inline Value(Key propertyName, const QVariantMap &map);
        inline Value(const int index, const QVariantList &list);

        // interface
        inline int toInt(bool *ok) const;
        inline double toDouble(bool *ok) const;
        inline ValueList toList(bool *ok) const;
        inline QString toString(bool *ok) const;
        inline bool toBoolean(bool *ok) const;
        inline void toNull(bool *ok) const;
        inline Object toObject(bool *ok) const;

    private:
    };

    class Object : public QVariantMap
    {
    public:
        inline Object(const QVariantMap &map);

        // interface
        inline Object();
        inline Value property(const Key& name) const;
        inline QList<Key> propertyNames() const;
    };

    class Service {
    public:
        inline Service(SchemaManagerBase *schemas);
        inline QVariantMap error() const;

        // interface
        inline void setError(const QString &message);
        inline SchemaValidation::Schema<VarObjectTypes> loadSchema(const QString &schemaName);

    private:
        SchemaManagerBase *m_schemas;
        QVariantMap m_errorMap;
    };
};

QT_END_NAMESPACE_JSONSTREAM

#endif // VAROBJECTTYPES_P_H

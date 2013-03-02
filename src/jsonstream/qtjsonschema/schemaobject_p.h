/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef OBJECT_H
#define OBJECT_H

#include <QtCore/qlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_HEADER

namespace SchemaValidation {

///**
//    Interface for object like classes. Instances of specialization for this class would be
//    used as input data for schema validation
//    \internal
//*/
//template<class T>
//class Object
//{
//public:
//    /**
//        Should return value of a property with the given \a name
//    */
//    Value<T> property(const Key<T>& name) const;

//    /**
//        Should return list of all properties name
//        \todo replace to iterator syntax
//    */
//    QList<Key<T> > propertyNames() const;

//};

///**
//  Interface for value like classes. A value can be one of types defined types in JSON
//  \internal
//*/
//template<class T>
//class Value {
//public:
//    int toInt(bool *ok) const;
//    double toDouble(bool *ok) const;
//    ValueList toList(bool *ok) const;
//    QString toString(bool *ok) const;
//    bool toBool(bool *ok) const;
//    void toNull(bool *ok) const;
//    Object<T> toObject(bool *ok) const;
//};
// ValueList::count()
// ValueList::constBegin()
// ValueList::constEnd()
// ValueList::const_iterator()
//
// void Service::setValidationError(const SchemaError &error)
// void Service::setLoadError(const QString &message)
// void Service::setSubError(const QString &message, int errorCode)
// Schema<T> Service::loadSchema(const QString &name)

template<class T>
class SchemaPrivate;

template<class T>
class Schema {
    typedef typename T::Value Value;
    typedef typename T::Key Key;
    typedef typename T::Object Object;
    typedef typename T::ValueList ValueList;
    typedef typename T::Service Service;

public:
    inline bool check(const Value &value, Service *callbackToUseForCheck) const;
    void checkDefault(Value &value, Object &object) const
    {
        d_ptr->checkDefault(value, object);
    }

    Schema()
        : d_ptr(new SchemaPrivate<T>())
    {}

protected:
    Schema(const Object& schema, Service *callbacksToUseForCompilation)
        : d_ptr(new SchemaPrivate<T>())
    {
        d_ptr->compile(schema, callbacksToUseForCompilation);
    }

public:
    static Schema compile(const Object& schemaObj, Service *callbacksToUseForCompilation)
    {
        Schema schema;
        if (callbacksToUseForCompilation) {
            // need to store a root schema to use for self reference
            callbacksToUseForCompilation->setRootSchema(schema);
        }
        schema.d_ptr->compile(schemaObj, callbacksToUseForCompilation);
        return schema;
    }

    bool isValid() const
    {
        return d_ptr->isValid();
    }

    bool hasErrors() const
    {
        return d_ptr->hasErrors();
    }

private:

    friend class SchemaPrivate<T>;
    QExplicitlySharedDataPointer<SchemaPrivate<T> > d_ptr;
};

template<class T>
class SchemaPrivate : public QSharedData
{
    typedef typename Schema<T>::Value Value;
    typedef typename Schema<T>::Key Key;
    typedef typename Schema<T>::Object Object;
    typedef typename Schema<T>::ValueList ValueList;
    typedef typename Schema<T>::Service Service;

    // this class is used to exchange information between attributes of a property
    // one instance is created for each schema property
    class CheckSharedData
    {
    public:
        enum Flag {
            NoFlags = 0x0,
            ExclusiveMinimum = 0x1,
            ExclusiveMaximum = 0x2,
            NoAdditionalProperties = 0x4,
            NoAdditionalItems = 0x8,
            HasItems = 0x10,
            HasProperties = 0x20
        };
        Q_DECLARE_FLAGS(Flags, Flag)
        Flags m_flags;
        QSharedPointer<Value> m_default; // keeps a default value
        QSharedPointer< Schema<T> > m_additionalSchema;
    };

    class Check {
    public:
        Check(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const char* errorMessage)
            : m_schema(schema)
            , m_data(data)
            , m_errorMessage(errorMessage)
        {
            Q_ASSERT(schema);
            Q_ASSERT(errorMessage);
        }
        virtual ~Check() {}
        bool check(const Value& value)
        {
            bool result = doCheck(value);
            if (!result) {
                // TODO it is tricky, as we do not have access to source code of this schema.
                // maybe we can set "additional, hidden " source property in each schema property, or some nice hash?
                m_schema->m_callbacks->setValidationError(QLatin1String("Schema validation error: ") +
                                                          QString::fromLatin1(m_errorMessage).arg(value.data()));
            }
            return result;
        }

        Value* getDefault() const { return m_data && m_data->m_default ? m_data->m_default.data() : 0; }

        virtual void checkDefault(Value &, Object &) const {} // do nothing in generic case

    protected:
        SchemaPrivate *m_schema;
        QSharedPointer<CheckSharedData> m_data; // is used to exchange information between attributes

        // return true if it is ok
        virtual bool doCheck(const Value&) = 0;
    private:
        const char *m_errorMessage;
    };

    // empty check
    class NullCheck;
    // 5.1
    class CheckType;
    // 5.2
    class CheckProperties;
    // 5.4
    class CheckAdditionalProperties;
    // 5.5
    class CheckItems;
    // 5.6
    class CheckAdditionalItems;
    // 5.7
    class CheckRequired;
    // 5.9
    class CheckMinimum;
    // 5.10
    class CheckMaximum;
    // 5.11
    class CheckExclusiveMinimum;
    // 5.12
    class CheckExclusiveMaximum;
    // 5.13
    class CheckMinItems;
    // 5.14
    class CheckMaxItems;
    // 5.16
    class CheckPattern;
    // 5.17
    class CheckMinLength;
    // 5.18
    class CheckMaxLength;
    // 5.19
    class CheckEnum;
    // 5.20
    class CheckDefault;
    // 5.21
    class CheckTitle;
    // 5.23
    class CheckFormat;
    // 5.24
    class CheckDivisibleBy;
    // 5.26
    class CheckExtends;
    // 5.28
    class CheckRef;
    class CheckDescription;

    inline Check *createCheckPoint(const Key &key, const Value &value, QSharedPointer<CheckSharedData> &data);
    inline bool check(const Value &value) const;

public:
    SchemaPrivate()
        : m_maxRequired(0)
        , m_requiredCount(0)
        , m_callbacks(0)
        , m_bLoadError(false)
    {}
    ~SchemaPrivate()
    {
        for (int i = 0; i < m_checks.count(); ++i)
            delete m_checks.at(i);
    }

    inline bool check(const Value &value, Service *callbackToUseForCheck) const;
    inline void compile(const Object &schema, Service *callbackToUseForCompile)
    {
        Q_ASSERT(callbackToUseForCompile);
        m_callbacks = callbackToUseForCompile;
        const QList<Key> checksKeys = schema.propertyNames();
        m_checks.reserve(checksKeys.count());

        // create object to share data between attribures of a property
        QSharedPointer<CheckSharedData> data(new CheckSharedData());

        foreach (const Key &key, checksKeys) {
            m_checks.append(createCheckPoint(key, schema.property(key), data));
        }
        m_callbacks = 0;
    }

    bool isValid() const
    {
        // we have some checks so it means that something got compiled.
        return m_checks.size() && !m_bLoadError;
    }

    bool hasErrors() const
    {
        return m_bLoadError;
    }

    void checkDefault(Value &value, Object &object) const;

    void setLoadError(const char *, const Value &, int);

private:
    QVarLengthArray<Check *, 4> m_checks;
    qint32 m_maxRequired;
    mutable qint32 m_requiredCount;
    mutable Service *m_callbacks;
    bool m_bLoadError;
};

}

#include "checkpoints_p.h"

QT_END_HEADER

#endif // OBJECT_H

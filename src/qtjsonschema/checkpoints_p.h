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

#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qregexp.h>

#include <QStringList>
#include <QDebug>
#include <QDate>
#include <QUrl>

#include <math.h>

QT_BEGIN_HEADER

#include "schemaerror.h"
QT_USE_NAMESPACE_JSONSTREAM

namespace SchemaValidation {

/**
  \internal
  This template is used for hash computation for static latin1 strings.
 */
template<ushort C1 = 0, ushort C2 = 0, ushort C3 = 0, ushort C4 = 0, ushort C5 = 0,
         ushort C6 = 0, ushort C7 = 0, ushort C8 = 0, ushort C9 = 0, ushort C10 = 0,
         ushort C11 = 0, ushort C12 = 0, ushort C13 = 0, ushort C14 = 0, ushort C15 = 0,
         ushort C16 = 0, ushort C17 = 0, ushort C18 = 0, ushort C19 = 0, ushort C20 = 0>
struct QStaticStringHash
{
    typedef QStaticStringHash<C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15, C16, C17, C18, C19, C20> Suffix;

    const static int Hash = (C1 ^ Suffix::Hash) + 8;
    //(C1 ^ ( (C2 ^ (...)) +8 )) +8
};

template<>
struct QStaticStringHash<>
{
    typedef QStaticStringHash<> Suffix;
    const static int Hash = 0;

    /**
      \internal
      This function has to be align with qStringHash::Hash value
    */
    inline static int hash(const QString &string)
    {
        const ushort *str = reinterpret_cast<const ushort*>(string.constData());
        return hash(str, 0, string.length());
    }
private:
    inline static int hash(const ushort *str, const int index, const int length)
    {
        return index != length ? (str[index] ^ hash(str, index + 1, length)) + 8
                     : 0;
    }
};

template<class T>
class SchemaPrivate<T>::NullCheck : public Check {
public:
    NullCheck(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data)
        : Check(schema, data, "") // TODO
    {}
protected:
    virtual bool doCheck(const Value&) { return true; }
};

// 5.1
template<class T>
class SchemaPrivate<T>::CheckType : public Check {
    enum Type {StringType = 0x0001,
               NumberType = 0x0002,
               IntegerType = 0x0004,
               BooleanType = 0x0008,
               ObjectType = 0x0010,
               ArrayType = 0x0020,
               NullType = 0x0040,
               AnyType = 0x0080,
               UnknownType = 0};
public:
    CheckType(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& type)
        : Check(schema, data, "Type check failed for %1")
        , m_type(UnknownType)
    {
        bool ok;
        QStringList typesName;

        QString typeName = type.toString(&ok);
        if (!ok) {
            ValueList typesList = type.toList(&ok);
            Q_ASSERT_X(ok, Q_FUNC_INFO, "Type is neither a string nor an array");
            typename ValueList::const_iterator i;
            for (i = typesList.constBegin(); i != typesList.constEnd(); ++i) {
                typeName = Value(*i).toString(&ok);
                if (ok) {
                    typesName << typeName;
                }
            }
        } else {
            typesName << typeName;
        }
        foreach (const QString &name, typesName) {
            const int hash = QStaticStringHash<>::hash(name.toLower());

            // FIXME there are chances of conflicts, do we care? What is chance for new types in JSON?
            // FIXME we need to check only 2 chars. That would be faster.
            // FIXME probably we want to support schemas here too.
            switch (hash) {
            case QStaticStringHash<'s','t','r','i','n','g'>::Hash:
                m_type |= StringType;
                break;
            case QStaticStringHash<'n','u','m','b','e','r'>::Hash:
                m_type |= NumberType;
                m_type |= IntegerType; // an integer is also a number
                break;
            case QStaticStringHash<'i','n','t','e','g','e','r'>::Hash:
                m_type |= IntegerType;
                break;
            case QStaticStringHash<'b','o','o','l','e','a','n'>::Hash:
                m_type |= BooleanType;
                break;
            case QStaticStringHash<'o','b','j','e','c','t'>::Hash:
                m_type |= ObjectType;
                break;
            case QStaticStringHash<'a','r','r','a','y'>::Hash:
                m_type |= ArrayType;
                break;
            case QStaticStringHash<'a','n','y'>::Hash:
                m_type |= AnyType;
                break;
            case QStaticStringHash<'n','u','l','l'>::Hash:
                m_type |= NullType;
                break;
            default:
                m_type |= UnknownType;
            }
        }

//        qDebug() << Q_FUNC_INFO << m_type << type.toString(&ok);
    }

    virtual bool doCheck(const Value &value)
    {
        if (m_type == UnknownType)
            return true;

        bool result = findType(value) & m_type;
//        bool ok;
//        qDebug() << Q_FUNC_INFO << findType(value)  << m_type << value.toString(&ok) << result;
        return result;
    }

private:
    inline Type findType(const Value &value) const
    {
        bool ok;
        // Lets assume that the value is valid.
        switch (m_type) {
        case StringType:
            value.toString(&ok);
            if (ok)
                return StringType;
            break;
        case NumberType:
            value.toDouble(&ok);
            if (ok)
                return NumberType;
            break;
        case IntegerType:
            value.toInt(&ok);
            if (ok)
                return IntegerType;
            break;
        case BooleanType:
            value.toBoolean(&ok);
            if (ok)
                return BooleanType;
            break;
        case ObjectType:
            value.toObject(&ok);
            if (ok)
                return ObjectType;
            break;
        case NullType:
            value.toNull(&ok);
            if (ok)
                return NullType;
            break;
        case AnyType:
            return AnyType;
        case UnknownType:
            break;
        default:
            break;
        };

        //TODO FIXME it can be better
        value.toInt(&ok);
        if (ok)
            return IntegerType;
        value.toDouble(&ok);
        if (ok)
            return NumberType;
        value.toObject(&ok);
        if (ok)
            return ObjectType;
        value.toString(&ok);
        if (ok)
            return StringType;
        value.toBoolean(&ok);
        if (ok)
            return BooleanType;
        value.toList(&ok);
        if (ok)
            return ArrayType;
        return AnyType;
    }

    uint m_type;
};

// 5.2
template<class T>
class SchemaPrivate<T>::CheckProperties : public Check {
public:
    CheckProperties(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &properties)
        : Check(schema, data, "Properties check failed for %1")
    {
        bool ok;
        const Object obj = properties.toObject(&ok);
        Q_ASSERT(ok);

        QList<Key> propertyNames = obj.propertyNames();
//        qDebug() << "    propertyNames: " << propertyNames <<this;

        m_checks.reserve(propertyNames.count());
        foreach (const Key &propertyName, propertyNames) {
            QVarLengthArray<Check *, 4> checks;
            const Object propertyChecks = obj.property(propertyName).toObject(&ok);
//            qDebug() << "    propertyChecks:" << propertyChecks;

            // create object to share data between attribures of a property
            QSharedPointer<CheckSharedData> data(new CheckSharedData());

            Q_ASSERT(ok);
            foreach (const Key &key, propertyChecks.propertyNames()) {
//                bool ok;
//                qDebug() << "        key:" << key << this << propertyChecks.property(key).toString(&ok)<< propertyChecks.property(key).toInt(&ok);
                checks.append(schema->createCheckPoint(key, propertyChecks.property(key), data));
            }
            m_checks.insert(propertyName, checks);
        }
    }

    ~CheckProperties()
    {
        typename QHash<Key, QVarLengthArray<Check *, 4> >::const_iterator i;
        for (i = m_checks.constBegin(); i != m_checks.constEnd(); ++i) {
            typename QVarLengthArray<Check *, 4>::const_iterator j;
            for (j = i.value().constBegin(); j != i.value().constEnd(); ++j){
                delete *j;
            }
        }
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        Object object = value.toObject(&ok);
        if (!ok)
            return false;

        if (Check::m_data->m_flags.testFlag(CheckSharedData::NoAdditionalProperties)) {
            QList<Key> strsSchemaProperties(m_checks.keys());
            QList<Key> strsObjectProperties(object.propertyNames());
            if (strsSchemaProperties.size() == strsObjectProperties.size()) {
                // number of properties are the same but lists still may differ
                qSort(strsSchemaProperties);
                qSort(strsObjectProperties);
                if (!qEqual(strsSchemaProperties.constBegin(), strsSchemaProperties.constEnd(), strsObjectProperties.constBegin())) {
                    // lists of properties differ - return an additionalProperties error
                    return false;
                }
            }
            else {
                // number of properties differ - return an additionalProperties error
                return false;
            }
        }

        //qDebug() << Q_FUNC_INFO;
        foreach (const Key &key, object.propertyNames()) {
            QVarLengthArray<Check *, 4> empty;
            QVarLengthArray<Check *, 4> checks = m_checks.value(key, empty);
            Value property = object.property(key);
            foreach (Check *check, checks) {
                //qDebug()  <<"CHECKING:" << check;
                if (!check->check(property)) {
                    return false;
                }
            }

            if (Check::m_data->m_additionalSchema && 0 == checks.count() && !m_checks.keys().contains(key)) {
                // do an extra property check if a property does not exist in the schema
                if (!Check::m_data->m_additionalSchema->check(property, Check::m_schema->m_callbacks)) {
                    return false;
                }
            }
        }

        return true;
    }


    void checkDefault(Value& value, Object &_object) const
    {
        bool ok;
        Object object = value.toObject(&ok);
        if (!ok)
            return;

        //qDebug() << Q_FUNC_INFO;

        // create missing properties list
        QList<Key> strs(m_checks.keys());

        foreach (const Key &key, object.propertyNames()) {
            QVarLengthArray<Check *, 4> empty;
            QVarLengthArray<Check *, 4> checks = m_checks.value(key, empty);
            Value property = object.property(key);

            // remove from missing properties list
            strs.removeOne(key);

            if (_object[key].isObject()) {
                foreach (Check *check, checks) {
                    Object oo(_object[key].toObject());
                    check->checkDefault(property, oo);
                    _object[key] = oo;
                }
            }
        }

        // add defaults for missing properties
        foreach (const Key &key, strs) {
            QVarLengthArray<Check *, 4> empty;
            QVarLengthArray<Check *, 4> checks = m_checks.value(key, empty);
            Value property = object.property(key);

            if (checks.first()->getDefault()) { // basic type
                _object.insert(key, checks.first()->getDefault()->value());
            }
            else { // looks like object or array
                Object object;
                foreach (Check *check, checks) {
                    Value value(QStringLiteral(""), Object());
                    check->checkDefault(value, object);
                }

                if (!object.isEmpty()) {
                    _object.insert(key, object);
                }
            }
        }
    }

private:
    QHash<Key, QVarLengthArray<Check *, 4> > m_checks;
};

// 5.4
template<class T>
class SchemaPrivate<T>::CheckAdditionalProperties : public Check {
public:
    CheckAdditionalProperties(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &_value)
        : Check(schema, data, "Additional properties check failed for %1")
    {
        bool ok, bAdditional = _value.toBoolean(&ok);
        if (ok && !bAdditional) {
            Check::m_data->m_flags |= CheckSharedData::NoAdditionalProperties;
        }
        else if (!ok) {
            // object if not bool
            Object obj = _value.toObject(&ok);
            if (ok) {
                // create extra check for additional properties - create new schema
                Check::m_data->m_additionalSchema = QSharedPointer< Schema<T> >(new Schema<T>(obj, schema->m_callbacks));
            }
        }
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value &)
    {
        // actual check is done in CheckAdditionalProperties::doCheck
        return true;
    }
};

// 5.5
template<class T>
class SchemaPrivate<T>::CheckItems : public Check {
public:
    CheckItems(SchemaPrivate *schemap, QSharedPointer<CheckSharedData> &data, const Value &schema)
        : Check(schemap, data, "Items check failed for %1"), m_bList(false)
    {
        // qDebug()  << Q_FUNC_INFO << this;
        bool ok;
        Object obj = schema.toObject(&ok);
        if (ok) {
            m_schema.append(Schema<T>(obj, schemap->m_callbacks));
        }
        else {
            ValueList list = schema.toList(&ok);
            if (ok) {
                m_bList = true;
                if (list.count() > 1)
                    m_schema.reserve(list.count());
                typename ValueList::const_iterator it;
                for (it = list.constBegin(); it != list.constEnd(); ++it) {
                    Object obj = (*it).toObject(&ok);
                    if (ok) {
                        m_schema.append(Schema<T>(obj, schemap->m_callbacks));
                    }
                }
            }
        }
        Check::m_data->m_flags |= CheckSharedData::HasItems;
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value& value)
    {
        //qDebug() << Q_FUNC_INFO << this;
        bool ok;
        ValueList array = value.toList(&ok);
        if (!ok)
            return false;

        bool bNoAdditional;
        if ((bNoAdditional = Check::m_data->m_flags.testFlag(CheckSharedData::NoAdditionalItems))) {
            if (m_bList && array.count() > (uint) m_schema.size())
                return false; // AdditionalItems is set to false
        }

        int nCnt(0), nIndex(0);
        Schema<T> & schema = m_schema[0];
        typename ValueList::const_iterator i;
        for (i = array.constBegin(); i != array.constEnd(); ++i, nCnt++) {
            if (m_bList) {
                // for a list each item should have a matching schema
                if (m_schema.size() > nCnt) {
                    nIndex = nCnt;
                    schema = m_schema[nIndex];
                }
                else if (Check::m_data->m_additionalSchema) {
                    schema = *Check::m_data->m_additionalSchema;
                }
                else {
                    return true; // nothing to validate with
                }
            }

            if (!schema.check(*i, Check::m_schema->m_callbacks)) {
                return false;
            }
        }
        return true;
    }
private:
    QVarLengthArray<Schema<T>, 1> m_schema;
    bool m_bList;
};

// 5.6
template<class T>
class SchemaPrivate<T>::CheckAdditionalItems : public Check {
public:
    CheckAdditionalItems(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &_value)
        : Check(schema, data, "Additional items check failed for %1")
    {
        bool ok, bAdditional = _value.toBoolean(&ok);
        if (ok && !bAdditional) {
            Check::m_data->m_flags |= CheckSharedData::NoAdditionalItems;
        }
        else if (!ok) {
            // object if not bool
            Object obj = _value.toObject(&ok);
            if (ok) {
                // create extra check for additional properties - create new schema
                Check::m_data->m_additionalSchema = QSharedPointer< Schema<T> >(new Schema<T>(obj, schema->m_callbacks));
            }
        }
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value &value)
    {
        // most of the time a check is done inside CheckItems::doCheck
        if (!Check::m_data->m_flags.testFlag(CheckSharedData::HasItems)) { // items attribute is absent so do a check here
            if (Check::m_data->m_flags.testFlag(CheckSharedData::NoAdditionalItems)) {
                // items attribute is absent, but additionalItems is set to false
                return false;
            }
            else if (Check::m_data->m_additionalSchema) {
                bool ok;
                ValueList array = value.toList(&ok);
                if (!ok)
                    return false;

                typename ValueList::const_iterator i;
                for (i = array.constBegin(); i != array.constEnd(); ++i) {
                    if (!Check::m_data->m_additionalSchema->check(*i, Check::m_schema->m_callbacks)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
};

// 5.7
template<class T>
class SchemaPrivate<T>::CheckRequired : public Check {
public:
    CheckRequired(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &required)
        : Check(schema, data, "Check required field")  // TODO what to do about Required ?
    {
        bool ok;
        m_req = required.toBoolean(&ok);
        if (!ok) {
            // maybe someone used string instead of bool
            QString value = required.toString(&ok).toLower();
            if (value == QString::fromLatin1("false"))
                m_req = false;
            else if (value == QString::fromLatin1("true"))
                m_req = true;
            else
                Q_ASSERT(false);

            qWarning() << QString::fromLatin1("Wrong 'required' syntax found, instead of boolean type a string was used");
        }
        Q_ASSERT(ok);
        if (m_req)
            Check::m_schema->m_maxRequired++;
    }

    virtual bool doCheck(const Value&)
    {
        //qDebug() << Q_FUNC_INFO << m_schema << this;
        if (m_req)
            Check::m_schema->m_requiredCount++;
        return true;
    }
private:
    bool m_req;
};

// 5.9
template<class T>
class SchemaPrivate<T>::CheckMinimum : public Check {
public:
    CheckMinimum(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &minimum)
        : Check(schema, data, "Minimum check failed for %1")
    {
        bool ok;
        m_min = minimum.toDouble(&ok);

        if (!ok) {
            Check::m_schema->setLoadError("wrong 'minimum' value", minimum, SchemaError::SchemaWrongParamType);
        }
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        double d = value.toDouble(&ok);
        return ok && (Check::m_data->m_flags.testFlag(CheckSharedData::ExclusiveMinimum) ? d > m_min : d >= m_min);
    }
private:
    double m_min;
};

// 5.10
template<class T>
class SchemaPrivate<T>::CheckMaximum : public Check {
public:
    CheckMaximum(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &maximum)
        : Check(schema, data, "Maximum check failed for %1")
    {
        // qDebug()  << Q_FUNC_INFO << this;
        bool ok;
        m_max = maximum.toDouble(&ok);

        if (!ok) {
            Check::m_schema->setLoadError("wrong 'maximum' value", maximum, SchemaError::SchemaWrongParamType);
        }
    }

    virtual bool doCheck(const Value &value)
    {
        //qDebug() << Q_FUNC_INFO << value << m_max << this;
        bool ok;
        double d = value.toDouble(&ok);
        return ok && (Check::m_data->m_flags.testFlag(CheckSharedData::ExclusiveMaximum) ? d < m_max : d <= m_max);
    }
private:
    double m_max;
};


// 5.11
template<class T>
class SchemaPrivate<T>::CheckExclusiveMinimum : public Check {
public:
    CheckExclusiveMinimum(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &_value)
        : Check(schema, data, "Exclusive minimum check failed for %1")
    {
        bool ok;
        bool bExclusive = _value.toBoolean(&ok);
        if (!ok) {
            // maybe someone used string instead of bool
            QString value = _value.toString(&ok).toLower();
            if (value == QString::fromLatin1("false"))
                bExclusive = false;
            else if (value == QString::fromLatin1("true"))
                bExclusive = true;
            else
                ok = false;

            if (!value.isEmpty())
                qWarning() << QString::fromLatin1("Wrong 'exclusiveMinimum' syntax found, instead of boolean type a string was used");
        }

        if (!ok) {
            Check::m_schema->setLoadError("wrong 'exclusiveMinimum' value", _value, SchemaError::SchemaWrongParamType);
        }

        if (bExclusive) {
            Check::m_data->m_flags |= CheckSharedData::ExclusiveMinimum;
        }
    }

    virtual bool doCheck(const Value &)
    {
        // check will be done in minimum
        return true;
    }
private:
};

// 5.12
template<class T>
class SchemaPrivate<T>::CheckExclusiveMaximum : public Check {
public:
    CheckExclusiveMaximum(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &_value)
        : Check(schema, data, "Exclusive minimum check failed for %1")
    {
        bool ok;
        bool bExclusive = _value.toBoolean(&ok);
        if (!ok) {
            // maybe someone used string instead of bool
            QString value = _value.toString(&ok).toLower();
            if (value == QString::fromLatin1("false"))
                bExclusive = false;
            else if (value == QString::fromLatin1("true"))
                bExclusive = true;
            else
                ok = false;

            if (!value.isEmpty())
                qWarning() << QString::fromLatin1("Wrong 'exclusiveMaximum' syntax found, instead of boolean type a string was used");
        }

        if (!ok) {
            Check::m_schema->setLoadError("wrong 'exclusiveMaximum' value", _value, SchemaError::SchemaWrongParamType);
        }

        if (bExclusive) {
            Check::m_data->m_flags |= CheckSharedData::ExclusiveMaximum;
        }
    }

    virtual bool doCheck(const Value &)
    {
        // check will be done in maximum
        return true;
    }
private:
};

// 5.13
template<class T>
class SchemaPrivate<T>::CheckMinItems : public Check {
public:
    CheckMinItems(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& minimum)
        : Check(schema, data, "Minimum item count check failed for %1")
    {
        bool ok;
        m_min = minimum.toInt(&ok);
        if (!ok) {
            Check::m_schema->setLoadError("wrong 'minItems' value", minimum,
                                          SchemaError::SchemaWrongParamType);
        }
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        int count = value.toList(&ok).count();
        return count >= m_min && ok;
    }
private:
    int m_min;
};

// 5.14
template<class T>
class SchemaPrivate<T>::CheckMaxItems : public Check {
public:
    CheckMaxItems(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& maximum)
        : Check(schema, data, "Maximum item count check failed for %1")
    {
        bool ok;
        m_max = maximum.toInt(&ok);
        if (!ok) {
            Check::m_schema->setLoadError("wrong 'maxItems' value", maximum,
                                          SchemaError::SchemaWrongParamType);
        }
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        int count = value.toList(&ok).count();
        return count <= m_max && ok;
    }

private:
    int m_max;
};

// 5.16
template<class T>
class SchemaPrivate<T>::CheckPattern : public Check {
public:
    CheckPattern(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& patternValue)
        : Check(schema, data, "Pattern check failed for %1")
    {
        bool ok;
        QString patternString = patternValue.toString(&ok);
        m_regexp.setPattern(patternString);
        if (!ok || !m_regexp.isValid()) {
            Check::m_schema->setLoadError("wrong 'pattern' value", patternValue,
                                          !ok ? SchemaError::SchemaWrongParamType : SchemaError::SchemaWrongParamValue);
        }
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        QString str = value.toString(&ok);
        if (!ok) {
            // According to spec (5.15) we should check the value only when it exist and it is a string.
            // It is a bit strange, but I think we have to return true here.
            return true;
        }
        return m_regexp.exactMatch(str);
    }
private:
    QRegExp m_regexp;
};

// 5.17
template<class T>
class SchemaPrivate<T>::CheckMinLength : public Check {
public:
    CheckMinLength(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& min)
        : Check(schema, data, "Minimal string length check failed for %1")
    {
        bool ok;
        m_min = min.toInt(&ok);
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        QString str = value.toString(&ok);
//        qDebug() << Q_FUNC_INFO << str << ok;
        if (!ok) {
            // According to spec (5.16) we should check the value only when it exist and it is a string.
            // It is a bit strange, but I think we have to return true here.
            return true;
        }
        return str.length() >= m_min;
    }
private:
    int m_min;
};

// 5.18
template<class T>
class SchemaPrivate<T>::CheckMaxLength : public Check {
public:
    CheckMaxLength(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& max)
        : Check(schema, data, "Maximal string length check failed for %1")
    {
        bool ok;
        m_max = max.toInt(&ok);
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok;
        QString str = value.toString(&ok);
//        qDebug() << Q_FUNC_INFO << str << ok;
        if (!ok) {
            // According to spec (5.16) we should check the value only when it exist and it is a string.
            // It is a bit strange, but I think we have to return true here.
            return true;
        }
        return str.length() <= m_max;
    }
private:
    int m_max;
};

// 5.19
template<class T>
class SchemaPrivate<T>::CheckEnum : public Check {
public:
    CheckEnum(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& value)
        : Check(schema, data, "Enum check failed for %1")
    {
        bool ok;
        m_enum = value.toList(&ok);
        Q_ASSERT(ok);
    }

    virtual bool doCheck(const Value &value)
    {
        typename ValueList::const_iterator i;
        for (i = m_enum.constBegin(); i != m_enum.constEnd(); ++i) {
            if (value.compare(*i)) {
                return true;
            }
        }
        return false;
    }

private:
    ValueList m_enum;
};

// 5.20
template<class T>
class SchemaPrivate<T>::CheckDefault : public Check {
public:
    CheckDefault(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& value)
        : Check(schema, data, "Default check failed for %1")
    {
        // used shared data to store
        Check::m_data->m_default = QSharedPointer<Value>(new Value(value));
    }

    virtual bool doCheck(const Value &)
    {
        return true;
    }
private:
};

// 5.23
template<class T>
class SchemaPrivate<T>::CheckFormat : public Check {
public:
    CheckFormat(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value& value)
        : Check(schema, data, "Enum format failed for %1")
    {
        bool ok;
        m_format = value.toString(&ok).toLower();
        Q_ASSERT(ok && !m_format.isEmpty());
    }

    virtual bool doCheck(const Value &value)
    {
        bool ok = true;
        int hash = QStaticStringHash<>::hash(m_format);
        switch (hash) {
        case QStaticStringHash<'d','a','t','e','-','t','i','m','e'>::Hash:
            if (QString::fromLatin1("date-time") == m_format) {
                QString str;
                if (!(str = value.toString(&ok)).isEmpty() && ok) {
                    return QDateTime::fromString(str, QStringLiteral("yyyy-MM-ddThh:mm:ssZ")).isValid();
                }
            }
            break;
        case QStaticStringHash<'d','a','t','e'>::Hash:
            if (QString::fromLatin1("date") == m_format) {
                QString str;
                if (!(str = value.toString(&ok)).isEmpty() && ok) {
                    return QDate::fromString(str, QStringLiteral("yyyy-MM-dd")).isValid();
                }
            }
            break;
        case QStaticStringHash<'t','i','m','e'>::Hash:
            if (QString::fromLatin1("time") == m_format) {
                QString str;
                if (!(str = value.toString(&ok)).isEmpty() && ok) {
                    return QTime::fromString(str, QStringLiteral("hh:mm:ss")).isValid();
                }
            }
            break;
        case QStaticStringHash<'r','e','g','e','x','p'>::Hash:
            if (QString::fromLatin1("regexp") == m_format) {
            }
            break;
        case QStaticStringHash<'u','r','i'>::Hash:
            if (QString::fromLatin1("uri") == m_format) {
                QString str;
                if (!(str = value.toString(&ok)).isEmpty() && ok) {
                    return str.contains(':') || QUrl(str, QUrl::StrictMode).isValid(); // URN or URL
                }
            }
            break;
        case QStaticStringHash<'u','r','l'>::Hash:
            if (QString::fromLatin1("url") == m_format) {
                QString str;
                if (!(str = value.toString(&ok)).isEmpty() && ok) {
                    return QUrl(str, QUrl::StrictMode).isValid();
                }
            }
            break;
        case QStaticStringHash<'p','h','o','n','e'>::Hash:
            if (QString::fromLatin1("phone") == m_format) {
                //TODO
            }
            break;
        case QStaticStringHash<'e','m','a','i','l'>::Hash:
            if (QString::fromLatin1("email") == m_format) {
                //TODO
            }
            break;
        case QStaticStringHash<'i','p','-','a','d','d','r','e','s','s'>::Hash:
            if (QString::fromLatin1("ip-address") == m_format) {
                //TODO
            }
            break;
        case QStaticStringHash<'i','p','v','6'>::Hash:
            if (QString::fromLatin1("ipv6") == m_format) {
                //TODO
            }
            break;
        case QStaticStringHash<'h','o','s','t','-','n','a','m','e'>::Hash:
            if (QString::fromLatin1("host-name") == m_format) {
                //TODO
            }
            break;
        case QStaticStringHash<'n','o','n','n','e','g','a','t','i','v','e','i','n','t','e','g','e','r'>::Hash: // NonNegativeInteger
            if (QString::fromLatin1("nonnegativeinteger") == m_format) {
                return (value.toInt(&ok) >= 0 && ok);
            }
            break;
        }
        return ok;
    }

private:
    QString m_format;
};

// 5.24
template<class T>
class SchemaPrivate<T>::CheckDivisibleBy : public Check {
public:
    CheckDivisibleBy(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &value)
        : Check(schema, data, "DivisibleBy check failed for %1")
    {
        // qDebug()  << Q_FUNC_INFO << this;
        bool ok;
        m_div = value.toDouble(&ok);

        if (!ok || m_div == 0)
            Check::m_schema->setLoadError("wrong 'divisibleBy' value", value,
                                          !ok ? SchemaError::SchemaWrongParamType : SchemaError::SchemaWrongParamValue);
    }

    virtual bool doCheck(const Value &value)
    {
        //qDebug() << Q_FUNC_INFO << value << m_div << this;
        bool ok;
        double d = value.toDouble(&ok);
        return m_div != 0 && ok && fmod(d, m_div) == 0;
    }
private:
    double m_div;
};

// 5.26
template<class T>
class SchemaPrivate<T>::CheckExtends : public Check {
public:
    CheckExtends(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &value)
        : Check(schema, data, "Extends check failed for %1")
    {
        // FIXME
        // Keep in mind that there is a bug in spec. (internet draft 3).
        // We should search for a schema not for a string here.
        // Tests are using "string" syntax, so we need to support it for a while
        bool ok;
        Object obj = value.toObject(&ok);
        if (!ok) {
            QString schemaName = value.toString(&ok);
            if (!ok || schemaName.length()) {
                ValueList array = value.toList(&ok);
                Q_ASSERT(ok);
                typename ValueList::const_iterator i;
                for (i = array.constBegin(); i != array.constEnd(); ++i) {
                    obj = Value(*i).toObject(&ok);
                    Q_ASSERT(ok);
                    m_extendedSchema.append(Schema<T>(obj, schema->m_callbacks));
                }
            } else {
                qWarning() << QString::fromLatin1("Wrong 'extends' syntax found, instead of \"%1\" should be \"%2\"")
                              .arg(schemaName, QString::fromLatin1("{\"$ref\":\"%1\"}").arg(schemaName));
                m_extendedSchema.append(schema->m_callbacks->loadSchema(schemaName));
            }
        } else {
            m_extendedSchema.append(Schema<T>(obj, schema->m_callbacks));
        }
    }

    virtual bool doCheck(const Value &value)
    {
        for (int i = 0; i < m_extendedSchema.count(); ++i) {
            if (!m_extendedSchema[i].check(value, Check::m_schema->m_callbacks))
                return false;
        }
        return true;
    }

    void checkDefault(Value& value, Object &_object) const
    {
        for (int i = 0; i < m_extendedSchema.count(); ++i) {
            m_extendedSchema[i].checkDefault(value, _object);
        }
    }

private:
    QVarLengthArray<Schema<T>, 4> m_extendedSchema;
};

// 5.28
template<class T>
class SchemaPrivate<T>::CheckRef : public Check {
public:
    CheckRef(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data, const Value &value)
        : Check(schema, data, "$Ref check failed for %1")
    {
        // TODO according to spec we should replace existing check by this one
        // I'm not sure what does it mean. Should we remove other checks?
        // What if we have two $ref? Can it happen? For now, lets use magic of
        // undefined bahaviour (without crashing of course).
        bool ok;
        QString schemaName = value.toString(&ok);
        Q_ASSERT(ok);

        m_newSchema = schema->m_callbacks->loadSchema(schemaName);
        if (!m_newSchema.isValid()) {
            // FIXME should we have current schema name?
            const QString msg =  QString::fromLatin1("Schema extends %1 but it is unknown.")
                    .arg(schemaName);
            qWarning() << msg;
            schema->m_callbacks->setValidationError(msg);
        }
    }
    virtual bool doCheck(const Value &value)
    {
        bool result = m_newSchema.check(value, Check::m_schema->m_callbacks);
//        qDebug() << Q_FUNC_INFO << result;
        return result;
    }

    void checkDefault(Value& value, Object &_object) const
    {
        m_newSchema.checkDefault(value, _object);

    }

private:
    Schema<T> m_newSchema;
};

template<class T>
class SchemaPrivate<T>::CheckDescription : public NullCheck {
public:
    CheckDescription(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data)
        : NullCheck(schema, data)
    {}
};

template<class T>
class SchemaPrivate<T>::CheckTitle : public NullCheck {
public:
    CheckTitle(SchemaPrivate *schema, QSharedPointer<CheckSharedData> &data)
        : NullCheck(schema, data)
    {}
};

template<class T>
typename SchemaPrivate<T>::Check *SchemaPrivate<T>::createCheckPoint(const Key &key, const Value &value, QSharedPointer<CheckSharedData> &data)
{
    QString keyName = key;
    keyName = keyName.toLower();

    // This is a perfect hash. BUT spec, in future, can be enriched by new values, that we should just ignore.
    // As we do not know about them we can't be sure that our perfect hash will be still perfect, therefore
    // we have to do additional string comparison that confirm result hash function result.
    int hash = QStaticStringHash<>::hash(keyName);
    switch (hash) {
    case QStaticStringHash<'r','e','q','u','i','r','e','d'>::Hash:
        if (QString::fromLatin1("required") == keyName)
            return new CheckRequired(this, data, value);
        break;
    case QStaticStringHash<'m','a','x','i','m','u','m'>::Hash:
        if (QString::fromLatin1("maximum") == keyName)
            return new CheckMaximum(this, data, value);
        break;
    case QStaticStringHash<'e','x','c','l','u','s','i','v','e','m','a','x','i','m','u','m'>::Hash:
        if (QString::fromLatin1("exclusivemaximum") == keyName)
            return new CheckExclusiveMaximum(this, data, value);
        break;
    case QStaticStringHash<'m','i','n','i','m','u','m'>::Hash:
        if (QString::fromLatin1("minimum") == keyName)
            return new CheckMinimum(this, data, value);
        break;
    case QStaticStringHash<'e','x','c','l','u','s','i','v','e','m','i','n','i','m','u','m'>::Hash:
        if (QString::fromLatin1("exclusiveminimum") == keyName)
            return new CheckExclusiveMinimum(this, data, value);
        break;
    case QStaticStringHash<'p','r','o','p','e','r','t','i','e','s'>::Hash:
        if (QString::fromLatin1("properties") == keyName)
            return new CheckProperties(this, data, value);
        break;
    case QStaticStringHash<'a','d','d','i','t','i','o','n','a','l','p','r','o','p','e','r','t','i','e','s'>::Hash:
        if (QString::fromLatin1("additionalproperties") == keyName)
            return new CheckAdditionalProperties(this, data, value);
        break;
    case QStaticStringHash<'d','e','s','c','r','i','p','t','i','o','n'>::Hash:
        if (QString::fromLatin1("description") == keyName)
            return new CheckDescription(this, data);
        break;
    case QStaticStringHash<'t','i','t','l','e'>::Hash:
        if (QString::fromLatin1("title") == keyName)
            return new CheckTitle(this, data);
        break;
    case QStaticStringHash<'m','a','x','i','t','e','m','s'>::Hash:
        if (QString::fromLatin1("maxitems") == keyName)
            return new CheckMaxItems(this, data, value);
        break;
    case QStaticStringHash<'m','i','n','i','t','e','m','s'>::Hash:
        if (QString::fromLatin1("minitems") == keyName)
            return new CheckMinItems(this, data, value);
        break;
    case QStaticStringHash<'i','t','e','m','s'>::Hash:
        if (QString::fromLatin1("items") == keyName)
            return new CheckItems(this, data, value);
        break;
    case QStaticStringHash<'a','d','d','i','t','i','o','n','a','l','i','t','e','m','s'>::Hash:
        if (QString::fromLatin1("additionalitems") == keyName)
            return new CheckAdditionalItems(this, data, value);
        break;
    case QStaticStringHash<'e','x','t','e','n','d','s'>::Hash:
        if (QString::fromLatin1("extends") == keyName)
            return new CheckExtends(this, data, value);
        break;
    case QStaticStringHash<'p','a','t','t','e','r','n'>::Hash:
        if (QString::fromLatin1("pattern") == keyName)
            return new CheckPattern(this, data, value);
        break;
    case QStaticStringHash<'m','i','n','l','e','n','g','t','h'>::Hash:
        if (QString::fromLatin1("minlength") == keyName)
            return new CheckMinLength(this, data, value);
        break;
    case QStaticStringHash<'m','a','x','l','e','n','g','t','h'>::Hash:
        if (QString::fromLatin1("maxlength") == keyName)
            return new CheckMaxLength(this, data, value);
        break;
    case QStaticStringHash<'e','n','u','m'>::Hash:
        if (QString::fromLatin1("enum") == keyName)
            return new CheckEnum(this, data, value);
        break;
    case QStaticStringHash<'f','o','r','m','a','t'>::Hash:
        if (QString::fromLatin1("format") == keyName)
            return new CheckFormat(this, data, value);
        break;
    case QStaticStringHash<'d','e','f','a','u','l','t'>::Hash:
        if (QString::fromLatin1("default") == keyName)
            return new CheckDefault(this, data, value);
        break;
    case QStaticStringHash<'d','i','v','i','s','i','b','l','e','b','y'>::Hash:
        if (QString::fromLatin1("divisibleby") == keyName)
            return new CheckDivisibleBy(this, data, value);
        break;
    case QStaticStringHash<'$','r','e','f'>::Hash:
        if (QString::fromLatin1("$ref") == keyName)
            return new CheckRef(this, data, value);
        break;
    case QStaticStringHash<'t','y','p','e'>::Hash:
        if (QString::fromLatin1("type") == keyName)
            return new CheckType(this, data, value);
        break;
    default:
//        qDebug() << "NOT FOUND"  << keyName;
        return new NullCheck(this, data);
    }

//    qDebug() << "FALLBACK"  << keyName;
//    bool  ok;
//    qCritical() << keyName << value.toString(&ok);
    return new NullCheck(this, data);
}

template<class T>
bool Schema<T>::check(const Value &value, Service *callbackToUseForCheck) const
{
    return d_ptr->check(value, callbackToUseForCheck);
}

template<class T>
bool SchemaPrivate<T>::check(const Value &value, Service *callbackToUseForCheck) const
{
    //qDebug() << Q_FUNC_INFO << m_checks.count() << this;
    Q_ASSERT(callbackToUseForCheck);
    Q_ASSERT(!m_callbacks);

    m_callbacks = callbackToUseForCheck;
    bool result = check(value);
    m_callbacks = 0;
    return result;
}

template<class T>
bool SchemaPrivate<T>::check(const Value &value) const
{
    Q_ASSERT(m_callbacks);

    m_requiredCount = 0;
    foreach (Check *check, m_checks) {
        if (!check->check(value)) {
            return false;
        }
    }
    if (m_requiredCount != m_maxRequired) {
        m_callbacks->setValidationError(QString::fromLatin1("Schema validation error: Required field is missing"));
        return false;
    }
    return true;
}

template<class T>
void SchemaPrivate<T>::checkDefault(Value &value, Object &object) const
{
    foreach (Check *check, m_checks) {
        check->checkDefault(value, object);
    }
}

template<class T>
void SchemaPrivate<T>::setLoadError(const char *message, const Value & value, int code)
{
    if (!m_bLoadError) {
        m_callbacks->setLoadError(QString::fromLatin1("Schema errors found. Schema can not be loaded properly."));
        m_bLoadError = true;
    }

    QString str;
    QDebug(&str) << value.value(); // put parameter value into the message
    str = QString::fromLatin1("Schema Error: %1 ( %2 )").arg(QString::fromLatin1(message)).arg(str);
    m_callbacks->setSubError(str, code);
}


} // namespace SchemaValidation

QT_END_HEADER

#endif // CHECKPOINTS_H

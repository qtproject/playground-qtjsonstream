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

#include "schemavalidator.h"

#include "schemamanager_p.h"

#include "jsonobjecttypes_impl_p.h"

#include "qjsondocument.h"
#include "qjsonobject.h"

#include <QFile>
#include <QDir>
#include <QCoreApplication>

#include "jsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
    \internal
*/
inline QJsonObject makeError(SchemaError::ErrorCode code, const QString &message)
{
    return SchemaError(code, message).object();
}

class SchemaValidator::SchemaValidatorPrivate
{
public:
    SchemaValidatorPrivate()
        : m_bInit(false), m_matcher(0)
    {

    }

    SchemaManager<QJsonObject, JsonObjectTypes> mSchemas;
    SchemaError mLastError;

    QRegExp m_filter;
    bool m_bInit; // filtering & indexing status (false-todo, true-done)
    QStringList m_strsFilteredSchemas;

    QSharedPointer<SchemaNameMatcher> m_matcher;
};

/*!
    \class SchemaValidator

    \brief The SchemaValidator class implements JSON Schema Validation API.
*/

/*!
     \enum SchemaValidator::SchemaNameInitialization
     \value UseFilename
         Initialize schema name (object type) from filename (without extension).
     \value UseParameter
         Use parameter as a schema name.
     \value UseProperty
         Initialize schema name (object type) from a JSON property with a specified name.
*/

/*!
    \brief The SchemaValidator class implements JSON Schema Validation API.

    Creates a new SchemaValidator with optional \a parent.
*/
SchemaValidator::SchemaValidator(QObject *parent) :
    QObject(parent), d_ptr(new SchemaValidatorPrivate)
{
}

/*!
    \internal
 */

SchemaValidator::~SchemaValidator()
{
    delete d_ptr;
}

/*!
    Returns a detailed error information about the last schema operation
*/
SchemaError SchemaValidator::getLastError() const
{
    return d_ptr->mLastError;
}

/*!
    Returns true if there are no schemas in the validator
*/
bool SchemaValidator::isEmpty() const
{
    return d_ptr->mSchemas.isEmpty();
}

/*!
    Returns the list of initialized schemas in the validator
*/
QStringList SchemaValidator::schemaNames() const
{
    return d_ptr->mSchemas.names();
}

/*!
    Returns true if this validator has a schema with the given \a name.
*/
bool SchemaValidator::hasSchema(const QString & name)
{
    return d_ptr->mSchemas.contains(name);
}

/*!
    Removes the schema with the given \a name from this validator.
*/
void SchemaValidator::removeSchema(const QString & name)
{
    d_ptr->mSchemas.take(name);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}

/*!
    Removes all the schemas from the validator
*/
void SchemaValidator::clear()
{
    d_ptr->mSchemas.clear();
}

/*!
    Schema validation filtering - limit schemas used during validation
    to schemas which name matches \a filter
*/
void SchemaValidator::setValidationFilter(const QRegExp &filter)
{
    d_ptr->m_filter = filter;
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}

/*!
    Sets a \a matcher object which does matching between json object and schema name
    without doing a full schema validation. This allows to do a validation without
    iteration through all schemas.
*/
void SchemaValidator::setSchemaNameMatcher(const SchemaNameMatcher &matcher)
{
    d_ptr->m_matcher = QSharedPointer<SchemaNameMatcher>(matcher.clone());
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}


/*!
    Supplements a validator object with data from schema files with \a ext extension
    in \a path folder.
    Schema name (object type) can be defined by the filename of the schema file or
    from \a schemaNameProperty property in JSON object.
    Returns true at success, otherwise getLastError() can be used to access
    a detailed error information.
*/
bool SchemaValidator::loadFromFolder(const QString & path, const QString & schemaNameProperty, const QByteArray & ext/*= "json"*/)
{
    Q_D(SchemaValidator);
    d->mLastError = _loadFromFolder(path, schemaNameProperty, ext);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return SchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Supplements a validator object with data from \a filename schema file, using \a type and \a schemaName.
    Returns true at success, otherwise getLastError() can be used to access
    a detailed error information.
*/
bool SchemaValidator::loadFromFile(const QString &filename, SchemaNameInitialization type, const QString & schemaName)
{
    Q_D(SchemaValidator);
    d->mLastError = _loadFromFile(filename, type, schemaName);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return SchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Supplements a validator object with data from a QByteArray \a json matching \a name and using \a type.
    Returns true at success, otherwise getLastError() can be used to access
    a detailed error information.
*/
bool SchemaValidator::loadFromData(const QByteArray & json, const QString & name, SchemaNameInitialization type)
{
    Q_D(SchemaValidator);
    d->mLastError = _loadFromData(json, name, type);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return SchemaError::NoError == d->mLastError.errorCode();
 }

/*!
    \internal
    Supplements a validator object with data from schema files with \a ext extension
    in \a path folder.
    Schema name (object type) can be defined by the filename of the schema file or
    from \a schemaNameProperty property in JSON object.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::_loadFromFolder(const QString & path, const QString & schemaNameProperty, const QByteArray & ext/*= "json"*/)
{
    QJsonObject ret;
    QDir dir(!path.isEmpty() ? path : QDir::currentPath());
    if (dir.exists())
    {
        SchemaNameInitialization type(schemaNameProperty.isEmpty() ? UseFilename : UseProperty);
        QString name(UseProperty == type ? schemaNameProperty : QString::null);
        int nLoaded = 0;

        // create a filter if required
        QStringList exts;
        if (!ext.isEmpty())
            exts.append(QStringLiteral("*.")+QString::fromLatin1(ext));

        QStringList items(dir.entryList(exts, QDir::Files | QDir::Readable));
        foreach (QString filename, items)
        {
            if (UseFilename == type)
            {
                // strip extension from a filename to create an object type
                name = ext.isEmpty() ? filename : filename.left(filename.length() - ext.length() - 1);
            }

            QJsonObject ret0 = _loadFromFile(dir.path() + QDir::separator() + filename, type, name);
            if (!ret0.isEmpty())
            {
                ret.insert(filename, ret0);
            }
            else
            {
                nLoaded++;
            }
        }

        // check result for errors
        if (!ret.isEmpty()) // loading errors
        {
            int nFailed = ret.count();
            ret.insert(SchemaError::kCodeStr, SchemaError::InvalidSchemaLoading);
            ret.insert(SchemaError::kMessageStr,
                       QString::fromLatin1("Loading failed for %1 schemas. %2 schemas are loaded successfully.").arg(nFailed).arg(nLoaded));

            if (nLoaded)
                ret.insert(SchemaError::kCounterStr, nLoaded);
        }
        else if (!nLoaded) // no schemas were found
        {
            ret = makeError(SchemaError::InvalidSchemaLoading,
                            QString::fromLatin1("Folder '%1' does not contain any schema.").arg(dir.path()));
        }
    }
    else
    {
        ret = makeError(SchemaError::InvalidSchemaFolder,
                        QString::fromLatin1("Folder '%1' does not exist.").arg(dir.path()));
    }

    if (!ret.isEmpty())
        ret.insert(SchemaError::kSourceStr, dir.path());

    return ret;
}

/*!
    \internal
    Supplements a validator object with data from \a filename schema file, using \a type and \a shemaName.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::_loadFromFile(const QString &filename, SchemaNameInitialization type, const QString & shemaName)
{
    QJsonObject ret;
    if (!filename.isEmpty())
    {
        QByteArray json;
        QFile schemaFile(QFile::exists(filename) ? filename : QDir::currentPath() + QDir::separator() + filename);
        if (schemaFile.open(QIODevice::ReadOnly) && !(json = schemaFile.readAll()).isEmpty())
        {
            schemaFile.close();

            QString name(shemaName);
            if (UseFilename == type && shemaName.isEmpty())
            {
                // strip extension from a filename to create an object type
                name = QFileInfo(schemaFile).baseName();
            }

            ret = _loadFromData(json, name, type);
        }
        else
        {   // file open error
            ret = makeError(SchemaError::FailedSchemaFileOpenRead,
                            QString::fromLatin1("Can't open/read '%1' schema file.").arg(schemaFile.fileName()));
        }

        if (!ret.isEmpty())
            ret.insert(SchemaError::kSourceStr, schemaFile.fileName());
    }
    else
    {
        ret = makeError(SchemaError::FailedSchemaFileOpenRead, QStringLiteral("Filename is empty"));
    }
    return ret;
}

/*!
    \internal
    Supplements a validator object from a QByteArray \a json matching \a name and using \a type.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::_loadFromData(const QByteArray & json, const QString & name, SchemaNameInitialization type)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject schemaObject = doc.object();

    //qDebug() << "shemaName " << name << " type= " << type;
    //qDebug() << "schemaBody " << schemaObject;

    if (doc.isNull() || schemaObject.isEmpty())
    {
        return makeError(SchemaError::InvalidObject, QStringLiteral("schema data is invalid"));
    }

    QJsonObject ret;
    QString schemaName;
    if (UseProperty == type && !name.isEmpty() && schemaObject.contains(name))
    {
        // retrive object type from JSON element
        schemaName = schemaObject[name].toString();
    }
    else if (UseProperty != type)
    {
        schemaName = name;
    }
    else if (!name.isEmpty())
    {
        // property containing schema name is absent
        return makeError(SchemaError::InvalidSchemaOperation,
                            QString::fromLatin1("name property '%1' must be present").arg(name));

    }

    if (!schemaName.isEmpty())
    {
        ret = setSchema(schemaName, schemaObject);
    }
    else
    {
        // no schema type
        ret = makeError(SchemaError::InvalidSchemaOperation,
                        QStringLiteral("schema name is missing"));
    }
    return ret;
}

/*!
    Validates \a object with \a schemaName schema.
    Returns true at success, otherwise getLastError() can be used to access
    a detailed error information.
*/
bool SchemaValidator::validateSchema(const QString &schemaName, const QJsonObject &object)
{
    Q_D(SchemaValidator);
    d->mLastError = d->mSchemas.validate(schemaName, object);
    return SchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Validates \a object with schemas in the validator.
    Returns true at success, otherwise getLastError() can be used to access
    a detailed error information.
*/
bool SchemaValidator::validateSchema(const QJsonObject &object)
{
    Q_D(SchemaValidator);
    //qDebug() << "VALIDATE: " << object;

    // do filtering & indexing initialization only once
    if (!d->m_bInit) {
        d->m_bInit = true;
        if (!d->m_filter.isEmpty())
        {
            d->m_strsFilteredSchemas = schemaNames().filter(d->m_filter);
        }

        // do indexing if required
        if (d->m_matcher && d->m_matcher->canIndex()) {
            // use filtered schemas if filter is set
            const QStringList strsSchemas(d->m_filter.isEmpty() ? schemaNames() : d->m_strsFilteredSchemas);
            QMap<QString, QJsonObject> map(d_ptr->mSchemas.schemas());
            d->m_matcher->reset();
            foreach (QString strSchema, strsSchemas) {
                QMap<QString, QJsonObject>::const_iterator it(map.find(strSchema));
                if (it != map.end())
                    d->m_matcher->createIndex(it.key(), it.value());
            }
        }
    }

    if (!d->m_matcher) {
        const QStringList strsSchemas(d->m_filter.isEmpty() ? schemaNames() : d->m_strsFilteredSchemas);

        // iterate through all schemas and find a valid one (not efficient way to do this without matcher)
        foreach (QString strSchema, strsSchemas) {
            if (validateSchema(strSchema, object)) {
                //qDebug() << "found schema: " << strSchema;
                return true;
            }
        }
    }
    else {
        // matcher allows much faster validation
        QStringList strsSchemas(d->m_matcher->getExactMatches(object));
        foreach (QString strSchema, strsSchemas) {
            if (validateSchema(strSchema, object)) {
                //qDebug() << "found schema @ ex: " << strSchema;
                return true;
            }
        }

        strsSchemas = d->m_matcher->getPossibleMatches(object);
        foreach (QString strSchema, strsSchemas) {
            if (validateSchema(strSchema, object)) {
                //qDebug() << "found schema @ pos: " << strSchema;
                return true;
            }
        }
    }
    return false;
}

/*!
  \internal
*/
QJsonObject SchemaValidator::setSchema(const QString &schemaName, QJsonObject schema)
{
    QJsonObject ret = d_ptr->mSchemas.insert(schemaName, schema);
    //qDebug() << "setSchema::errors: " << ret;
    return ret;
}

/*!
    \class SchemaValidator::SchemaNameMatcher

    \brief The SchemaNameMatcher is a base class for schema matching.
    JSON object validation requires to know an exact schema or otherewise to iterate
    through all available schema in the \c SchemaValidator validator object.
    The SchemaNameMatcher class allows to limit this iteration in order to speed up a
    validation process. \l{SchemaValidator::setSchemaNameMatcher()} should be used to set
    a matcher for the validator .

    \sa SchemaValidator::setSchemaNameMatcher()
*/

/*!
    \fn SchemaValidator::SchemaNameMatcher::SchemaNameMatcher(bool _bCanIndex)

    Constructs a SchemaNameMatcher object.
    If \a _bCanIndex is true, then the object will index schemas for faster
    matching by invoking \l{createIndex()} for each available schema.

    \sa createIndex()
*/

/*!
    \fn SchemaValidator::SchemaNameMatcher::~SchemaNameMatcher()

    Deletes the \c SchemaNameMatcher object
*/

/*!
    \fn SchemaNameMatcher *SchemaValidator::SchemaNameMatcher::clone() const

    Creates a copy of the \c SchemaNameMatcher object.
*/

/*!
    \fn bool SchemaValidator::SchemaNameMatcher::canIndex() const

    Returns true if the object can index schemas for faster
    matching by invoking \l{createIndex()} for each available schema.

    \sa createIndex()
*/

/*!
    \fn void SchemaValidator::SchemaNameMatcher::createIndex(const QString &schemaName, const QJsonObject &schema)

    Creates an index for a \a schema named \a schemaName to do a quicker name matching
*/

/*!
    \fn QStringList SchemaValidator::SchemaNameMatcher::getExactMatches(const QJsonObject &object)

    Returns a list of names for the schemas that exactly match the specified \a object and can be used
    for it's validation. Knowing exact schema name allows to skip schema iteration and validate with
    some schemas only.

    \sa getPossibleMatches()
*/

/*!
    \fn QStringList SchemaValidator::SchemaNameMatcher::getPossibleMatches(const QJsonObject &object)

    Returns a list of names for the schemas that probably match the specified \a object and can be used
    for it's validation after \l{getExactMatches()} validation fail.

    \sa getExactMatches()
*/

/*!
    \fn void SchemaValidator::SchemaNameMatcher::reset()

    Resets the object to initial state. This method is called every time a schema is added or removed
    from SchemaValidator object.
*/

/*!
    \class SchemaValidator::SchemaPropertyNameMatcher

    \brief The SchemaPropertyNameMatcher class implements a name matcher for a case when
    schema name is defined by some property in JSON object
*/

/*!
    \fn SchemaValidator::SchemaPropertyNameMatcher::SchemaPropertyNameMatcher(const QString & property)

    Constructs a SchemaPropertyNameMatcher object where \a property defines a schema name in JSON object.
*/

/*!
    \fn SchemaNameMatcher *SchemaValidator::SchemaPropertyNameMatcher::clone() const

    Creates a copy of the \c SchemaPropertyNameMatcher object.
*/

/*!
    \fn QStringList SchemaValidator::SchemaPropertyNameMatcher::getExactMatches(const QJsonObject &object)

    Returns a list of names for the schemas that exactly match the specified \a object and can be used
    for it's validation.
*/

/*!
    \class SchemaValidator::SchemaUniqueKeyNameMatcher

    \brief The SchemaUniqueKeyNameMatcher class implements a name matcher when
    schema contains a uniquely defined top-level key/property that can be used as a quick index
*/

class SchemaValidator::SchemaUniqueKeyNameMatcher::SchemaUniqueKeyNameMatcherPrivate
{
public:
    QHash<QString,QStringList> m_items;
    QStringList m_others;
};

/*!
    Constructs a SchemaUniqueKeyNameMatcher object where \a key defines a top-level key/property
    that can be used as a quick index.
*/

SchemaValidator::SchemaUniqueKeyNameMatcher::SchemaUniqueKeyNameMatcher(const QString & key)
    : SchemaNameMatcher(true)
    , m_key(key)
    , d_ptr(new SchemaUniqueKeyNameMatcherPrivate)
{
}

/*!
    Deletes the \c SchemaUniqueKeyNameMatcher object
*/

SchemaValidator::SchemaUniqueKeyNameMatcher::~SchemaUniqueKeyNameMatcher()
{
    if (d_ptr)
        delete d_ptr;
}

/*!
    \fn SchemaNameMatcher *SchemaValidator::SchemaUniqueKeyNameMatcher::clone() const

    Creates a copy of the \c SchemaUniqueKeyNameMatcher object.
*/

/*!
  Creates an index for a \a schema named \a schemaName to do a quicker name matching
*/

void SchemaValidator::SchemaUniqueKeyNameMatcher::createIndex(const QString &schemaName, const QJsonObject & schema)
{
    if (schema.contains(QStringLiteral("properties"))) {
        QJsonValue props = schema[QStringLiteral("properties")];
        if (props.isObject() && props.toObject().contains(m_key)) {
            QJsonObject o = props.toObject()[m_key].toObject();
            if (o.contains(QStringLiteral("required")) &&
                o.contains(QStringLiteral("type")) &&
                o.contains(QStringLiteral("pattern"))) {
                if (true == o[QStringLiteral("required")].toBool() &&
                    o[QStringLiteral("type")].toString() == QStringLiteral("string")) {
                    QString key = o[QStringLiteral("pattern")].toString();
                    if (!key.isEmpty())
                    {
                        QHash<QString,QStringList>::iterator it(d_ptr->m_items.find(key));
                        if (it != d_ptr->m_items.end())
                            (*it).append(schemaName);
                        else
                            d_ptr->m_items.insert(key, QStringList() << schemaName);
                        return;
                    }
                }
            }
        }
    }
    d_ptr->m_others.append(schemaName);
}

/*!
    Returns a list of names for the schemas that exactly match the specified \a object and can be used
    for it's validation.

    \sa getPossibleMatches()
*/

QStringList SchemaValidator::SchemaUniqueKeyNameMatcher::getExactMatches(const QJsonObject &object)
{
    QString str(!m_key.isEmpty() && object.contains(m_key) ? object[m_key].toString() : QString::null);
    QHash<QString,QStringList>::const_iterator it;
    return !str.isEmpty() && (it = d_ptr->m_items.find(str)) != d_ptr->m_items.end() ? *it : QStringList();
}

/*!
    Returns a list of names for the schemas that probably match the specified \a object and can be used
    for it's validation after \l{getExactMatches()} validation fail.

    \sa getExactMatches()
*/

QStringList SchemaValidator::SchemaUniqueKeyNameMatcher::getPossibleMatches(const QJsonObject &object)
{
    Q_UNUSED(object);
    return d_ptr->m_others;
}

/*!
    Resets the object to initial state. This method is called every time a schema is added or removed
    from SchemaValidator object.
*/

void SchemaValidator::SchemaUniqueKeyNameMatcher::reset()
{
    d_ptr->m_items.clear();
    d_ptr->m_others.clear();
}

#include "moc_schemavalidator.cpp"

QT_END_NAMESPACE_JSONSTREAM

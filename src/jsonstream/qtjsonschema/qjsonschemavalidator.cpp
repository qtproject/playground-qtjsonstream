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

#include "qjsonschemavalidator.h"

#include "schemamanager_p.h"

#include "jsonobjecttypes_impl_p.h"

#include "qjsondocument.h"
#include "qjsonobject.h"

#include <QFile>
#include <QDir>
#include <QCoreApplication>

#include "qjsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
    \internal
*/
inline QJsonObject makeError(QJsonSchemaError::ErrorCode code, const QString &message)
{
    return QJsonSchemaError(code, message).object();
}

class QJsonSchemaValidator::QJsonSchemaValidatorPrivate
{
public:
    QJsonSchemaValidatorPrivate()
        : m_bInit(false), m_matcher(0)
    {

    }

    SchemaManager<QJsonObject, JsonObjectTypes> mSchemas;
    QJsonSchemaError mLastError;

    QRegExp m_filter;
    bool m_bInit; // filtering & indexing status (false-todo, true-done)
    QStringList m_strsFilteredSchemas;

    QSharedPointer<SchemaNameMatcher> m_matcher;
};

/*!
    \class QJsonSchemaValidator

    \brief The QJsonSchemaValidator class validates JSON objects against JSON schemas

    QJsonSchemaValidator validates JSON objects against JSON schemas.  Schemas are loaded
    individually from data (loadFromData()), from a file (loadFromFile())
    or collectively (loadFromFolder()).  Each schema is given a name, which is either
    specified directly when loading, or derived from the filename of the schema file.

    JSON objects are validated against either a specific named schema or against all
    schemas.  When validating against all schemas, you can restrict the schemas
    that are checked by calling setValidationFilter().  Also, you can greatly speed up
    validation by providing a SchemaNameMatcher object by calling setSchemaNameMatcher().
    A SchemaNameMatcher quickly reduces the number of schemas that need to be checked,
    so that most of the time only one schema is checked for a valid object.

    Schema loading and validation methods all return a boolean value indicating
    whether the operation succeeded.  In the case of failure, call getLastError()
    to retrieve an object describing the error that occurred.
*/

/*!
     \enum QJsonSchemaValidator::SchemaNameInitialization

     Specifies how the schema loading methods should name each loaded schema.

     \value UseFilename
         Use the file's baseName (file name without extension) as the schema name.
     \value UseParameter
         Interpret the provided string parameter as the schema name.
     \value UseProperty
         Interpret the provided string parameter as a property name.  The value of
         this property in the schema object will be used as the schema name.
*/

/*!
    Creates a new QJsonSchemaValidator with optional \a parent.
*/
QJsonSchemaValidator::QJsonSchemaValidator(QObject *parent) :
    QObject(parent), d_ptr(new QJsonSchemaValidatorPrivate)
{
}

/*!
    \internal
 */

QJsonSchemaValidator::~QJsonSchemaValidator()
{
    delete d_ptr;
}

/*!
    Returns an error information object describing any errors encountered during
    the last schema operation.

    \sa loadFromData(), loadFromFile(), loadFromFolder(), validateSchema()
*/
QJsonSchemaError QJsonSchemaValidator::getLastError() const
{
    return d_ptr->mLastError;
}

/*!
    Returns true if no schemas have been loaded into the validator, false otherwise.
*/
bool QJsonSchemaValidator::isEmpty() const
{
    return d_ptr->mSchemas.isEmpty();
}

/*!
    Returns a list of names of schemas which have been loaded into the validator.
*/
QStringList QJsonSchemaValidator::schemaNames() const
{
    return d_ptr->mSchemas.names();
}

/*!
    Returns true if this validator has a schema with the given \a name.
*/
bool QJsonSchemaValidator::hasSchema(const QString & name)
{
    return d_ptr->mSchemas.contains(name);
}

/*!
    Removes the schema with the given \a name from this validator.
*/
void QJsonSchemaValidator::removeSchema(const QString & name)
{
    d_ptr->mSchemas.take(name);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}

/*!
    Removes all schemas from the validator.
*/
void QJsonSchemaValidator::clear()
{
    d_ptr->mSchemas.clear();
}

/*!
    Limits the schemas which will be used when validating an object to those
    whose name matches \a filter.  To use all schemas, set an empty filter.
*/
void QJsonSchemaValidator::setValidationFilter(const QRegExp &filter)
{
    d_ptr->m_filter = filter;
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}

/*!
    Sets \a matcher as a schema name matcher object that will be used when validating
    JSON objects. A SchemaNameMatcher is designed to quickly reduce the number of
    schemas that need to be checked, so that most of the time only one schema is checked
    for a valid object.
*/
void QJsonSchemaValidator::setSchemaNameMatcher(const SchemaNameMatcher &matcher)
{
    d_ptr->m_matcher = QSharedPointer<SchemaNameMatcher>(matcher.clone());
    d_ptr->m_bInit = false; // clear last filtering & indexing results
}


/*!
    Load schemas from files in folder specified by \a path.  The files may be restricted to those
    with extension \a ext.  If \a schemaNameProperty is not empty, it will be used
    to determine each schema's name, otherwise each file's basename will be used to name
    the schemas (see SchemaNameInitialization for details).

    Returns true if all schema files were loaded successfully, or false otherwise.  Schema
    loading error information can be retrieved using getLastError().
*/
bool QJsonSchemaValidator::loadFromFolder(const QString & path, const QString & schemaNameProperty, const QByteArray & ext/*= "json"*/)
{
    Q_D(QJsonSchemaValidator);
    d->mLastError = _loadFromFolder(path, schemaNameProperty, ext);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return QJsonSchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Load a schema from file \a filename.  If \a type is UseFilename, the schema name will be
    set to the file's baseName.  If \a type is UseParameter, \a schemaName is used as the
    the schema name.  If \a type is UseProperty, \a schemaName is interpreted as a property
    name which will be used to extract the schema name from the schema.
    See SchemaNameInitialization for more details.

    Returns true if the schema file was loaded successfully, or false otherwise.  Schema
    loading error information can be retrieved using getLastError().
*/
bool QJsonSchemaValidator::loadFromFile(const QString &filename, SchemaNameInitialization type, const QString & schemaName)
{
    Q_D(QJsonSchemaValidator);
    d->mLastError = _loadFromFile(filename, type, schemaName);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return QJsonSchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Load a schema from \a json.  If \a type is UseParameter, \a name is used as the
    the schema name.  If \a type is UseProperty, \a name is interpreted as a property
    name which will be used to extract the schema name from the schema.
    See SchemaNameInitialization for more details.

    Returns true if the schema was loaded successfully, or false otherwise.  Schema
    loading error information can be retrieved using getLastError().
*/
bool QJsonSchemaValidator::loadFromData(const QByteArray & json, const QString & name, SchemaNameInitialization type)
{
    Q_D(QJsonSchemaValidator);
    d->mLastError = _loadFromData(json, name, type);
    d_ptr->m_bInit = false; // clear last filtering & indexing results
    return QJsonSchemaError::NoError == d->mLastError.errorCode();
 }

/*!
    \internal
    Supplements a validator object with data from schema files with \a ext extension
    in \a path folder.
    Schema name (object type) can be defined by the filename of the schema file or
    from \a schemaNameProperty property in JSON object.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject QJsonSchemaValidator::_loadFromFolder(const QString & path, const QString & schemaNameProperty, const QByteArray & ext/*= "json"*/)
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
            ret.insert(QJsonSchemaError::kCodeStr, QJsonSchemaError::InvalidSchemaLoading);
            ret.insert(QJsonSchemaError::kMessageStr,
                       QString::fromLatin1("Loading failed for %1 schemas. %2 schemas are loaded successfully.").arg(nFailed).arg(nLoaded));

            if (nLoaded)
                ret.insert(QJsonSchemaError::kCounterStr, nLoaded);
        }
        else if (!nLoaded) // no schemas were found
        {
            ret = makeError(QJsonSchemaError::InvalidSchemaLoading,
                            QString::fromLatin1("Folder '%1' does not contain any schema.").arg(dir.path()));
        }
    }
    else
    {
        ret = makeError(QJsonSchemaError::InvalidSchemaFolder,
                        QString::fromLatin1("Folder '%1' does not exist.").arg(dir.path()));
    }

    if (!ret.isEmpty())
        ret.insert(QJsonSchemaError::kSourceStr, dir.path());

    return ret;
}

/*!
    \internal
    Supplements a validator object with data from \a filename schema file, using \a type and \a schemaName.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject QJsonSchemaValidator::_loadFromFile(const QString &filename, SchemaNameInitialization type, const QString & shemaName)
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
            ret = makeError(QJsonSchemaError::FailedSchemaFileOpenRead,
                            QString::fromLatin1("Can't open/read '%1' schema file.").arg(schemaFile.fileName()));
        }

        if (!ret.isEmpty())
            ret.insert(QJsonSchemaError::kSourceStr, schemaFile.fileName());
    }
    else
    {
        ret = makeError(QJsonSchemaError::FailedSchemaFileOpenRead, QStringLiteral("Filename is empty"));
    }
    return ret;
}

/*!
    \internal
    Supplements a validator object from a QByteArray \a json matching \a name and using \a type.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject QJsonSchemaValidator::_loadFromData(const QByteArray & json, const QString & name, SchemaNameInitialization type)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err );
    if (QJsonParseError::NoError != err.error)
    {
        // calculate line and position from file offset
        int nLine = 0, nPos = 1;
        if (err.offset > 0) {
            QByteArray b = QByteArray::fromRawData(json, err.offset);
            nLine = b.count('\n');
            nPos = err.offset - b.lastIndexOf('\n');
            if (nPos > 0)
                --nPos;
        }
        ++nLine;

        QString str;
        str = QString::fromLatin1("JSON syntax error %1 in line %2 position %3").arg(err.error).arg(nLine).arg(nPos);
        return makeError(QJsonSchemaError::InvalidObject, str);
    }

    QJsonObject schemaObject = doc.object();

    //qDebug() << "shemaName " << name << " type= " << type;
    //qDebug() << "schemaBody " << schemaObject;

    if (doc.isNull() || schemaObject.isEmpty())
    {
        return makeError(QJsonSchemaError::InvalidObject, QStringLiteral("schema data can not be empty"));
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
        return makeError(QJsonSchemaError::InvalidSchemaOperation,
                            QString::fromLatin1("name property '%1' must be present").arg(name));

    }

    if (!schemaName.isEmpty())
    {
        ret = setSchema(schemaName, schemaObject);
    }
    else
    {
        // no schema type
        ret = makeError(QJsonSchemaError::InvalidSchemaOperation,
                        QStringLiteral("schema name is missing"));
    }
    return ret;
}

/*!
    Validates \a object using the schema with name \a schemaName.
    Returns true if the object was validated successfully, or false otherwise.
    Validation error information can be retrieved using getLastError().
*/
bool QJsonSchemaValidator::validateSchema(const QString &schemaName, const QJsonObject &object)
{
    Q_D(QJsonSchemaValidator);
    d->mLastError = d->mSchemas.validate(schemaName, object);
    return QJsonSchemaError::NoError == d->mLastError.errorCode();
}

/*!
    Validates \a object using all matching schemas in the validator.
    Returns true if the object was validated successfully, or false otherwise.
    Validation error information can be retrieved using getLastError().
*/
bool QJsonSchemaValidator::validateSchema(const QJsonObject &object)
{
    Q_D(QJsonSchemaValidator);
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
QJsonObject QJsonSchemaValidator::setSchema(const QString &schemaName, QJsonObject schema)
{
    QJsonObject ret = d_ptr->mSchemas.insert(schemaName, schema);
    //qDebug() << "setSchema::errors: " << ret;
    return ret;
}

/*!
    \class QJsonSchemaValidator::SchemaNameMatcher
    \brief The SchemaNameMatcher class is an abstract class that provides schema matching
    functionality to a QJsonSchemaValidator object.

    Without schema matching, validating a JSON object involves possibly checking
    every single schema until one matches.  Schema matching allows the validator to
    reduce the number of possible schema checks.  The getExactMatches() method returns
    names schemas that definitely match a given JSON object (based on the criteria
    that a particular subclass is trying to enforce).  These schemas will be checked
    first, and if the object is successfully validated by one of these schemas, validation
    is successful.  If no exact match was validated, the getPossibleMatches() method
    returns any other schemas that may match a given JSON object.  These schemas will
    then be checked.

    \sa QJsonSchemaValidator::setSchemaNameMatcher()
*/

/*!
    \fn QJsonSchemaValidator::SchemaNameMatcher::SchemaNameMatcher(bool _bCanIndex)

    Constructs a SchemaNameMatcher object.
    \a _bCanIndex specifies whether this matcher object can index schemas for faster
    matching.  In practice, this means that createIndex() will be called each time
    a new schema is loaded into the validator.

    \sa createIndex()
*/

/*!
    \fn QJsonSchemaValidator::SchemaNameMatcher::~SchemaNameMatcher()

    Deletes the \c SchemaNameMatcher object
*/

/*!
    \fn SchemaNameMatcher *QJsonSchemaValidator::SchemaNameMatcher::clone() const

    Creates a copy of the \c SchemaNameMatcher object.
*/

/*!
    \fn bool QJsonSchemaValidator::SchemaNameMatcher::canIndex() const

    Returns true if the schema matcher can index schemas for faster
    matching by invoking createIndex() for each available schema.

    \sa createIndex()
*/

/*!
    \fn void QJsonSchemaValidator::SchemaNameMatcher::createIndex(const QString &schemaName, const QJsonObject &schema)

    This method should create an internal index for a \a schema named \a schemaName
    so that later calls to getExactMatches() and getPossibleMatches() can complete faster.
*/

/*!
    \fn QStringList QJsonSchemaValidator::SchemaNameMatcher::getExactMatches(const QJsonObject &object)

    Returns a list of names for the schemas that exactly match the specified \a object and can be used
    for its validation. Knowing exact schema name allows the QJsonSchemaValidator to optimize validation
    by checking the most likely matching schemas first.

    \sa getPossibleMatches()
*/

/*!
    \fn QStringList QJsonSchemaValidator::SchemaNameMatcher::getPossibleMatches(const QJsonObject &object)

    Returns a list of names for the schemas that could match the specified \a object and can be used
    for its validation if validation using exact schema matches fails.

    \sa getExactMatches()
*/

/*!
    \fn void QJsonSchemaValidator::SchemaNameMatcher::reset()

    Resets the object to initial state. This method is called every time a schema is added or removed
    from QJsonSchemaValidator object.
*/

/*!
    \class QJsonSchemaValidator::SchemaPropertyNameMatcher

    \brief The SchemaPropertyNameMatcher class implements a name matcher for the case when
    a property in the JSON object contains the name of a schema that should be used to
    validate the object.

    For example, the JSON object might have a "type" property that specifies the name
    of the schema to use:

    \code
    {
      "type": "rectangle",
      "width": 10,
      "height": 20
    }
    \endcode

    A SchemaPropertyNameMatcher object would then be created like this:

    \code
    SchemaPropertyNameMatcher matcher(QString("type"));
    \endcode
*/

/*!
    \fn QJsonSchemaValidator::SchemaPropertyNameMatcher::SchemaPropertyNameMatcher(const QString & property)

    Constructs a SchemaPropertyNameMatcher object where \a property specifies a property
    in a JSON object that contains a schema name that should be used to validate the object.
*/

/*!
    \fn SchemaNameMatcher *QJsonSchemaValidator::SchemaPropertyNameMatcher::clone() const

    Creates a copy of the \c SchemaPropertyNameMatcher object.
*/

/*!
    \fn QStringList QJsonSchemaValidator::SchemaPropertyNameMatcher::getExactMatches(const QJsonObject &object)

    Returns a list of names for the schemas that exactly match the specified \a object and can be used
    for its validation. Knowing exact schema name allows the QJsonSchemaValidator to optimize validation
    by checking the most likely matching schemas first.
*/

/*!
    \class QJsonSchemaValidator::SchemaUniqueKeyNameMatcher

    \brief The SchemaUniqueKeyNameMatcher class implements a schema name matcher for
    the case where schemas contains a uniquely defined top-level key/property pairs
    that can be used as a quick index for exact matching schemas.

    For example, a schema might look like this:

    \code
    {
      "title": "My Schema",
      "type": "object",
      "properties": {
        "objectType": {
          "type": "string",
          "required": true,
          "pattern": "objectTypeNumberOne"
        },

        [other properties]
      }
    \endcode

    Because the "objectType" property is required and has an explicit pattern, it is
    considered unique and the existence of this property/value pair in a JSON object
    will cause an exact match.
*/

class QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::SchemaUniqueKeyNameMatcherPrivate
{
public:
    QHash<QString,QStringList> m_items;
    QStringList m_others;
};

/*!
    Constructs a SchemaUniqueKeyNameMatcher object for the case where schemas contains
    a uniquely defined top-level \a key /property pairs that can be used as a quick index
    for exact matching schemas.
*/

QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::SchemaUniqueKeyNameMatcher(const QString & key)
    : SchemaNameMatcher(true)
    , m_key(key)
    , d_ptr(new SchemaUniqueKeyNameMatcherPrivate)
{
}

/*!
    Deletes the \c SchemaUniqueKeyNameMatcher object
*/

QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::~SchemaUniqueKeyNameMatcher()
{
    if (d_ptr)
        delete d_ptr;
}

/*!
    \fn SchemaNameMatcher *QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::clone() const

    Creates a copy of the \c SchemaUniqueKeyNameMatcher object.
*/

/*!
  Creates an index for a \a schema named \a schemaName so later calls to
  getExactMatches() and getPossibleMatches() can complete faster.
*/

void QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::createIndex(const QString &schemaName, const QJsonObject & schema)
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
    for its validation. Knowing exact schema name allows the QJsonSchemaValidator to optimize validation
    by checking the most likely matching schemas first.

    \sa getPossibleMatches()
*/

QStringList QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::getExactMatches(const QJsonObject &object)
{
    QString str(!m_key.isEmpty() && object.contains(m_key) ? object[m_key].toString() : QString::null);
    QHash<QString,QStringList>::const_iterator it;
    return !str.isEmpty() && (it = d_ptr->m_items.find(str)) != d_ptr->m_items.end() ? *it : QStringList();
}

/*!
    Returns a list of names for the schemas that could match the specified \a object and can be used
    for its validation if validation using exact schema matches fails.

    \sa getExactMatches()
*/

QStringList QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::getPossibleMatches(const QJsonObject &object)
{
    Q_UNUSED(object);
    return d_ptr->m_others;
}

/*!
    Resets the object to initial state. This method is called every time a schema is added or removed
    from QJsonSchemaValidator object.
*/

void QJsonSchemaValidator::SchemaUniqueKeyNameMatcher::reset()
{
    d_ptr->m_items.clear();
    d_ptr->m_others.clear();
}

#include "moc_qjsonschemavalidator.cpp"

QT_END_NAMESPACE_JSONSTREAM

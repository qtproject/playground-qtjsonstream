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

class SchemaValidator::SchemaValidatorPrivate
{
public:
    SchemaManager<QJsonObject, JsonObjectTypes> mSchemas;
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
     \value UseElement
         Initialize schema name (object type) from a JSON element with a specified name.
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
    Initializes a validator object with data from schema files with \a ext extension
    in \a path folder.
    Schema name (object type) can be defined by the filename of the schema file or
    from \a schemaNameElement element in JSON object.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::initializeFromFolder(const QString & path, const QString & schemaNameElement, const QByteArray & ext/*= "json"*/)
{
    QJsonObject ret;
    QDir dir(!path.isEmpty() ? path : QDir::currentPath());
    if (dir.exists())
    {
        SchemaNameInitialization type(schemaNameElement.isEmpty() ? UseFilename : UseElement);
        QString name(UseElement == type ? schemaNameElement : QString::null);

        // create a filter if required
        QStringList exts;
        if (!ext.isEmpty())
            exts << QByteArray("*." + ext);

        QStringList items(dir.entryList(exts, QDir::Files | QDir::Readable));
        foreach (QString filename, items)
        {
            if (UseFilename == type)
            {
                // strip extension from a filename to create an object type
                name = ext.isEmpty() ? filename : filename.left(filename.length() - ext.length() - 1);
            }

            QJsonObject ret0 = initializeFromFile(dir.path() + QDir::separator() + filename, type, name);
            if (!ret0.isEmpty())
            {
                ret.insert(filename, ret0);
            }
        }
    }
    return ret;
}

/*!
    Initializes a validator object with data from \a filename schema file, using \a type and \a shemaName.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::initializeFromFile(const QString &filename, SchemaNameInitialization type, const QString & shemaName)
{
    QJsonObject ret;
    if (!filename.isEmpty())
    {
        QFile schemaFile(QFile::exists(filename) ? filename : QDir::currentPath() + QDir::separator() + filename);
        if (schemaFile.open(QIODevice::ReadOnly))
        {
            QByteArray json = schemaFile.readAll();
            schemaFile.close();

            QString name(shemaName);
            if (UseFilename == type && shemaName.isEmpty())
            {
                // strip extension from a filename to create an object type
                name = QFileInfo(schemaFile).baseName();
            }

            ret = initializeFromData(json, name, type);
        }
        else
        {
            // file open error
        }
    }
    return ret;
}

/*!
    Initializes a validator object from a QByteArray \a json matching \a name and using \a type.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::initializeFromData(const QByteArray & json, const QString & name, SchemaNameInitialization type)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject schemaObject = doc.object();

    //qDebug() << "shemaName " << name << " type= " << type;
    //qDebug() << "schemaBody " << schemaObject;

    QJsonObject ret;
    QString schemaName;
    if (UseElement == type  && schemaObject.contains(name))
    {
        // retrive object type from JSON element
        // don't need this element any more (?)
        schemaName = schemaObject.take(name).toString();
    }
    else if (UseElement != type)
    {
        schemaName = name;
    }

    if (!schemaName.isEmpty())
    {
        ret = setSchema(schemaName, schemaObject);
    }
    else
    {
        // no schema type
    }
    return ret;
}

/*!
    Validates \a object with \a schemaName schema.
    Returns empty variant map at success or a map filled with error information otherwise
*/
QJsonObject SchemaValidator::validateSchema(const QString &schemaName, QJsonObject object)
{
    return d_ptr->mSchemas.validate(schemaName, object);
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

#include "moc_schemavalidator.cpp"

QT_END_NAMESPACE_JSONSTREAM

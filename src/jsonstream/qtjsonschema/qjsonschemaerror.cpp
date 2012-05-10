/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjsonschemaerror.h"

#include <QStringList>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

const QString QJsonSchemaError::kCodeStr    = QString::fromLatin1("::code");
const QString QJsonSchemaError::kMessageStr = QString::fromLatin1("::message");
const QString QJsonSchemaError::kSourceStr = QString::fromLatin1("::source");
const QString QJsonSchemaError::kCounterStr = QString::fromLatin1("::count");
const QString QJsonSchemaError::kErrorPrefixStr = QString::fromLatin1("::");

/*!
    \class QJsonSchemaError
    \brief The QJsonSchemaError class details error conditions from parsing or
    validating JSON objects and schemas.

    The error object includes a code, human-readable message, and the JSON object
    that caused the error.  It may also include the source file/path of the
    offending object.

    In the case of loading multiple schema files from a folder, there may be
    multiple errors.  In this case, each individual error can be found in
    the subErrors() list.

    \sa QJsonSchemaError::ErrorCode
 */

/*!
     \enum QJsonSchemaError::ErrorCode
     \omitvalue NoError
     \value FailedSchemaValidation
         JSON object failed schema validation
     \value InvalidSchemaOperation
         Error somewhere in the schema itself?
     \value InvalidObject
         The JSON object/schema  could not be parsed.
     \value FailedSchemaFileOpenRead
         Schema file could not be opened or read.
     \value InvalidSchemaFolder
         Schema folder does not exist
     \value InvalidSchemaLoading
         Schema loading errors
     \value SchemaWrongParamType
         Schema attribute has an invalid type
     \value SchemaWrongParamValue
         Schema attribute has an invalid value
 */

/*!
  \fn QJsonSchemaError::QJsonSchemaError()

  Creates an empty schema error object.
*/

/*!
  \fn QJsonSchemaError::QJsonSchemaError(const QJsonObject & data)

  Creates an schema error object pertaining to the JSON object \a data.
*/

/*!
  \fn QJsonObject QJsonSchemaError::object() const

  Returns the JSON object that the error pertains to.  If the error is
  the result of an attempt to load a schema, the object will be the schema object.
*/

/*!
  Creates QJsonSchemaError object with specified error \a code and \a message.
*/
QJsonSchemaError::QJsonSchemaError(ErrorCode code, const QString & message)
{
    m_data.insert(kCodeStr, code);
    m_data.insert(kMessageStr, message);
}

/*!
  Returns an error code of the last schema error.
*/
QJsonSchemaError::ErrorCode QJsonSchemaError::errorCode() const
{
    return m_data.isEmpty() ? QJsonSchemaError::NoError : (QJsonSchemaError::ErrorCode)(m_data.value(kCodeStr).toDouble());
}

/*!
  Returns a human readable description of the last schema error.
*/
QString QJsonSchemaError::errorString() const
{
    return m_data.isEmpty() ? QString() : m_data.value(kMessageStr).toString();
}

/*!
  Returns the source of the last schema error.  This is either a filename
  or directory path.  In the case where a schema or object is loaded directly from
  data, this value will be empty.
*/
QString QJsonSchemaError::errorSource() const
{
    return m_data.isEmpty() ? QString() : m_data.value(kSourceStr).toString();
}

/*!
  Returns a list of sub errors.
*/
QList<QJsonSchemaError> QJsonSchemaError::subErrors() const
{
    QList<QJsonSchemaError> errors;
    foreach (QString key, m_data.keys()) {
        if (!key.startsWith(kErrorPrefixStr)) {
            QJsonObject object(m_data[key].toObject());
            if (object.contains(kCodeStr))
                errors.append(QJsonSchemaError(object));
        }
    }

    return errors;
}

QDebug operator <<(QDebug dbg, const QJsonSchemaError &e)
{
    dbg << "SchemaError(" << e.m_data << ")";
    return dbg;
}

QT_END_NAMESPACE_JSONSTREAM

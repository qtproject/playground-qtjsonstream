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

#include "schemaerror.h"

#include <QStringList>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONSTREAM

const QString SchemaError::kCodeStr    = QString::fromLatin1("::code");
const QString SchemaError::kMessageStr = QString::fromLatin1("::message");
const QString SchemaError::kSourceStr = QString::fromLatin1("::source");
const QString SchemaError::kCounterStr = QString::fromLatin1("::count");
const QString SchemaError::kErrorPrefixStr = QString::fromLatin1("::");

/*!
    \class SchemaError
    \brief The SchemaError class lists possible error codes.
    \sa SchemaError::ErrorCode
 */

/*!
     \enum SchemaError::ErrorCode
     \omitvalue NoError
     \value FailedSchemaValidation
         Object to be created/updated was invalid according to the schema.
     \value InvalidSchemaOperation
         Error somewhere in the schema itself?
     \value InvalidObject
         Unable to parse an incoming object
     \value FailedSchemaFileOpenRead
         Schema file could not be opened or read from
     \value InvalidSchemaFolder
         Schema folder does not exist
     \value InvalidSchemaLoading
         Schema loading errors
 */

/*!
  Creates SchemaError object with specified error code and message.
*/
SchemaError::SchemaError(ErrorCode code, const QString & message)
{
    m_data.insert(kCodeStr, code);
    m_data.insert(kMessageStr, message);
}

/*!
  Returns an error code of the last schema error.
*/
SchemaError::ErrorCode SchemaError::errorCode() const
{
    return m_data.isEmpty() ? SchemaError::NoError : (SchemaError::ErrorCode)(m_data.value(kCodeStr).toDouble());
}

/*!
  Returns a human readable description of the last schema error.
*/
QString SchemaError::errorString() const
{
    return m_data.isEmpty() ? QString() : m_data.value(kMessageStr).toString();
}

/*!
  Returns a source of the last schema error.
*/
QString SchemaError::errorSource() const
{
    return m_data.isEmpty() ? QString() : m_data.value(kSourceStr).toString();
}

/*!
  Returns a list of sub errors.
*/
QList<SchemaError> SchemaError::subErrors() const
{
    QList<SchemaError> errors;
    foreach (QString key, m_data.keys()) {
        if (!key.startsWith(kErrorPrefixStr)) {
            QJsonObject object(m_data[key].toObject());
            if (object.contains(kCodeStr))
                errors.append(SchemaError(object));
        }
    }

    return errors;
}

QDebug operator <<(QDebug dbg, const SchemaError &e)
{
    dbg << "SchemaError(" << e.m_data << ")";
    return dbg;
}

QT_END_NAMESPACE_JSONSTREAM

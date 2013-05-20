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

#ifndef SCHEMAERROR_H
#define SCHEMAERROR_H

#include "qjsonschema-global.h"

#include <QJsonObject>
#include <QList>

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT QJsonSchemaError
{
public:
    enum ErrorCode {
        NoError = 0,
        FailedSchemaValidation, // Invalid according to the schema
        InvalidSchemaOperation,
        InvalidObject,              // Unable to parse an incoming object
        FailedSchemaFileOpenRead,   // Schema file could not be opened or read from
        InvalidSchemaFolder,        // Schema folder does not exist
        InvalidSchemaLoading,       // Schema loading errors

        // Schema Errors
        SchemaWrongParamType = 100,
        SchemaWrongParamValue
    };

    QJsonSchemaError() {}
    QJsonSchemaError(const QJsonObject & _data) : m_data(_data) {}
    QJsonSchemaError(ErrorCode, const QString &);

    ErrorCode errorCode() const;
    QString errorString() const;
    QString errorSource() const;

    QList<QJsonSchemaError> subErrors() const;

    QJsonObject object() const { return m_data; }

    static const QString kCodeStr;
    static const QString kMessageStr;
    static const QString kSourceStr;
    static const QString kCounterStr;
    static const QString kErrorPrefixStr;

private:
    friend Q_ADDON_JSONSTREAM_EXPORT QDebug operator<<(QDebug, const QJsonSchemaError &);

    QJsonObject m_data;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAERROR_H

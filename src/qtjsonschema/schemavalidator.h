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

#ifndef SCHEMAVALIDATOR_H
#define SCHEMAVALIDATOR_H

#include <QObject>
#include <QJsonObject>

#include "jsonschema-global.h"
#include "schemaerror.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT SchemaValidator : public QObject
{
    Q_OBJECT
public:
    explicit SchemaValidator(QObject *parent = 0);
    ~SchemaValidator();

    enum SchemaNameInitialization
    {
        UseFilename = 0,
        UseParameter,
        UseProperty
    };

    bool loadFromFile(const QString &, SchemaNameInitialization = UseFilename, const QString & = QString::null);
    bool loadFromFolder(const QString &, const QString & = QString::null, const QByteArray & ext = "json");
    bool loadFromData(const QByteArray &, const QString &, SchemaNameInitialization = UseParameter);

    bool validateSchema(const QString &schemaName, const QJsonObject &object);
    bool validateSchema(const QJsonObject &object);

    SchemaError getLastError() const;

    bool isEmpty() const;
    QStringList schemaNames() const;
    bool hasSchema(const QString &);
    void removeSchema(const QString &);
    void clear();

protected:
    QJsonObject setSchema(const QString &schemaName, QJsonObject schema);

signals:

public slots:

private:
    QJsonObject _loadFromFile(const QString &, SchemaNameInitialization = UseFilename, const QString & = QString::null);
    QJsonObject _loadFromFolder(const QString &, const QString & = QString::null, const QByteArray & ext = "json");
    QJsonObject _loadFromData(const QByteArray &, const QString &, SchemaNameInitialization = UseParameter);

private:
    class SchemaValidatorPrivate;
    SchemaValidatorPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(SchemaValidator);
};

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAVALIDATOR_H

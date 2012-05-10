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
#include <QRegExp>
#include <QStringList>

#include "qjsonschema-global.h"
#include "qjsonschemaerror.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT QJsonSchemaValidator : public QObject
{
    Q_OBJECT
public:
    explicit QJsonSchemaValidator(QObject *parent = 0);
    ~QJsonSchemaValidator();

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

    QJsonSchemaError getLastError() const;

    bool isEmpty() const;
    QStringList schemaNames() const;
    bool hasSchema(const QString &);
    void removeSchema(const QString &);
    void clear();

    // schema validation filtering - limit schemas to be used during validation
    void setValidationFilter(const QRegExp &);

    // allows faster validation by matching between json object and schema name without doing schema iteration
    class SchemaNameMatcher;
    void setSchemaNameMatcher(const SchemaNameMatcher &);

protected:
    QJsonObject setSchema(const QString &schemaName, QJsonObject schema);

signals:

public slots:

private:
    QJsonObject _loadFromFile(const QString &, SchemaNameInitialization = UseFilename, const QString & = QString::null);
    QJsonObject _loadFromFolder(const QString &, const QString & = QString::null, const QByteArray & ext = "json");
    QJsonObject _loadFromData(const QByteArray &, const QString &, SchemaNameInitialization = UseParameter);

private:
    class QJsonSchemaValidatorPrivate;
    QJsonSchemaValidatorPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(QJsonSchemaValidator);

public:
    // helper classes
    class SchemaNameMatcher
    {
    public:
        SchemaNameMatcher(bool _bCanIndex) : m_bCanIndex(_bCanIndex) {}
        virtual ~SchemaNameMatcher() {}
        virtual SchemaNameMatcher *clone() const = 0;

        bool canIndex() const { return m_bCanIndex; }

        // creates an index for a schema to do quicker name matching
        virtual void createIndex(const QString &, const QJsonObject &) {}

        // knowing exact schema name allows to skip schema iteration and validate with some schemas only
        virtual QStringList getExactMatches(const QJsonObject &) = 0;
        virtual QStringList getPossibleMatches(const QJsonObject &) { return QStringList(); }

        virtual void reset() {}
protected:
        bool m_bCanIndex;
    };

    // schema name is defined by a property value in json object
    class SchemaPropertyNameMatcher : public SchemaNameMatcher
    {
    public:
        SchemaPropertyNameMatcher(const QString & property) : SchemaNameMatcher(false), m_property(property) {}
        SchemaNameMatcher *clone() const { return new SchemaPropertyNameMatcher(*this); }

        virtual QStringList getExactMatches(const QJsonObject &object)
        {
            QStringList strs;
            if (!m_property.isEmpty() && object.contains(m_property)) {
                QString str(object[m_property].toString());
                if (!str.isEmpty())
                    strs.append(str);
            }
            return strs;
        }

    private:
        QString m_property;
    };

    // schema contains a uniquely defined top-level key/property that can be used as a quick index
    class SchemaUniqueKeyNameMatcher : public SchemaNameMatcher
    {
    public:
        SchemaUniqueKeyNameMatcher(const QString & key);
        ~SchemaUniqueKeyNameMatcher();
        SchemaNameMatcher *clone() const { return new SchemaUniqueKeyNameMatcher(m_key); }

        // creates an index for a schema to do quicker name matching
        virtual void createIndex(const QString &schemaName, const QJsonObject & schema);

        // knowing exact schema name allows to skip schema iteration and validate with some schemas only
        virtual QStringList getExactMatches(const QJsonObject &);

        // possible matches will be validated too if exact match is missing
        virtual QStringList getPossibleMatches(const QJsonObject &);

        virtual void reset();

    private:
        QString m_key;

        class SchemaUniqueKeyNameMatcherPrivate;
        SchemaUniqueKeyNameMatcherPrivate * const d_ptr;
        Q_DECLARE_PRIVATE(SchemaUniqueKeyNameMatcher);
    };
};

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAVALIDATOR_H

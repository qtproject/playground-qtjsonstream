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

#include <QtTest/QtTest>

#include "schemavalidator.h"

QT_USE_NAMESPACE_JSONSTREAM

class tst_JsonSchema : public QObject
{
    Q_OBJECT

private slots:
    void schemaTest();
    // 5.1 type
    void testTypeValidation();
    // 5.2 properties
    void testProperitesValidation();
    // 5.5 items
    void testItemsValidation();
    // 5.7
    void testRequiredValidation();
    // 5.9, 5.10
    void testMinimumMaximumValidation();
    // 5.11, 5.12
    void testExclusiveMinimumExclusiveMaximumValidation();
    // 5.13
    void testMinItemsValidation();
    // 5.14
    void testMaxItemsValidation();
    // 5.16
    void testPatternValidation();
    // 5.17
    void testMinLengthValidation();
    // 5.18
    void testMaxLengthValidation();
    // 5.21
    void testTitleValidation();
    // 5.22
    void testDescriptionValidation();
    // 5.26
    void testExtendsValidation();
    // 5.28
    void testRefValidation();

private:
    bool validate(const char *data,  const QByteArray & schema);
    bool validate(const QJsonValue & object, const QByteArray & schema);
};

void tst_JsonSchema::schemaTest()
{
    bool result;
    SchemaValidator validator;
    result = validator.loadFromFolder(QDir::currentPath(), "title");
    QVERIFY(result);

    // Create an item that matches the schema
    QJsonObject item;
    item.insert("create-test", 22);
    item.insert("another-field", QLatin1String("a string"));

    result = validator.validateSchema("SchemaTestObject", item);
    qDebug() << "VALID validation result: " << result;
    QVERIFY(result);

    // Create an item that does not match the schema
    QJsonObject noncompliant;
    noncompliant.insert("create-test", 22);

    result = validator.validateSchema("SchemaTestObject", noncompliant);
    qDebug() << "INVALID validation result: " << result << " message is:" << validator.getLastError().errorString();
    QVERIFY(!result && validator.getLastError().errorCode() == SchemaError::FailedSchemaValidation);
}

void tst_JsonSchema::testTypeValidation()
{
    // type vs same schema type
    QVERIFY(validate(QJsonObject(), "{ \"type\" : \"object\" }"));
    QVERIFY(validate(QJsonArray(), "{ \"type\" : \"array\" }"));
    QVERIFY(validate(QJsonValue(QString("")), "{ \"type\" : \"string\" }"));
    QVERIFY(validate(QJsonValue(22), "{ \"type\" : \"number\" }"));
//    QVERIFY(validate(QJsonValue(22), "{ \"type\" : \"integer\" }"));
    QVERIFY(validate(QJsonValue(false), "{ \"type\" : \"boolean\" }"));
    QVERIFY(validate(QJsonValue(), "{ \"type\" : \"null\" }"));
    QVERIFY(validate(QJsonValue(true), "{ \"type\" : \"any\" }"));

    // type vs different schema type
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"object\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"array\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"string\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"number\" }"));
//    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"integer\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"boolean\" }"));
    QVERIFY(!validate(QJsonValue(false), "{ \"type\" : \"null\" }"));

    //object type vs non-object schema type
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"null\" }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"boolean\" }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"number\" }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"integer\" }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"string\" }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : \"array\" }"));

    //union type
    QVERIFY(validate(QJsonObject(), "{ \"type\" : [\"null\", \"boolean\", \"number\", \"integer\", \"string\", \"array\", \"object\"] }"));
    QVERIFY(!validate(QJsonObject(), "{ \"type\" : [\"null\", \"boolean\", \"number\", \"integer\", \"string\", \"array\"] }"));

    //schema union type
    QVERIFY(validate(QJsonObject(), "{ \"type\" : [{ \"type\" : \"string\" }, { \"type\" : \"object\" }] }"));
    QVERIFY(validate(QJsonValue(33), "{ \"type\" : [{ \"type\" : \"string\" }, { \"type\" : \"object\" }, \"number\"] }"));
    QVERIFY(!validate(QJsonArray(), "{ \"type\" : [\"string\", { \"type\" : \"object\" }] }"));
}

// 5.2 properties
void tst_JsonSchema::testProperitesValidation()
{
    QVERIFY(validate(QJsonObject(), "{ \"type\" : \"object\", \"properties\" : {} }"));

    QVERIFY(validate("{ \"a\" : 1 }", "{ \"type\" : \"object\", \"properties\" : { \"a\" : {}} }"));
    QVERIFY(validate("{ \"a\" : 1 }", "{ \"type\" : \"object\", \"properties\" : { \"a\" : { \"type\" : \"number\" }} }"));
    QVERIFY(!validate("{ \"a\" : 1 }", "{ \"type\" : \"object\", \"properties\" : { \"a\" : { \"type\" : \"string\" }} }"));

    QVERIFY(validate("{ \"a\" : { \"b\" : \"two\" } }",
                     "{ \"type\" : \"object\", \"properties\" : { \"a\" : { \"type\" : \"object\", \"properties\" : { \"b\" : { \"type\" : \"string\" } } }} }"));
    QVERIFY(!validate("{ \"a\" : { \"b\" : \"two\" } }",
                      "{ \"type\" : \"object\", \"properties\" : { \"a\" : { \"type\" : \"object\", \"properties\" : { \"b\" : { \"type\" : \"number\" } } }} }"));
}

// 5.5 items
void tst_JsonSchema::testItemsValidation()
{
    QVERIFY(validate(QJsonArray(), "{ \"type\" : \"array\", \"items\" : { \"type\" : \"string\" } }"));

    QJsonArray array;
    array.append(QLatin1String("foo")); // ["foo"]
    QVERIFY(validate(array, "{ \"type\" : \"array\", \"items\" : { \"type\" : \"string\" } }"));
    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : { \"type\" : \"number\" } }")); // INVALID

    array.append(2); // ["foo", 2]
    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : { \"type\" : \"string\" } }")); // INVALID
    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : { \"type\" : \"number\" } }")); // INVALID
    QVERIFY(validate(array, "{ \"type\" : \"array\", \"items\" : [{ \"type\" : \"string\" }, { \"type\" : \"number\" }] }"));

    array.removeAt(0); // [2]
    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : { \"type\" : \"string\" } }")); // INVALID

    array[0] = QLatin1String("foo");
    array[1] = QLatin1String("two"); //["foo", "two"]
    // should fail!!!!!
//fix    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : [{ \"type\" : \"string\" }, { \"type\" : \"number\" }] }"));
}

// 5.7
void tst_JsonSchema::testRequiredValidation()
{
    QVERIFY(validate("{ \"b\" : true }", "{ \"properties\" : { \"a\" : {} } }"));
    QVERIFY(validate(QJsonObject(), "{ \"properties\" : { \"a\" : { \"require\" : false } } }"));
    QVERIFY(validate("{ \"a\" : false }", "{ \"properties\" : { \"a\" : { \"required\" : false } } }"));
    QVERIFY(validate("{ \"a\" : false }", "{ \"properties\" : { \"a\" : { \"required\" : true } } }"));

    QVERIFY(!validate(QJsonObject(), "{ \"properties\" : { \"a\" : { \"required\" : true } } }"));
    QVERIFY(!validate("{ \"b\" : true }", "{ \"properties\" : { \"a\" : { \"required\" : true } } }"));
}

// 5.9, 5.10
void tst_JsonSchema::testMinimumMaximumValidation()
{
    QVERIFY(validate(QJsonValue(0), "{}"));
    QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 10 }"));
    QVERIFY(validate(QJsonValue(5), "{ \"minimum\" : 1, \"maximum\" : 10 }"));
    QVERIFY(validate(QJsonValue(10), "{ \"minimum\" : 1, \"maximum\" : 10 }"));
    QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 1 }"));

    QVERIFY(!validate(QJsonValue(0), "{ \"minimum\" : 1, \"maximum\" : 10 }"));
    QVERIFY(!validate(QJsonValue(11), "{ \"minimum\" : 1, \"maximum\" : 10 }"));
}

// 5.11, 5.12
void tst_JsonSchema::testExclusiveMinimumExclusiveMaximumValidation()
{
    //true
    QVERIFY(!validate(QJsonValue(0), "{ \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false }"));  //illegal
 /*FIX   QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false }"));
    QVERIFY(validate(QJsonValue(5), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(validate(QJsonValue(10), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 1, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));

    QVERIFY(!validate(QJsonValue(0), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(!validate(QJsonValue(11), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));

    //false
    QVERIFY(!validate(QJsonValue(0), "{ \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true }"));  //illegal
    QVERIFY(validate(QJsonValue(1.0001), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(validate(QJsonValue(5), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(validate(QJsonValue(9.9999), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));

    QVERIFY(!validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true }"));
    QVERIFY(!validate(QJsonValue(10), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 1, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(0), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(11), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));*/
}

// 5.13
void tst_JsonSchema::testMinItemsValidation()
{
}

// 5.14
void tst_JsonSchema::testMaxItemsValidation()
{
}

// 5.16
void tst_JsonSchema::testPatternValidation()
{
}

// 5.17
void tst_JsonSchema::testMinLengthValidation()
{
}

// 5.18
void tst_JsonSchema::testMaxLengthValidation()
{
}

// 5.21
void tst_JsonSchema::testTitleValidation()
{
}

// 5.22
void tst_JsonSchema::testDescriptionValidation()
{
}

// 5.26
void tst_JsonSchema::testExtendsValidation()
{
}

// 5.28
void tst_JsonSchema::testRefValidation()
{
}

bool tst_JsonSchema::validate(const char *data, const QByteArray & schemaBody)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull())
    {
        qDebug() << "JSON object data is invalid";
        return false;
    }

    QJsonObject object = doc.object();
    return validate(object, schemaBody);
}

bool tst_JsonSchema::validate(const QJsonValue & value, const QByteArray & schemaBody)
{
    bool result;

    QJsonObject object;
    object.insert("test", value);
    qDebug() << "object " << object;

    SchemaValidator validator;

    // wrap schema body
    QByteArray schema = QString("{ \"properties\": { \"test\": %1 } }").arg(schemaBody.constData()).toUtf8();

    result = validator.loadFromData(schema, "testSchema");
    qDebug() << "####### load result : " << result;

    if (result)
    {
        result = validator.validateSchema("testSchema", object);
        qDebug() << "####### validation result: " << result << " message is:" << validator.getLastError().errorString();
    }
    else
    {
        qDebug() << "############################ LOAD ERROR: " << schemaBody;
    }
    return result;
}

QTEST_MAIN(tst_JsonSchema)

#include "tst_jsonschema.moc"

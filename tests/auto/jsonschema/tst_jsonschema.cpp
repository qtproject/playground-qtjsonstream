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
    // TODO: 5.3 patternProperties
    // 5.4 additionalProperties
    void testAdditionalPropertiesValidation();
    // 5.5 items
    void testItemsValidation();
    // 5.6 additionalItems
    void testAdditionalItems();
    // 5.7
    void testRequiredValidation();
    // TODO: 5.8 dependencies
    // 5.9, 5.10
    void testMinimumMaximumValidation();
    // 5.11, 5.12
    void testExclusiveMinimumExclusiveMaximumValidation();
    // 5.13, 5.14
    void testMinMaxItemsValidation();
    // TODO: 5.15 uniqueItems
    // 5.16
    void testPatternValidation();
    // 5.17, 5.18
    void testMinMaxLengthValidation();
    // 5.19
    void testEnumValidation();
    // TODO: 5.20 default
    void testDefaultValidation();
    // 5.21
    void testTitleValidation();
    // 5.22
    void testDescriptionValidation();
    // 5.23 format
    void testFormatValidation();
    // 5.24
    void testDivisibleByValidation();
    // TODO: 5.25 disallow
    // 5.26
    void testExtendsValidation();
    // TODO: 5.27 id
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

    // test SchemaValidator::schemaNames() and SchemaValidator::hasSchema() with empty object
    QVERIFY(validator.schemaNames().isEmpty());
    QVERIFY(!validator.hasSchema("SchemaTestObject"));

    result = validator.loadFromFolder(QDir::currentPath(), "title");
    QVERIFY(result);

    // test SchemaValidator::schemaNames() and SchemaValidator::hasSchema() after schema was added
    QVERIFY(validator.schemaNames().contains("SchemaTestObject"));
    QVERIFY(validator.hasSchema("SchemaTestObject"));

    // Create an item that matches the schema
    QJsonObject item;
    item.insert("create-test", 11);
    item.insert("create-test0", 1);
    item.insert("another-field", QLatin1String("uuid:{zxcvbnm}"));

    result = validator.validateSchema("should_fail", item); // should fail - schema does not exist
    QVERIFY(!result);

    result = validator.validateSchema("SchemaTestObject", item);
    //qDebug() << "VALID validation result: " << result;
    QVERIFY(result);

    // Create an item that does not match the schema
    QJsonObject noncompliant;
    noncompliant.insert("create-test", 22);

    result = validator.validateSchema("SchemaTestObject", noncompliant);
    //qDebug() << "INVALID validation result: " << result << " message is:" << validator.getLastError().errorString();
    QVERIFY(!result && validator.getLastError().errorCode() == SchemaError::FailedSchemaValidation);

    // test SchemaValidator::removesSchema()
    validator.removeSchema("SchemaTestObject");
    QVERIFY(validator.schemaNames().isEmpty());
    QVERIFY(!validator.hasSchema("SchemaTestObject"));
}

void tst_JsonSchema::testTypeValidation()
{
    // type vs same schema type
    QVERIFY(validate(QJsonObject(), "{ \"type\" : \"object\" }"));
    QVERIFY(validate(QJsonArray(), "{ \"type\" : \"array\" }"));
    QVERIFY(validate(QJsonValue(QString("")), "{ \"type\" : \"string\" }"));
    QVERIFY(validate(QJsonValue(22), "{ \"type\" : \"number\" }"));
    QVERIFY(validate(QJsonValue(22), "{ \"type\" : \"integer\" }"));
    QVERIFY(validate(QJsonValue(false), "{ \"type\" : \"boolean\" }"));
//FIX    QVERIFY(validate(QJsonValue(), "{ \"type\" : \"null\" }"));
    QVERIFY(validate(QJsonValue(true), "{ \"type\" : \"any\" }"));

    // type vs different schema type
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"object\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"array\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"string\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"number\" }"));
    QVERIFY(!validate(QJsonValue(), "{ \"type\" : \"integer\" }"));
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

// 5.4 additionalProperties
void tst_JsonSchema::testAdditionalPropertiesValidation()
{
    //object tests
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }", "{ \"additionalProperties\" : true }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                     "{ \"properties\" : { \"a\" : {}, \"b\" : {} }, \"additionalProperties\" : true }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                     "{ \"properties\" : { \"a\" : {}, \"b\" : {}, \"c\" : {} }, \"additionalProperties\" : false }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2 }",  // a property 'c' is missing - still valid
                     "{ \"properties\" : { \"a\" : {}, \"b\" : {}, \"c\" : {} }, \"additionalProperties\" : false }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                     "{ \"additionalProperties\" : { \"type\" : \"number\"  } }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                     "{ \"properties\" : { \"a\" : {}, \"b\" : {} }, \"additionalProperties\" : { \"type\" : \"number\"  } }"));
    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                     "{ \"properties\" : { \"a\" : {}, \"b\" : {}, \"c\" : {} }, \"additionalProperties\" : { \"type\" : \"string\" } }"));

    QVERIFY(!validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                      "{ \"properties\" : { \"a\" : {}, \"b\" : {} }, \"additionalProperties\" : false }"));
    QVERIFY(!validate("{ \"a\" : 1, \"b\" : 2, \"c\" : 3 }",
                      "{ \"properties\" : { \"a\" : {}, \"b\" : {} }, \"additionalProperties\" : { \"type\" : \"string\" } }"));

    //array tests
    QJsonArray array; // [1,2,3]
    array.append(1);
    array.append(2);
    array.append(3);
    QVERIFY(validate(array, "{ \"additionalProperties\" : true }"));
    QVERIFY(validate(array, "{ \"additionalProperties\" : false }"));
    QVERIFY(validate(array, "{ \"additionalProperties\" : { \"type\" : \"number\"  } }"));
    QVERIFY(validate(array, "{ \"additionalProperties\" : { \"type\" : \"string\" } }"));

    array = QJsonArray();
    array.append(QLatin1String("foo"));
    array.append(QLatin1String("two")); //["foo", "two"]
    QVERIFY(validate(array, "{ \"items\" : { \"type\" : \"string\" }, \"additionalProperties\" : false }"));
    QVERIFY(validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalProperties\" : false }"));

    array.append(3); // ["foo", "two", 3]
    QVERIFY(validate(array,
        "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalProperties\" : { \"type\" : \"number\"  } }"));
    QVERIFY(!validate(array,
        "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalProperties\" : { \"type\" : \"string\"  } }"));

    array[2] = QLatin1String("three"); // ["foo", "two", "three"]
    QVERIFY(validate(array,
        "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalProperties\" : { \"type\" : \"number\"  } }"));
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

    array = QJsonArray();
    array.append(QLatin1String("foo"));
    array.append(QLatin1String("two")); //["foo", "two"]
    // should fail!!!!!
    QVERIFY(!validate(array, "{ \"type\" : \"array\", \"items\" : [{ \"type\" : \"string\" }, { \"type\" : \"number\" }] }"));
}

// 5.6 additionalItems
void tst_JsonSchema::testAdditionalItems()
{
    //array tests
    QJsonArray array;
    array.append(1); // [1]
    array.append(2); // [1, 2]
    array.append(3); // [1, 2, 3]

    QVERIFY(validate(array, "{ \"additionalItems\" : true }"));
    QVERIFY(validate(array, "{ \"additionalItems\" : { \"type\" : \"number\" } }"));
    QVERIFY(!validate(array, "{ \"additionalItems\" : false }"));
    QVERIFY(!validate(array, "{ \"additionalItems\" : { \"type\" : \"string\" } }"));

    array = QJsonArray();
    array.append(QLatin1String("1")); // ['1']
    array.append(QLatin1String("2")); // ['1','2']

    QVERIFY(validate(array, "{ \"items\" : { \"type\" : \"string\" }, \"additionalItems\" : false }"));
    QVERIFY(validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalItems\" : false }"));

    array.append(3); // ['1', '2', 3]
    QVERIFY(validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalItems\" : { \"type\" : \"number\" } }"));

    array[2] = QLatin1String("3"); // ['1', '2', '3']
    QVERIFY(validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" }, { \"type\" : \"string\" } ], \"additionalItems\": { \"type\" : \"number\" } }"));

    QVERIFY(!validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ],\"additionalItems\": false }"));
    QVERIFY(!validate(array, "{ \"items\" : [ { \"type\" : \"string\" }, { \"type\" : \"string\" } ],\"additionalItems\": { \"type\" : \"number\" } }"));
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
    //valid
    QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false }"));
    QVERIFY(validate(QJsonValue(5), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(validate(QJsonValue(10), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 1, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));

    // invalid
    QVERIFY(!validate(QJsonValue(0), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));
    QVERIFY(!validate(QJsonValue(11), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : false, \"exclusiveMaximum\" : false  }"));

    //valid
    QVERIFY(validate(QJsonValue(1.0001), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(validate(QJsonValue(5), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(validate(QJsonValue(9.9999), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));

    // invalid
    QVERIFY(!validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true }"));
    QVERIFY(!validate(QJsonValue(10), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(1), "{ \"minimum\" : 1, \"maximum\" : 1, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(0), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
    QVERIFY(!validate(QJsonValue(11), "{ \"minimum\" : 1, \"maximum\" : 10, \"exclusiveMinimum\" : true, \"exclusiveMaximum\" : true  }"));
}

// 5.13, 5.14
void tst_JsonSchema::testMinMaxItemsValidation()
{
    QVERIFY(validate(QJsonArray(), "{}"));
    QVERIFY(!validate(QJsonArray(), "{ \"minItems\" : 1, \"maxItems\" : 0 }"));
    QVERIFY(!validate(QJsonArray(), "{ \"minItems\" : 1, \"maxItems\" : 3 }"));

    QJsonArray array;
    array.append(1); // [1]
    QVERIFY(validate(array, "{ \"minItems\" : 1, \"maxItems\" : 1 }"));
    QVERIFY(validate(array, "{ \"minItems\" : 1, \"maxItems\" : 3 }"));

    array.append(2); // [1,2]
    QVERIFY(validate(array, "{ \"minItems\" : 1, \"maxItems\" : 3 }"));
    array.append(3); // [1, 2, 3]
    QVERIFY(validate(array, "{ \"minItems\" : 1, \"maxItems\" : 3 }"));

    array.append(4); // [1, 2, 3, 4]
    QVERIFY(!validate(array, "{ \"minItems\" : 1, \"maxItems\" : 3 }"));
}

// 5.16
void tst_JsonSchema::testPatternValidation()
{
    QVERIFY(validate(QJsonValue(QString("")), "{}"));
    QVERIFY(validate(QJsonValue(QString("")), "{ \"pattern\" : \"^$\" }"));

    // number
    QVERIFY(validate(QJsonValue(QString("123")), "{ \"pattern\" : \"\\\\d+\" }"));
    QVERIFY(!validate(QJsonValue(QString("12t")), "{ \"pattern\" : \"\\\\d+\" }"));

    // string
    QVERIFY(validate(QJsonValue(QString("today")), "{ \"pattern\" : \"today\" }"));
    QVERIFY(validate(QJsonValue(QString("today")), "{ \"pattern\" : \".oda.\" }"));
    QVERIFY(!validate(QJsonValue(QString("today")), "{ \"pattern\" : \"day\" }"));

    QVERIFY(!validate(QJsonValue(QString("")), "{ \"pattern\" : \"^ $\" }"));
    QVERIFY(!validate(QJsonValue(QString("today")), "{ \"pattern\" : \"dam\" }"));
    QVERIFY(!validate(QJsonValue(QString("aaaaa")), "{ \"pattern\" : \"aa(a\" }"));
}

// 5.17, 5.18
void tst_JsonSchema::testMinMaxLengthValidation()
{
    QVERIFY(validate(QJsonValue(QString("1")), "{ \"minLength\" : 1, \"maxLength\" : 1 }"));
    QVERIFY(validate(QJsonValue(QString("1")), "{ \"minLength\" : 1, \"maxLength\" : 3 }"));
    QVERIFY(validate(QJsonValue(QString("12")), "{ \"minLength\" : 1, \"maxLength\" : 3 }"));
    QVERIFY(validate(QJsonValue(QString("123")), "{ \"minLength\" : 1, \"maxLength\" : 3 }"));

    QVERIFY(!validate(QJsonValue(QString("")), "{ \"minLength\" : 1, \"maxLength\" : 0 }"));
    QVERIFY(!validate(QJsonValue(QString("")), "{ \"minLength\" : 1, \"maxLength\" : 3 }"));
    QVERIFY(!validate(QJsonValue(QString("1234")), "{ \"minLength\" : 1, \"maxLength\" : 3 }"));
}

// 5.19
void tst_JsonSchema::testEnumValidation()
{
    QVERIFY(validate(QJsonValue(true), "{ \"enum\" : [false, true] }"));
    QVERIFY(validate(QJsonValue(2), "{ \"enum\" : [1, 2, 3] }"));
    QVERIFY(validate(QJsonValue(QString("a")), "{ \"enum\" : [\"a\"] }"));
//FIX    QVERIFY(validate({}, "{ \"properties\" : { \"a\" : { \"enum\" : [\"a\"], \"optional\" : true, \"required\" : false } } }"));

    QVERIFY(!validate(QJsonValue(true), "{ \"enum\" : [\"false\", \"true\"] }"));
    QVERIFY(!validate(QJsonValue(4), "{ \"enum\" : [1, 2, 3, \"4\"] }"));
    QVERIFY(!validate(QJsonValue(QString()), "{ \"enum\" : [] }"));
//FIX    QVERIFY(!validate({}, "{ \"properties\" : { \"a\" : { \"enum\" : [\"a\"], \"optional\" : false, \"required\" : true } } }"));
}


// 5.20
void tst_JsonSchema::testDefaultValidation()
{
//    QVERIFY(validate("{ \"b\" : true }", "{ \"type\":\"object\", \"properties\" : { \"a\" : { \"type\":\"string\", \"default\": \"hoi\"} } }"));
    QVERIFY(validate("{ \"b\" : true }", "{ \"type\":\"object\", \"properties\" : { \"a\" : { \"type\":\"string\", \"default\": \"hoi\"}, \"c\" : { \"type\":\"array\", \"default\":[\"a\",\"b\",\"c\"]} } }"));
}


// 5.21
void tst_JsonSchema::testTitleValidation()
{
}

// 5.22
void tst_JsonSchema::testDescriptionValidation()
{
}

// 5.23 format
void tst_JsonSchema::testFormatValidation()
{
    // format=date-time
    QVERIFY(validate(QJsonValue(QString("2112-12-12T12:34:56Z")), "{ \"format\" : \"date-time\" }"));
    QVERIFY(!validate(QJsonValue(QString("21121212T123456Z")), "{ \"format\" : \"date-time\" }"));
    // format=date
    QVERIFY(validate(QJsonValue(QString("2112-12-12")), "{ \"format\" : \"date\" }"));
    QVERIFY(!validate(QJsonValue(QString("21121212")), "{ \"format\" : \"date\" }"));
    // format=time
    QVERIFY(validate(QJsonValue(QString("12:34:56")), "{ \"format\" : \"time\" }"));
    QVERIFY(!validate(QJsonValue(QString("123456")), "{ \"format\" : \"time\" }"));
    // format=url
    QVERIFY(validate(QJsonValue(QString("http://www.zzz.zu/zzz")), "{ \"format\" : \"url\" }"));
    // format=uri
    QVERIFY(validate(QJsonValue(QString("uuid:{zxcvbnm}")), "{ \"format\" : \"uri\" }"));
    QVERIFY(validate(QJsonValue(QString("urn:issn:1536-3613")), "{ \"format\" : \"uri\" }"));
    // format=NonNegativeInteger
    QVERIFY(validate(QJsonValue(56), "{ \"format\" : \"NonNegativeInteger\" }"));
    QVERIFY(!validate(QJsonValue(56.5), "{ \"format\" : \"NonNegativeInteger\" }"));
    QVERIFY(!validate(QJsonValue(-56), "{ \"format\" : \"NonNegativeInteger\" }"));
}

// 5.24
void tst_JsonSchema::testDivisibleByValidation()
{
    QVERIFY(validate(QJsonValue(0), "{ \"divisibleBy\" : 1 }"));
    QVERIFY(validate(QJsonValue(10), "{ \"divisibleBy\" : 5 }"));
    QVERIFY(validate(QJsonValue(10), "{ \"divisibleBy\" : 10 }"));
    QVERIFY(validate(QJsonValue(0), "{ \"divisibleBy\" : 2.5 }"));
    QVERIFY(validate(QJsonValue(5), "{ \"divisibleBy\" : 2.5 }"));
    QVERIFY(validate(QJsonValue(7.5), "{ \"divisibleBy\" : 2.5 }"));

    QVERIFY(!validate(QJsonValue(0), "{ \"divisibleBy\" : 0 }"));
    QVERIFY(!validate(QJsonValue(7), "{ \"divisibleBy\" : 5 }"));
    QVERIFY(!validate(QJsonValue(4.5), "{ \"divisibleBy\" : 2 }"));
    QVERIFY(!validate(QJsonValue(7.5), "{ \"divisibleBy\" : 1.8 }"));
}

// 5.26
void tst_JsonSchema::testExtendsValidation()
{
    QVERIFY(validate("{}"," { \"extends\" : {} }"));
    QVERIFY(validate("{}"," { \"extends\" : { \"type\" : \"object\" } }"));
    QVERIFY(validate(QJsonValue(1), "{ \"type\" : \"integer\", \"extends\" : { \"type\" : \"number\" } }"));
//FIX    QVERIFY(validate("{ \"a\" : 1, \"b\" : 2 }"," { \"properties\" : { \"a\" : { \"type\" : \"number\" } }, \"additionalProperties\" : false, \"extends\" : { \"properties\" : { \"b\" : { \"type\" : \"number\" } } } }"));

    QVERIFY(!validate(1, "{ \"type\" : \"number\", \"extends\" : { \"type\" : \"string\" } }"));

    //TODO: More tests
}

// 5.28
void tst_JsonSchema::testRefValidation()
{
    //
    QVERIFY(validate("{ \"a\" : {} }", "{ \"type\" : \"object\", \"additionalProperties\" : { \"$ref\" : \"#\" } }"));
    QVERIFY(!validate("{ \"a\" : 1 }", "{ \"type\" : \"object\", \"additionalProperties\" : { \"$ref\" : \"#\" } }"));
    QVERIFY(!validate("{ \"a\" : \"1\" }", "{ \"type\" : \"object\", \"additionalProperties\" : { \"$ref\" : \"#\" } }"));
}

bool tst_JsonSchema::validate(const char *data, const QByteArray & schemaBody)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull())
    {
        //qDebug() << "JSON object data is invalid";
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
    //qDebug() << "object " << object;

    SchemaValidator validator;

    // wrap schema body
    QByteArray schema = QString("{ \"properties\": { \"test\": %1 } }").arg(schemaBody.constData()).toUtf8();

    result = validator.loadFromData(schema, "testSchema");
    //qDebug() << "####### load result : " << result;

    if (result)
    {
        result = validator.validateSchema("testSchema", object);
        //qDebug() << "####### validation result: " << result << " message is:" << validator.getLastError().errorString();
    }
    else
    {
        //qDebug() << "############################ LOAD ERROR: " << schemaBody;
    }
    return result;
}

QTEST_MAIN(tst_JsonSchema)

#include "tst_jsonschema.moc"

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>

#include "private/qjsonbuffer_p.h"

QT_USE_NAMESPACE_JSONSTREAM

class tst_JsonBuffer : public QObject
{
    Q_OBJECT

private slots:
    void utf8();
    void utf8extend();
};


const char *utf8spaces[] = { "{\"a\":123.0}{\"b\":234}\t \r\n", // test for a buffer size
                             "{\"a\": 123}   \n{\"b\"   :234  }  ",
                             "{     \"a\"  : 123.0000}   \n { \"b\"   :234  }  ",
                             "\n    {\"a\": 123}   \n{\"b\"   :234  }  ",
                             "{\"a\":123.0} {\"b\":234}  {\"c\":345 ",
                             "\xef\xbb\xbf{\"a\":123.0} {\"b\":234}"
};

void tst_JsonBuffer::utf8()
{
    int n = sizeof(utf8spaces) / sizeof(utf8spaces[0]);
    for (int i = 0 ; i < n ; i++ ) {
        QJsonBuffer buf;
        buf.append(utf8spaces[i], strlen(utf8spaces[i]));

        QVERIFY(buf.messageAvailable());
        QJsonObject a = buf.readMessage();
        QCOMPARE(a.value("a").toDouble(), 123.0);

        QVERIFY(buf.messageAvailable());
        QJsonObject b = buf.readMessage();
        QCOMPARE(b.value("b").toDouble(), 234.0);

        QVERIFY(!buf.messageAvailable());
        QVERIFY(buf.format() == FormatUTF8);

        if (0 == i)
            QVERIFY(buf.size() == 0); // buffer should be empty at the end
    }
}

struct PartialData {
    const char *packet;
    const char *var;
    double value;
};

PartialData utf8packets[] = { { "{\"a\":123.0", 0, 0 },
                              { "    }", "a", 123.0 },
                              { "  \n{\"b\"   :234  }  ", "b", 234.0 },
                              { "  \n{\"c\"   :  1000.0  }  ", "c", 1000.0 },
                              { "{\"value\":  ", 0, 0 },
                              { " 1.0", 0, 0 },
                              { " \t\t\t}\t \r\n", "value", 1.0 } // test for a buffer size
};

void tst_JsonBuffer::utf8extend()
{
    QJsonBuffer buf;
    int n = sizeof(utf8packets) / sizeof(utf8packets[0]);
    for (int i = 0 ; i < n ; i++ ) {
        PartialData& d = utf8packets[i];
        buf.append(d.packet, strlen(d.packet));

        QCOMPARE(buf.messageAvailable(), d.var != 0);
        if (d.var) {
            QJsonObject a = buf.readMessage();
            QCOMPARE(a.value(d.var).toDouble(), d.value);
        }
        QVERIFY(!buf.messageAvailable());
        QVERIFY(buf.format() == FormatUTF8);
    }
    QVERIFY(buf.size() == 0); // buffer should be empty at the end
}

QTEST_MAIN(tst_JsonBuffer)

#include "tst_jsonbuffer.moc"

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

#include <QCoreApplication>
#include <QStringList>
#include <QDebug>
#include <QJsonArray>

#include "qjsonclient.h"

QT_USE_NAMESPACE_JSONSTREAM

QString gFormat;
QString gSocketname = "/tmp/tst_jsonstream";

class Container : public QObject
{
    Q_OBJECT

public:
    Container();
    void sendMessage();
    void sendSchemaTestMessage();

public slots:
    void received(const QJsonObject& message);
    void disconnected();

private:
    QJsonClient *mClient;
    int         mCounter;
};

Container::Container()
    : mCounter(0)
{
    qDebug() << "Creating new json client with format" << gFormat;
    mClient = new QJsonClient;
    connect(mClient, SIGNAL(messageReceived(const QJsonObject&)),
            SLOT(received(const QJsonObject&)));
    connect(mClient, SIGNAL(disconnected()), SLOT(disconnected()));
    if (gFormat == "qbjs")
        mClient->setFormat(FormatQBJS);
    else if (gFormat == "bson")
        mClient->setFormat(FormatBSON);
    else if (gFormat == "utf" || gFormat == "utf8")
        mClient->setFormat(FormatUTF8);
    else if (gFormat == "utf16be")
        mClient->setFormat(FormatUTF16BE);
    else if (gFormat == "utf16le")
        mClient->setFormat(FormatUTF16LE);
    else if (gFormat == "utf32be")
        mClient->setFormat(FormatUTF32BE);
    else if (gFormat == "utf32le")
        mClient->setFormat(FormatUTF32LE);

    if (!mClient->connectLocal(gSocketname)) {
        qWarning() << "Unable to connect to" << gSocketname;
        exit(2);
    }
}

void Container::sendMessage()
{
    QJsonObject msg;
    msg.insert("text", QLatin1String("Standard text"));
    msg.insert("number", mCounter++);
    msg.insert("int", 100);
    msg.insert("float", 100.0);
    msg.insert("true", true);
    msg.insert("false", false);
    msg.insert("array", QJsonArray::fromStringList(QStringList() << "one" << "two" << "three"));
    QJsonObject obj;
    obj.insert("item1", QLatin1String("This is item 1"));
    obj.insert("item2", QLatin1String("This is item 2"));
    msg.insert("object", obj);
    mClient->send(msg);
}

void Container::sendSchemaTestMessage()
{
    qDebug() << "sending PaintTextEvent";
    // send paint text message
    QJsonObject msg;
    msg.insert("event", QLatin1String("PaintTextEvent"));
    msg.insert("text", QLatin1String("Schema test"));
    msg.insert("font-size", 100);
    msg.insert("x", 25);
    msg.insert("y", 100);
    msg.insert("bold", true);
    mClient->send(msg);
}

void Container::received(const QJsonObject& message)
{
    // test large string size
    if (message.contains("large") || message.contains("large_size")) {
        if (message.value("large").toString().size() != message.value("large_size").toDouble()) {
            qWarning() << "Large string size mismatch" << message.value("large").toString().size() << "!=" << message.value("large_size").toDouble();
            exit(3);
        }
    }

    QString command = message.value("command").toString();
    if (!command.isEmpty()) {
        qDebug() << "Received command" << command;
        if (command == "exit")
            exit(0);
        else if (command == "crash")
            exit(1);
        else if (command == "reply")
            sendMessage();
        else if (command == "flurry") {
            for (int count = message.value("count").toDouble() ; count ; --count )
                sendMessage();
        }
    }
}

void Container::disconnected()
{
    exit(0);
}

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = QCoreApplication::arguments();
    QString progname = args.takeFirst();
    bool bSchema = false;
    while (args.size()) {
        QString arg = args.at(0);
        if (!arg.startsWith("-"))
            break;
        args.removeFirst();
        if (arg == "-format")
            gFormat = args.takeFirst();
        else if (arg == "-socket")
            gSocketname = args.takeFirst();
        else if (arg == "-schema")
            bSchema = true;
    }

    Container c;
    if (!bSchema)
        c.sendMessage();
    else
        c.sendSchemaTestMessage();
    return app.exec();
}

#include "main.moc"

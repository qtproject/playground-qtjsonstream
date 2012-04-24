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

#include <QtTest>
#include <QLocalSocket>
#include <QLocalServer>
#include "jsonserver.h"
#include "jsonconnection.h"
#include "jsonendpoint.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>

QT_USE_NAMESPACE_JSONSTREAM

Q_DECLARE_METATYPE(QJsonObject);
Q_DECLARE_METATYPE(JsonConnection::State);

void waitForSpy(QSignalSpy& spy, int count, int timeout=5000) {
    QTime stopWatch;
    stopWatch.restart();
    forever {
        if (spy.count() == count)
            break;
        QTestEventLoop::instance().enterLoop(1);
        if (stopWatch.elapsed() >= timeout)
            QFAIL("Timed out");
    }
}

class Child : public QObject {
    Q_OBJECT
public:
    Child(const QString& progname, const QStringList& arguments) {
        process = new QProcess;
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyread()));
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(finished(int, QProcess::ExitStatus)));
        connect(process, SIGNAL(stateChanged(QProcess::ProcessState)),
                SLOT(stateChanged(QProcess::ProcessState)));
        connect(process, SIGNAL(error(QProcess::ProcessError)),
                SLOT(error(QProcess::ProcessError)));
        process->start(progname, arguments);
        QVERIFY(process->waitForStarted(5000));
    }

    ~Child() {
        if (process)
            delete process;
        process = 0;
    }

    void waitForFinished() {
        if (process->state() == QProcess::Running)
            QVERIFY(process->waitForFinished(5000));
        QVERIFY(process->exitCode() == 0);
        delete process;
        process = 0;
    }

protected slots:
    void readyread() {
        QByteArray ba = process->readAllStandardOutput();

        if (ba.simplified() == "READY") {
            qDebug() << "SERVER READY";
            emit serverReady();
            return;
        }

        QList<QByteArray> list = ba.split('\n');
        foreach (const QByteArray& s, list)
            if (s.size())
                qDebug() << "PROCESS" << s;
    }
    void stateChanged(QProcess::ProcessState state) {
        qDebug() << "Process state" << state;
    }
    void error(QProcess::ProcessError err) {
        qDebug() << "Process error" << err;
    }
    void finished( int exitcode, QProcess::ExitStatus status ) {
        qDebug() << Q_FUNC_INFO << exitcode << status;
    }

signals:
    void serverReady();

private:
    QProcess *process;
};

/****************************/
class EndpointContainer : public QObject
{
    Q_OBJECT

public:
    EndpointContainer(QObject *parent = 0);

    JsonEndpoint *addEndpoint(const QString & name);

    void sendMessage(const QString & endpointDestination = QString::null, JsonEndpoint *sender = 0);
    void sendMessage(const QStringList & endpointDestinationList, int messagesPerEndpoint, bool grouped);

    QList<QObject *> endpoints() { return mEndpoints; }

    JsonConnection *connection() { return mConnection; }
    void setConnection(JsonConnection *connection);

public slots:
    void processMessages();

signals:
    void messageReceived(const QJsonObject&, QObject *);

protected:
    void addEndpoint(JsonEndpoint *endpoint);

private:
    JsonConnection *mConnection;
    JsonEndpoint *mStream;
    QList<QObject *> mEndpoints;
    int         mCounter;
};


EndpointContainer::EndpointContainer(QObject *parent)
    : QObject(parent), mConnection(0), mCounter(0)
{
}

JsonEndpoint *EndpointContainer::addEndpoint(const QString & name)
{
    JsonEndpoint *endpoint = new JsonEndpoint(name, mConnection);
    endpoint->setParent(mConnection);
    addEndpoint(endpoint);
    return endpoint;
}

void EndpointContainer::addEndpoint(JsonEndpoint *endpoint)
{
    mStream = endpoint;
    connect(mStream, SIGNAL(readyReadMessage()), SLOT(processMessages()));
    mEndpoints.append(mStream);
}

void EndpointContainer::setConnection(JsonConnection *connection)
{
    mConnection = connection;
    foreach (QObject *obj, mEndpoints) {
        JsonEndpoint *endpoint = qobject_cast<JsonEndpoint *> (obj);
        if (endpoint)
            endpoint->setConnection(connection);
    }
}


void EndpointContainer::sendMessage(const QString & endpointDestination, JsonEndpoint *sender)
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

    if (!endpointDestination.isEmpty()) {
        msg.insert("endpoint", endpointDestination);
    }

    (sender != 0 ? sender : mStream)->send(msg);
}

void EndpointContainer::sendMessage(const QStringList &endpointDestinationList,
                                    int messagesPerEndpoint, bool grouped)
{
    QJsonObject msg;
    msg.insert("command", QLatin1String("flurry"));
    msg.insert("count", messagesPerEndpoint);
    msg.insert("endpoints", QJsonArray::fromStringList(endpointDestinationList));
    msg.insert("grouped", grouped);

    mStream->send(msg);
}

void EndpointContainer::processMessages() {
    QVERIFY(QThread::currentThread() == thread());
//    qDebug() << "XXX EndpointContainer::processMessages: " << sender();
    QObject *source(sender());
    QVERIFY(source && mEndpoints.contains(source));
    if (mEndpoints.contains(source)) {
        JsonEndpoint *endpoint = qobject_cast< JsonEndpoint *>(source);
//        qDebug() << "    XXX: came from: " << endpoint->name();
        while (endpoint->messageAvailable()) {
            QJsonObject obj = endpoint->readMessage();
//            qDebug() << "    XXX: going to: " << obj.value("endpoint").toString();
            if (!obj.isEmpty())
                emit messageReceived(obj, source);
        }
    }
}

/****************************/
class ConnectionContainer : public EndpointContainer
{
    Q_OBJECT

public:
    ConnectionContainer(const QString & socketName, bool bSeparateThread = false);

    void doConnect();
    void closeConnection() {
        JsonConnection *oldConnection = connection();
        setConnection(0);
        delete oldConnection;
    }

signals:
    void disconnected();

private:
    QString mSocketName;
};

ConnectionContainer::ConnectionContainer(const QString & socketName, bool bSeparateThread/*= false*/)
    : EndpointContainer(), mSocketName(socketName)
{
    qDebug() << "Creating new json client at " << socketName;
    JsonConnection *connection = new JsonConnection();
    connection->setUseSeparateThreadForProcessing(bSeparateThread);
    connection->setFormat(FormatUTF8);

    connect(connection, SIGNAL(disconnected()), SIGNAL(disconnected()));

    addEndpoint(connection->defaultEndpoint());
    setConnection(connection);
}

void ConnectionContainer::doConnect()
{
    if (!connection()->connectLocal(mSocketName)) {
        qWarning() << "Unable to connect to" << mSocketName;
        exit(2);
    }
}

/****************************/

class tst_JsonConnection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void connectionTest();
    void connectionSameThreadTest();
    void declarativeTest();
    void declarativeDefaultTest();
    void multipleEndpointsTest();
    void multipleThreadTest();
    void autoreconnectTest();
private:
    void registerQmlTypes();

    QQmlEngine engine;
};

void tst_JsonConnection::initTestCase()
{
    qRegisterMetaType<QJsonObject>();
    registerQmlTypes();
}


void tst_JsonConnection::connectionTest()
{
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    ConnectionContainer c(socketname,true);

    JsonEndpoint *endpoint = c.addEndpoint("test");
    QVERIFY(endpoint->name() == "test");

    QVERIFY(c.connection()->state() == JsonConnection::Unconnected);
    c.doConnect();
    QVERIFY(c.connection()->state() == JsonConnection::Connected);
    c.sendMessage(endpoint->name());

    QSignalSpy spy(&c, SIGNAL(messageReceived(QJsonObject,QObject *)));
    waitForSpy(spy, 1);

    QJsonObject msg = qvariant_cast<QJsonObject>(spy.last().at(0));
    qDebug() << msg;

    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
    QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 100);
    QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
    QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
    QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
    QVERIFY(msg.value("array").isArray());
    QJsonArray array = msg.value("array").toArray();
    QVERIFY(array.size() == 3);
    QVERIFY(array.at(0).toString() == "one");
    QVERIFY(array.at(1).toString() == "two");
    QVERIFY(array.at(2).toString() == "three");
    QVERIFY(msg.value("object").isObject());
    QJsonObject object = msg.value("object").toObject();
    QVERIFY(object.value("item1").toString() == "This is item 1");
    QVERIFY(object.value("item2").toString() == "This is item 2");

    c.closeConnection();

    child.waitForFinished();
}

void tst_JsonConnection::connectionSameThreadTest()
{
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    ConnectionContainer c(socketname, false);

    JsonEndpoint *endpoint = c.addEndpoint("test");
    QVERIFY(endpoint->name() == "test");

    c.doConnect();
    c.sendMessage(endpoint->name());

    QSignalSpy spy(&c, SIGNAL(messageReceived(QJsonObject,QObject *)));
    waitForSpy(spy, 1);

    QJsonObject msg = qvariant_cast<QJsonObject>(spy.last().at(0));
    qDebug() << msg;

    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
    QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 100);
    QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
    QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
    QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
    QVERIFY(msg.value("array").isArray());
    QJsonArray array = msg.value("array").toArray();
    QVERIFY(array.size() == 3);
    QVERIFY(array.at(0).toString() == "one");
    QVERIFY(array.at(1).toString() == "two");
    QVERIFY(array.at(2).toString() == "three");
    QVERIFY(msg.value("object").isObject());
    QJsonObject object = msg.value("object").toObject();
    QVERIFY(object.value("item1").toString() == "This is item 1");
    QVERIFY(object.value("item2").toString() == "This is item 2");

    c.closeConnection();

    child.waitForFinished();
}

class Watcher : public QObject
{
    Q_OBJECT
public:
    Watcher() {}

signals:
    void done();
};

void tst_JsonConnection::registerQmlTypes()
{
    qmlRegisterType<JsonConnection>("Qt.json.connection.test", 1,0, "JsonConnection");
    qmlRegisterType<JsonEndpoint>("Qt.json.connection.test", 1,0, "JsonEndpoint");
}

static const char szData[] =
        "import QtQuick 2.0 \n\
         import Qt.json.connection.test 1.0 \n\
            QtObject { \n\
            id: root \n\
            property var retmsg \n\
            \
            property variant prop; \n\
            prop: JsonConnection { \n\
                id: _connection \n\
                localSocketName: \"/tmp/tst_socket\" \n\
                endpointPropertyName: \"endpoint\" \n\
            } \n\
            \
            property variant prope; \n\
            prope: JsonEndpoint { \n\
                 id: endpoint \n\
                 name: \"endpoint\" \n\
                 connection: _connection \n\
                 onReadyReadMessage: { \n\
                                retmsg = endpoint.readMessageMap(); \n\
                                retmsg.extra = \"extra\"; \n\
                                retmsg.int++; \n\
                                watcher.done(); \n\
                 } \n\
            } \n\
            \
            Component.onCompleted: { \n\
                _connection.connectLocal(); \n\
                endpoint.send(msg); \n\
            } \n\
         }";


void tst_JsonConnection::declarativeTest()
{
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    Watcher watcher;
    QSignalSpy spy(&watcher, SIGNAL(done()));
    engine.rootContext()->setContextProperty("watcher", &watcher);

    QJsonObject msg;
    msg.insert("endpoint", QLatin1String("endpoint"));
    msg.insert("text", QLatin1String("Standard text"));
    msg.insert("number", 0);
    msg.insert("int", 100);
    msg.insert("float", 100.0);
    msg.insert("true", true);
    msg.insert("false", false);
    msg.insert("array", QJsonArray::fromStringList(QStringList() << "one" << "two" << "three"));
    QJsonObject obj;
    obj.insert("item1", QLatin1String("This is item 1"));
    obj.insert("item2", QLatin1String("This is item 2"));
    msg.insert("object", obj);
    engine.rootContext()->setContextProperty("msg", QVariant::fromValue(msg));

    QJsonDocument document(msg);
    QString str = document.toJson();
    engine.rootContext()->setContextProperty("msgstr", str);

    QQmlComponent component(&engine);
    component.setData(szData, QUrl());


    if (!component.isReady()) {
        qDebug() << "QQmlComponent::setData error: " << component.errorString();
    }
    QVERIFY(component.isReady());

    QObject *componentObject = component.create();
    QVERIFY(componentObject != 0);

    waitForSpy(spy, 1);

    msg = QJsonObject::fromVariantMap(componentObject->property("retmsg").value<QVariantMap>());
    QVERIFY(msg.value("extra").isString() && msg.value("extra").toString() == QLatin1String("extra"));
    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
    QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 101);
    QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
    QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
    QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
    QVERIFY(msg.value("array").isArray());
    QJsonArray array = msg.value("array").toArray();
    QVERIFY(array.size() == 3);
    QVERIFY(array.at(0).toString() == "one");
    QVERIFY(array.at(1).toString() == "two");
    QVERIFY(array.at(2).toString() == "three");
    QVERIFY(msg.value("object").isObject());
    QJsonObject object = msg.value("object").toObject();
    QVERIFY(object.value("item1").toString() == "This is item 1");
    QVERIFY(object.value("item2").toString() == "This is item 2");

    delete componentObject;

    child.waitForFinished();

}

static const char szDefaultData[] =
        "import QtQuick 2.0 \n\
         import Qt.json.connection.test 1.0 \n\
            QtObject { \n\
            id: root \n\
            property var retmsg \n\
            \
            property variant prop; \n\
            prop: JsonConnection { \n\
                id: _connection \n\
                localSocketName: \"/tmp/tst_socket\" \n\
                endpointPropertyName: \"endpoint\" \n\
            } \n\
            \
            property variant prope; \n\
            prope: JsonEndpoint { \n\
                 id: endpoint \n\
                 connection: _connection \n\
                 onReadyReadMessage: { \n\
                                retmsg = endpoint.readMessageMap(); \n\
                                retmsg.extra = \"extra\"; \n\
                                retmsg.int++; \n\
                                watcher.done(); \n\
                 } \n\
            } \n\
            \
            Component.onCompleted: { \n\
                _connection.connectLocal(); \n\
                endpoint.send(msg); \n\
            } \n\
         }";


void tst_JsonConnection::declarativeDefaultTest()
{
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    Watcher watcher;
    QSignalSpy spy(&watcher, SIGNAL(done()));
    engine.rootContext()->setContextProperty("watcher", &watcher);

    QJsonObject msg;
    msg.insert("endpoint", QLatin1String("endpoint"));
    msg.insert("text", QLatin1String("Standard text"));
    msg.insert("number", 0);
    msg.insert("int", 100);
    msg.insert("float", 100.0);
    msg.insert("true", true);
    msg.insert("false", false);
    msg.insert("array", QJsonArray::fromStringList(QStringList() << "one" << "two" << "three"));
    QJsonObject obj;
    obj.insert("item1", QLatin1String("This is item 1"));
    obj.insert("item2", QLatin1String("This is item 2"));
    msg.insert("object", obj);
    engine.rootContext()->setContextProperty("msg", QVariant::fromValue(msg));

    QJsonDocument document(msg);
    QString str = document.toJson();
    engine.rootContext()->setContextProperty("msgstr", str);

    QQmlComponent component(&engine);
    component.setData(szDefaultData, QUrl());


    if (!component.isReady()) {
        qDebug() << "QQmlComponent::setData error: " << component.errorString();
    }
    QVERIFY(component.isReady());

    QObject *componentObject = component.create();
    QVERIFY(componentObject != 0);

    waitForSpy(spy, 1);

    msg = QJsonObject::fromVariantMap(componentObject->property("retmsg").value<QVariantMap>());
    QVERIFY(msg.value("extra").isString() && msg.value("extra").toString() == QLatin1String("extra"));
    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
    QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 101);
    QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
    QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
    QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
    QVERIFY(msg.value("array").isArray());
    QJsonArray array = msg.value("array").toArray();
    QVERIFY(array.size() == 3);
    QVERIFY(array.at(0).toString() == "one");
    QVERIFY(array.at(1).toString() == "two");
    QVERIFY(array.at(2).toString() == "three");
    QVERIFY(msg.value("object").isObject());
    QJsonObject object = msg.value("object").toObject();
    QVERIFY(object.value("item1").toString() == "This is item 1");
    QVERIFY(object.value("item2").toString() == "This is item 2");

    delete componentObject;

    child.waitForFinished();

}

void tst_JsonConnection::multipleEndpointsTest()
{
    const int knEndpointsNumber = 5;
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    ConnectionContainer c(socketname,true);

    for (int i = 0; i < knEndpointsNumber; i++) {
        QString endpointName("test");
        endpointName += QString::number(i);

        JsonEndpoint *endpoint = c.addEndpoint(endpointName);
        QVERIFY(endpoint->name() == endpointName);
    }

    QVERIFY(c.connection()->state() == JsonConnection::Unconnected);
    c.doConnect();
    QVERIFY(c.connection()->state() == JsonConnection::Connected);

    for (int i = 0; i < knEndpointsNumber; i++) {
        QString endpointName("test");
        endpointName += QString::number(i);
        c.sendMessage(endpointName);
    }

    QSignalSpy spy(&c, SIGNAL(messageReceived(QJsonObject,QObject *)));
    waitForSpy(spy, knEndpointsNumber);

    QList<QObject *> endpoints(c.endpoints());

    QSignalSpy::const_iterator it;
    for (it = spy.constBegin(); it != spy.constEnd(); it++) {
        QJsonObject msg = qvariant_cast<QJsonObject>(it->at(0));

        JsonEndpoint *endpoint = qobject_cast<JsonEndpoint *>(qvariant_cast<QObject *>(it->at(1)));
        QVERIFY(endpoints.removeOne(endpoint));

        QVERIFY(msg.value("endpoint").isString() && msg.value("endpoint").toString() == endpoint->name());
        QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
        QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 100);
        QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
        QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
        QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
        QVERIFY(msg.value("array").isArray());
        QJsonArray array = msg.value("array").toArray();
        QVERIFY(array.size() == 3);
        QVERIFY(array.at(0).toString() == "one");
        QVERIFY(array.at(1).toString() == "two");
        QVERIFY(array.at(2).toString() == "three");
        QVERIFY(msg.value("object").isObject());
        QJsonObject object = msg.value("object").toObject();
        QVERIFY(object.value("item1").toString() == "This is item 1");
        QVERIFY(object.value("item2").toString() == "This is item 2");
    }

    c.closeConnection();

    child.waitForFinished();
}

void tst_JsonConnection::multipleThreadTest()
{
    const int knThreadCount = 5;
    const int knMessagesPerThread = 10;
    bool differentThreads = true;  // set to false to see if errors are due to threading

    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    ConnectionContainer c(socketname,true);

    QList<EndpointContainer *> containers;
    QList<QSignalSpy *> spies;
    QStringList endpointNames;
    for (int i = 0; i < knThreadCount; i++) {
        QThread *newThread = 0;
        if (differentThreads)
            newThread = new QThread(&c);
        EndpointContainer *ec = new EndpointContainer();
        ec->setConnection(c.connection());
        if (differentThreads)
            ec->moveToThread(newThread);
        containers.append(ec);

        QString endpointName("test");
        endpointName += QString::number(i);
        endpointNames.append(endpointName);

        JsonEndpoint *endpoint = ec->addEndpoint(endpointName);
        QVERIFY(endpoint->name() == endpointName);

        QSignalSpy *spy = new QSignalSpy(ec, SIGNAL(messageReceived(QJsonObject,QObject*)));
        spies.append(spy);

        if (differentThreads)
            newThread->start();
    }

    QVERIFY(c.connection()->state() == JsonConnection::Unconnected);
    c.doConnect();
    QVERIFY(c.connection()->state() == JsonConnection::Connected);

    // grouped
    c.sendMessage(endpointNames, knMessagesPerThread, true);

    for (int spyCount = 0; spyCount < spies.count(); ++spyCount) {
        QSignalSpy *spy = spies.at(spyCount);
        waitForSpy(*spy, knMessagesPerThread);
        for (int i = 0; i < spy->count(); ++i) {
            QJsonObject msg = qvariant_cast<QJsonObject>(spy->at(i).at(0));
//            qDebug() << "YYY: msg endpoint = " << msg.value("endpoint").toString() << "  should be: " << endpointNames.at(spyCount);
            QVERIFY(msg.value("endpoint").toString() == endpointNames.at(spyCount));
        }
//        qDebug() << "YYY    done spy -- count = " << spy->count();
    }

    // staggered

    foreach (QSignalSpy *spy, spies)
        spy->clear();

    c.sendMessage(endpointNames, knMessagesPerThread, false);

    for (int spyCount = 0; spyCount < spies.count(); ++spyCount) {
        QSignalSpy *spy = spies.at(spyCount);
        waitForSpy(*spy, knMessagesPerThread);
        for (int i = 0; i < spy->count(); ++i) {
            QJsonObject msg = qvariant_cast<QJsonObject>(spy->at(i).at(0));
//            qDebug() << "YYY: msg endpoint = " << msg.value("endpoint").toString() << "  should be: " << endpointNames.at(spyCount);
            QVERIFY(msg.value("endpoint").toString() == endpointNames.at(spyCount));
        }
//        qDebug() << "YYY    done spy -- count = " << spy->count();
    }

    c.closeConnection();

    child.waitForFinished();

    qDeleteAll(containers);
    qDeleteAll(spies);
}

void tst_JsonConnection::autoreconnectTest()
{
    QString socketname = "/tmp/tst_socket";

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    QSignalSpy spy0(&child, SIGNAL(serverReady()));
    waitForSpy(spy0, 1);

    ConnectionContainer c(socketname,true);
    c.connection()->setAutoReconnectEnabled(true);

    JsonEndpoint *endpoint = c.addEndpoint("test");
    QVERIFY(endpoint->name() == "test");

    QVERIFY(c.connection()->state() == JsonConnection::Unconnected);
    c.doConnect();
    QVERIFY(c.connection()->state() == JsonConnection::Connected);

    QJsonObject msg;
    msg.insert("endpoint", QLatin1String("test"));
    msg.insert("text", QLatin1String("Disconnect message"));
    msg.insert("command", QLatin1String("disconnect"));
    msg.insert("timeout", 2000); // reconnect after 2 secs
    endpoint->send(msg);

    QSignalSpy spy(&c, SIGNAL(messageReceived(QJsonObject,QObject *)));
    waitForSpy(spy, 1);

    msg = qvariant_cast<QJsonObject>(spy.last().at(0));

    QVERIFY(msg.value("endpoint").isString() && msg.value("endpoint").toString() == endpoint->name());
    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Disconnect message"));
    QVERIFY(msg.value("timeout").isDouble() && msg.value("timeout").toDouble() == 2000);

    // wait for disconnect -> connencting state
    QSignalSpy spy1(c.connection(), SIGNAL(stateChanged(JsonConnection::State)));
    waitForSpy(spy1, 1);
    JsonConnection::State state = qvariant_cast<JsonConnection::State>(spy1.last().at(0));
    QVERIFY(state == JsonConnection::Connecting);
    QVERIFY(c.connection()->state() == JsonConnection::Connecting);

    // wait for reconnection
    waitForSpy(spy1, 2, 10000);
    state = qvariant_cast<JsonConnection::State>(spy1.last().at(0));
    QVERIFY(state == JsonConnection::Connected);
    QVERIFY(c.connection()->state() == JsonConnection::Connected);

    // send a new message after reconnection and wait for a reply
    msg = QJsonObject();
    msg.insert("endpoint", QLatin1String("test"));
    msg.insert("text", QLatin1String("New message"));
    endpoint->send(msg);

    QSignalSpy spy2(&c, SIGNAL(messageReceived(QJsonObject,QObject *)));
    waitForSpy(spy2, 1);

    msg = qvariant_cast<QJsonObject>(spy2.last().at(0));

    QVERIFY(msg.value("endpoint").isString() && msg.value("endpoint").toString() == endpoint->name());
    QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("New message"));

    c.closeConnection();

    child.waitForFinished();
}

QTEST_MAIN

(tst_JsonConnection)

#include "tst_jsonconnection.moc"

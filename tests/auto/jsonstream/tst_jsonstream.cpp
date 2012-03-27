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
#include "jsonstream.h"
#include "jsonpipe.h"
#include "jsonuidauthority.h"
#include "jsonuidrangeauthority.h"
#include "schemavalidator.h"

QT_USE_NAMESPACE_JSONSTREAM

Q_DECLARE_METATYPE(QJsonObject);

class Spy {
public:
    Spy(JsonServer *server)
        : addedSpy(server, SIGNAL(connectionAdded(const QString&)))
        , removedSpy(server, SIGNAL(connectionRemoved(const QString&)))
        , receivedSpy(server, SIGNAL(messageReceived(const QString&, const QJsonObject&)))
        , failedSpy(server, SIGNAL(authorizationFailed()))
    {}

    void waitAdded(int timeout=5000) {
        int oldCount = addedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (addedSpy.count() > oldCount)
                break;
        }
    }

    void waitRemoved(int timeout=5000) {
        int oldCount = removedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (removedSpy.count() > oldCount)
                break;
        }
    }

    void waitReceived(int timeout=5000) {
        int oldCount = receivedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (receivedSpy.count() > oldCount)
                break;
        }
    }

    void waitFailed(int timeout=5000) {
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (failedSpy.count())
                break;
        }
    }

    QString     lastSender()  { return receivedSpy.last().at(0).toString(); }
    QJsonObject lastMessage() { return qvariant_cast<QJsonObject>(receivedSpy.last().at(1));}

    QTime      stopWatch;
    QSignalSpy addedSpy, removedSpy, receivedSpy, failedSpy;
};

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

private:
    QProcess *process;
};

/****************************/
class BasicServer : public QObject {
    Q_OBJECT

public:
    BasicServer(const QString& socketname, qint64 _sz = 0, bool _handleReadBufOverflow = false)
        : socket(0), stream(0), readBufferSize(_sz)
        , mHandleReadBufOverflow(_handleReadBufOverflow), mLastError(JsonStream::NoError)
    {
        QLocalServer::removeServer(socketname);
        server = new QLocalServer(this);
        connect(server, SIGNAL(newConnection()), SLOT(handleConnection()));
        QVERIFY(server->listen(socketname));
    }

    ~BasicServer() {
        QVERIFY(server);
        delete server;
        server = NULL;
    }

    bool send(const QJsonObject& message) {
        return stream ? stream->send(message) : false;
    }

    void waitDisconnect(int timeout=5000) {
        QTime stopWatch;
        stopWatch.start();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (!socket)
                break;
        }
    }

    EncodingFormat format() {
        return stream->format();
    }

    JsonStream   *jsonStream() const { return stream; }
private slots:
    void handleConnection() {
        socket = server->nextPendingConnection();
        QVERIFY(socket);
        stream = new JsonStream(socket);
        stream->setParent(socket);
        if (readBufferSize > 0)
            stream->setReadBufferSize(readBufferSize);
        connect(socket, SIGNAL(disconnected()), SLOT(handleDisconnection()));
        connect(stream, SIGNAL(readyReadMessage()), SLOT(processMessages()));
        connect(stream, SIGNAL(readBufferOverflow(qint64)), SLOT(handleReadBufferOverflow(qint64)));
    }

    void processMessages() {
        while (stream->messageAvailable()) {
            QJsonObject obj = stream->readMessage();
            if (!obj.isEmpty())
                emit messageReceived(obj);
        }
    }

    void handleDisconnection() {
        QVERIFY(socket);
        socket->deleteLater();
        socket = NULL;
        mLastError = stream->lastError();
        stream = NULL;
    }

    void handleReadBufferOverflow(qint64 sz) {
        if (mHandleReadBufOverflow) {
            QVERIFY(readBufferSize > 0 && sz > readBufferSize);
            stream->setReadBufferSize(sz);
        }
        emit readBufferOverflow(sz);
    }

signals:
    void messageReceived(const QJsonObject&);
    void readBufferOverflow(qint64);

private:
    QLocalServer *server;
    QLocalSocket *socket;
    JsonStream   *stream;
    qint64        readBufferSize;
    bool          mHandleReadBufOverflow;
public:
    JsonStream::JsonStreamError mLastError;
};

/****************************/

class tst_JsonStream : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void noAuthTest();
    void authTest();
    void authFail();
    void authRangeTest();
    void authRangeFail();
    void formatTest();
    void schemaTest();
    void pipeTest();
    void pipeFormatTest();
    void pipeWaitTest();
    void bufferSizeTest();
    void bufferMaxReadSizeFailTest();
};

void tst_JsonStream::initTestCase()
{
    qRegisterMetaType<QJsonObject>();
    qRegisterMetaType<JsonPipe::PipeError>("PipeError");
}


void tst_JsonStream::noAuthTest()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    QVERIFY(server.listen(socketname));

    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();
    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
}

void tst_JsonStream::authTest()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDAuthority *authority = new JsonUIDAuthority;
    QVERIFY(server.listen(socketname, authority));

    authority->authorize(geteuid());
    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();
    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
    delete authority;
}

void tst_JsonStream::authFail()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDAuthority *authority = new JsonUIDAuthority;
    QVERIFY(server.listen(socketname, authority));

    // authority->authorize(geteuid());
    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitFailed();
    child.waitForFinished();
    delete authority;
}


void tst_JsonStream::authRangeTest()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDRangeAuthority *authority = new JsonUIDRangeAuthority;
    authority->setMinimum(geteuid());
    authority->setMaximum(geteuid());

    QVERIFY(server.listen(socketname, authority));

    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();
    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
    delete authority;
}

void tst_JsonStream::authRangeFail()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDRangeAuthority *authority = new JsonUIDRangeAuthority;
    QVERIFY(server.listen(socketname, authority));

    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitFailed();
    child.waitForFinished();
    delete authority;
}


void tst_JsonStream::formatTest()
{
    QString socketname = "/tmp/tst_socket";

    QStringList formats = QStringList() << "qbjs" << "bson" << "utf8" << "utf16be" << "utf16le";

    foreach (const QString& format, formats) {
        BasicServer server(socketname);
        QSignalSpy spy(&server, SIGNAL(messageReceived(const QJsonObject&)));
        QTime stopWatch;

        Child child("testClient/testClient",
                    QStringList() << "-socket" << socketname << "-format" << format);

        stopWatch.start();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= 5000)
                QFAIL("Timed out");
            if (spy.count())
                break;
        }

        if (format == "qbjs")
            QVERIFY(server.format() == FormatQBJS);
        else if (format == "bson")
            QVERIFY(server.format() == FormatBSON);
        else if (format == "utf8")
            QVERIFY(server.format() == FormatUTF8);
        else if (format == "utf16be")
            QVERIFY(server.format() == FormatUTF16BE);
        else if (format == "utf16le")
            QVERIFY(server.format() == FormatUTF16LE);
        else
            QFAIL("Unrecognized format");

        QJsonObject msg = qvariant_cast<QJsonObject>(spy.last().at(0));
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

        msg.insert("command", QLatin1String("exit"));
        QVERIFY(server.send(msg));
        server.waitDisconnect();
        child.waitForFinished();
    }
}

void tst_JsonStream::bufferSizeTest()
{
    QString socketname = "/tmp/tst_socket";

    BasicServer server(socketname, 100, true);
    QSignalSpy spy(&server, SIGNAL(messageReceived(const QJsonObject&)));
    QSignalSpy spy1(&server, SIGNAL(readBufferOverflow(qint64)));
    QTime stopWatch;

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    stopWatch.start();
    forever {
        QTestEventLoop::instance().enterLoop(1);
        if (stopWatch.elapsed() >= 5000)
            QFAIL("Timed out");
        if (spy.count())
            break;
    }
    QVERIFY(spy1.count() == 1); // overflow happend only once

    QJsonObject msg = qvariant_cast<QJsonObject>(spy.last().at(0));
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

    msg.insert("command", QLatin1String("exit"));

    server.jsonStream()->setWriteBufferSize(100);
    QVERIFY(!server.send(msg));
    QVERIFY(server.jsonStream()->lastError() == JsonStream::MaxWriteBufferSizeExceeded);

    QString strLarge(500000, '*');
    msg.insert("large", strLarge);
    msg.insert("large_size", strLarge.size());
    server.jsonStream()->setWriteBufferSize(0);
    QVERIFY(server.send(msg));
    QVERIFY(server.jsonStream()->lastError() == JsonStream::NoError);

    server.waitDisconnect();
    child.waitForFinished();
}

void tst_JsonStream::bufferMaxReadSizeFailTest()
{
    QString socketname = "/tmp/tst_socket";

    BasicServer server(socketname, 100);
    QSignalSpy spy1(&server, SIGNAL(readBufferOverflow(qint64)));
    QTime stopWatch;

    Child child("testClient/testClient",
                QStringList() << "-socket" << socketname);

    stopWatch.start();
    forever {
        QTestEventLoop::instance().enterLoop(1);
        if (stopWatch.elapsed() >= 5000)
            QFAIL("Timed out");
        if (spy1.count()) {
            break;
        }
    }

    QVERIFY(!server.jsonStream()); //disconnected
    QVERIFY(spy1.count() == 1); // overflow happend only once
    QVERIFY(server.mLastError == JsonStream::MaxReadBufferSizeExceeded);
}

void tst_JsonStream::schemaTest()
{
    QString strSchemasDir(QDir::currentPath() + "/" + "schemas");
    QVERIFY(QFile::exists(strSchemasDir));

    QString socketname = "/tmp/tst_socket";
    JsonServer server;

    QVERIFY(server.inboundValidator());
    QVERIFY(server.outboundValidator());

    server.setValidatorFlags(JsonServer::ValidatorFlags(JsonServer::WarnIfInvalid | JsonServer::DropIfInvalid));
    server.inboundValidator()->loadFromFolder(strSchemasDir);
    server.inboundValidator()->setValidationFilter(QRegExp("Paint\\w+Event|BackgroundEvent"));
    server.inboundValidator()->setSchemaNameMatcher(SchemaValidator::SchemaUniqueKeyNameMatcher("event"));


    server.outboundValidator()->loadFromFolder(strSchemasDir);
    server.outboundValidator()->setValidationFilter(QRegExp("Reply\\w+"));
    server.outboundValidator()->setSchemaNameMatcher(SchemaValidator::SchemaUniqueKeyNameMatcher("event"));

    QVERIFY(!server.inboundValidator()->isEmpty());
    QVERIFY(!server.outboundValidator()->isEmpty());

    Spy spy(&server);
    QVERIFY(server.listen(socketname));

    Child child("testClient/testClient", QStringList() << "-socket" << socketname << "-schema");

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();

    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
}

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


class Pipes {
public:
    Pipes() {
        ::pipe(fd1);
        ::pipe(fd2);
    }
    ~Pipes() {
        ::close(fd1[0]);
        ::close(fd1[1]);
        ::close(fd2[0]);
        ::close(fd2[1]);
    }
    void join(JsonPipe& jp1, JsonPipe& jp2) {
        // fd1[0] = Read end of jp1    fd1[1] = Write end of jp2
        // fd2[0] = Read end of jp2    fd2[1] = Write end of jp1
        jp1.setFds(fd1[0], fd2[1]);
        jp2.setFds(fd2[0], fd1[1]);
    }
    int fd1[2], fd2[2];
};

class PipeSpy {
public:
    PipeSpy(JsonPipe& jp)
        : msg(&jp, SIGNAL(messageReceived(const QJsonObject&)))
        , err(&jp, SIGNAL(error(PipeError))) {}
    QJsonObject at(int i) { return qvariant_cast<QJsonObject>(msg.at(i).at(0)); }
    QJsonObject last()    { return qvariant_cast<QJsonObject>(msg.last().at(0)); }
    QSignalSpy msg, err;
};

void tst_JsonStream::pipeTest()
{
    Pipes pipes;
    JsonPipe jpipe1, jpipe2;

    QVERIFY(!jpipe1.writeEnabled());
    QVERIFY(!jpipe1.readEnabled());
    QVERIFY(!jpipe2.writeEnabled());
    QVERIFY(!jpipe2.readEnabled());

    pipes.join(jpipe1, jpipe2);

    QVERIFY(jpipe1.writeEnabled());
    QVERIFY(jpipe1.readEnabled());
    QVERIFY(jpipe2.writeEnabled());
    QVERIFY(jpipe2.readEnabled());

    PipeSpy spy1(jpipe1);
    PipeSpy spy2(jpipe2);

    QJsonObject msg;
    msg.insert("name", QStringLiteral("Fred"));
    QVERIFY(jpipe1.send(msg));
    waitForSpy(spy2.msg, 1);
    QCOMPARE(spy2.at(0).value("name").toString(), QStringLiteral("Fred"));
}

void tst_JsonStream::pipeFormatTest()
{
    QList<EncodingFormat> formats = QList<EncodingFormat>() << FormatUTF8 << FormatBSON << FormatQBJS << FormatUTF16BE << FormatUTF16LE;

    foreach (EncodingFormat format, formats) {
        Pipes pipes;
        JsonPipe jpipe1, jpipe2;
        pipes.join(jpipe1, jpipe2);
        PipeSpy spy(jpipe2);
        jpipe1.setFormat(format);
        QCOMPARE(jpipe1.format(), format);

        QJsonObject msg;
        msg.insert("name", QStringLiteral("Fred"));
        QVERIFY(jpipe1.send(msg));
        waitForSpy(spy.msg, 1);
        QCOMPARE(spy.at(0).value("name").toString(), QStringLiteral("Fred"));
        QCOMPARE(jpipe2.format(), format);
    }
}

void tst_JsonStream::pipeWaitTest()
{
    Pipes pipes;
    JsonPipe jpipe1, jpipe2;
    pipes.join(jpipe1, jpipe2);

    QJsonObject msg;
    msg.insert("name", QStringLiteral("Jabberwocky"));
    QVERIFY(jpipe1.send(msg));
    QVERIFY(jpipe1.waitForBytesWritten());   // Actually push it out

    ::close(pipes.fd2[1]);  // Close the write end of jp1
    QVERIFY(jpipe1.send(msg));
    QVERIFY(!jpipe1.waitForBytesWritten());
}

QTEST_MAIN(tst_JsonStream)

#include "tst_jsonstream.moc"

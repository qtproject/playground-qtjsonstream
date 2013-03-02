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

#include <QCoreApplication>
#include <QStringList>
#include <QDebug>
#include <QJsonArray>

#include <QLocalSocket>
#include <QLocalServer>
#include <QDebug>
#include <QTimer>
#include <QFile>

#include "qjsonstream.h"
#include "qjsonserver.h"

QT_USE_NAMESPACE_JSONSTREAM

QString gSocketname = "/tmp/tst_jsonstream";

/****************************/
class BasicServer : public QObject {
    Q_OBJECT

public:
    BasicServer(const QString& socketname)
        : socket(0), stream(0)
    {
        QLocalServer::removeServer(socketname);
        QFile::remove(socketname);
        server = new QLocalServer(this);
        connect(server, SIGNAL(newConnection()), SLOT(handleConnection()));
        Q_ASSERT(server->listen(socketname));

        QTimer::singleShot(100, this, SLOT(ready()));
    }

    ~BasicServer() {
        Q_ASSERT(server);
        delete server;
        server = NULL;
    }

    bool send(const QJsonObject& message) {
        return stream ? stream->send(message) : false;
    }

    void disconnectFromServer(int timeout)
    {
        QTimer::singleShot(timeout, this, SLOT(doDisconnect()));
    }

private slots:
    void ready()
    {
        fprintf(stdout, "READY\n"); // send ready command
        fflush(stdout);
    }

    void handleConnection() {
        socket = server->nextPendingConnection();
        Q_ASSERT(socket);
        stream = new QJsonStream(socket);
        stream->setParent(socket);
        connect(socket, SIGNAL(disconnected()), SLOT(handleDisconnection()));
        connect(stream, SIGNAL(readyReadMessage()), SLOT(processMessages()));
    }

    void processMessages() {
        while (stream->messageAvailable()) {
            QJsonObject obj = stream->readMessage();
            if (!obj.isEmpty())
                emit messageReceived(obj);
        }
    }

    void doDisconnect() {
        // disconnect and wait for a new connection
        if (socket) {
            socket->disconnect(this);
            socket->disconnectFromServer();
            socket->deleteLater();
            stream->deleteLater();
            socket = NULL;
            stream = NULL;
        }
    }


    void handleDisconnection() {
        Q_ASSERT(socket);
        socket->deleteLater();
        socket = NULL;
        stream = NULL;

        exit(0);
    }

signals:
    void messageReceived(const QJsonObject&);

private:
    QLocalServer *server;
    QLocalSocket *socket;
    QJsonStream   *stream;
};

class Container : public QObject
{
    Q_OBJECT

public:
    Container();
    void sendMessage(const QString &endpoint = QString::null);

public slots:
    void received(const QJsonObject& message);

private:
    BasicServer *mServer;
    int         mCounter;
};

Container::Container()
    : mCounter(0)
{
    qDebug() << "Creating new json server at" << gSocketname;
    mServer = new BasicServer(gSocketname);
    connect(mServer, SIGNAL(messageReceived(const QJsonObject&)),
            SLOT(received(const QJsonObject&)));
}

void Container::sendMessage(const QString &endpoint)
{
    static int counter = 0;

    QJsonObject msg;
    msg.insert("counter", counter++);
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

    if (!endpoint.isEmpty())
        msg.insert("endpoint", endpoint);

    mServer->send(msg);
}

void Container::received(const QJsonObject& message)
{
    qDebug() << "received " << message;

    QString command = message.value("command").toString();
    if (!command.isEmpty()) {
        qDebug() << "Received command" << command;
        if (command == "exit")
            exit(0);
        else if (command == "crash")
            exit(1);
        else if (command == "reply")
            sendMessage(message.value("endpoint").toString());
        else if (command == "flurry") {
            QJsonArray endpoints = message.value("endpoints").toArray();
            int msgsPerEndpoint = static_cast<int> (message.value("count").toDouble());
            int nEndpoints = endpoints.size() ? endpoints.size() : 1;
            bool grouped = message.value("grouped").toBool();
            for (int count = msgsPerEndpoint * nEndpoints; count ; --count ) {
                QString endpoint;
                if (nEndpoints > 0) {
                    int idx = grouped ? ((count-1) / msgsPerEndpoint) : ((count-1) % nEndpoints);
//                    qDebug() << "ZZZ idx = " << idx;
                    endpoint = endpoints.at(idx).toString();
                }
//                qDebug() << "ZZZ     sending message for " << endpoint;
                sendMessage(endpoint);
            }
        }
        else if (command == "disconnect") {
            // send message back first
            mServer->send(message);

            int timeout = message.value("timeout").toDouble();
            mServer->disconnectFromServer(timeout);
        }
    }
    else {
        // send message back
        mServer->send(message);
    }
}

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = QCoreApplication::arguments();
    QString progname = args.takeFirst();
    while (args.size()) {
        QString arg = args.at(0);
        if (!arg.startsWith("-"))
            break;
        args.removeFirst();
        if (arg == "-socket")
            gSocketname = args.takeFirst();
    }

    Container c;
    return app.exec();
}

#include "main.moc"

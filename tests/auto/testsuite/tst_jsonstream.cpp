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

#include <QtTest/QtTest>

#include "tst_jsonclient.h"
#include "tst_jsonserver.h"

class tst_JsonStream : public QObject
{
    Q_OBJECT

private slots:
    void noAuthTest();
    void pidAuthorityTest();
    void tokenAuthorityTest();

private:
    void waitResult(tst_JsonServer *);

};

void tst_JsonStream::noAuthTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;
    QString socketname = "/tmp/tst_socket";
    QString message = "foobar";

    tst_JsonServer *server = new tst_JsonServer(socketname);
    server->noAuthTest();

    tst_JsonClient *client = new tst_JsonClient(socketname, message);

    waitResult(server);

    delete server;

    qDebug() << Q_FUNC_INFO << " Completed";
}

void tst_JsonStream::pidAuthorityTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;
 /*   QString socketname = "/tmp/tst_socket";
    QString message = "foobar";

    tst_JsonServer *server = new tst_JsonServer(socketname);
    server->pidAuthorityTest();

    tst_JsonClient *client = new tst_JsonClient(socketname, message);

    waitResult(server);

    delete server;*/

    qDebug() << Q_FUNC_INFO << " Completed";
}

void tst_JsonStream::tokenAuthorityTest()
{
    qDebug() << "Running" << Q_FUNC_INFO;
    QString socketname = "/tmp/tst_socket";
    QString message = "foobar";

    tst_JsonServer *server = new tst_JsonServer(socketname);
    server->tokenAuthorityTest();

    tst_JsonClient *client = new tst_JsonClient(socketname, message);

    waitResult(server);

    delete server;

    qDebug() << Q_FUNC_INFO << " Completed";
}

void tst_JsonStream::waitResult(tst_JsonServer *server)
{
    qDebug() << Q_FUNC_INFO;

    QTime stopWatch;
    stopWatch.start();
    forever {
    QTestEventLoop::instance().enterLoop(1);
    if (stopWatch.elapsed() >= 5000)
        QFAIL("Timed out");
    if (!server->note().isEmpty())
        break;
    }

    stopWatch.restart();
    forever {
    qDebug() << "Waiting for process to enter";
    QTestEventLoop::instance().enterLoop(1);
    if (stopWatch.elapsed() >= 5000)
        QFAIL("Timed out waiting for client to disconnect");
    if (server->id().isEmpty())
        break;
    }
}

QTEST_MAIN(tst_JsonStream)

#include "tst_jsonstream.moc"

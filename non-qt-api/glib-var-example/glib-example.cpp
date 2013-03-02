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

#include "glib-example.h"
#include "jsonclient.h"
#include "variant.h"

#include <glib.h>
#include <glib-object.h>
#include <sigc++/sigc++.h>
#include <sigc++/functors/mem_fun.h>

#include <stdlib.h>

std::string gSocketname = "/tmp/tst_jsonstream";

class Container
{
public:
    Container();
    void sendMessage();

private:
    void received(const std::string& message);
    void receivedMap(GHashTable *);
    void receivedVariantMap(const VariantMap & message);

private:
    JsonClient *mClient;
    int         mCounter;
};

Container::Container()
    : mCounter(0)
{
    g_print("Creating new json client\n");

    mClient = new JsonClient;
    mClient->messageReceivedVariantMap.connect( sigc::mem_fun(this, &Container::receivedVariantMap) );

    if (!mClient->connectLocal(gSocketname))
    {
        g_print("Unable to connect to %s\n", gSocketname.c_str());
        exit(2);
    }
}

/*!
  Prepares data in VariantMap object and sends it using JsonClient.
  It's a JsonClient responsibility to convert data into JSON format.
*/
void Container::sendMessage()
{
    VariantMap msg;
    msg.insert("text", "Standard text");
    msg.insert("number", mCounter++);
    msg.insert("int", 100);
    msg.insert("float", 100.0);
    msg.insert("true", true);
    msg.insert("false", false);
    msg.insert("array", VariantList() << "one" << "two" << "three");
    VariantMap obj;
    obj.insert("item1", "This is item 1");
    obj.insert("item2", "This is item 2");
    msg.insert("object", obj);

    mClient->send(msg);
}

/*!
  Processes a reply in VariantMap object.
*/
void Container::receivedVariantMap(const VariantMap& message)
{
    std::string command = message.value("command").toString();
    if (!command.empty()) {
        g_print("Received command %s\n", command.c_str());
        if (command == "exit")
            exit(0);
        else if (command == "crash")
            exit(1);
        else if (command == "reply")
            sendMessage();
        else if (command == "flurry") {
            for (int count = message.value("count").toInt() ; count ; --count )
            sendMessage();
        }
    }
}

int
main(int argc, char **argv)
{
    g_type_init();

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-socket")
        {
            if (i < argc - 1)
                gSocketname = argv[++i];
            break;
        }
    }

    g_print("socketname [%s]\n", gSocketname.c_str());

    Container c;
    c.sendMessage();

    g_main_loop_run(loop);

    return 0;
 }

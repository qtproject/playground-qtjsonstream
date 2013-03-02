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

#include <glib.h>
#include <glib-object.h>
#include <sigc++/sigc++.h>
#include <sigc++/functors/mem_fun.h>

#include <json-glib/json-glib.h>

#include <stdlib.h>

#ifndef G_VALUE_INIT
#define G_VALUE_INIT  { 0, { { 0 } } }
#endif

std::string gSocketname = "/tmp/tst_jsonstream";

class Container
{
public:
    Container();
    void sendMessage();
    void sendMessageMap();

private:
    void received(const std::string& message);
    void receivedMap(GHashTable *);

private:
    JsonClient *mClient;
    int         mCounter;
};

Container::Container()
    : mCounter(0)
{
    g_print("Creating new json client\n");

    mClient = new JsonClient;
//    mClient->messageReceived.connect( sigc::mem_fun(this, &Container::received) );
    mClient->messageReceivedMap.connect( sigc::mem_fun(this, &Container::receivedMap) );

    if (!mClient->connectLocal(gSocketname))
    {
        g_print("Unable to connect to %s\n", gSocketname.c_str());
        exit(2);
    }
}

/*!
  Prepares data in GHashTable object and sends it using JsonClient.
  It's a JsonClient responsibility to convert data into JSON format.
*/
void Container::sendMessageMap()
{
    GHashTable *dict = g_hash_table_new(g_str_hash, g_str_equal);

    GValue v = G_VALUE_INIT;
    g_value_init (&v, G_TYPE_STRING);
    g_value_set_static_string (&v, "Standard text");
    g_hash_table_insert(dict, const_cast<char*> ("text"), &v);

    GValue v0 = G_VALUE_INIT;
    g_value_init (&v0, G_TYPE_INT64);
    g_value_set_int64(&v0, mCounter++);
    g_hash_table_insert(dict, const_cast<char*> ("number"), &v0);

    GValue v1 = G_VALUE_INIT;
    g_value_init (&v1, G_TYPE_INT);
    g_value_set_int(&v1, 100);
    g_hash_table_insert(dict, const_cast<char*> ("int"), &v1);

    GValue v2 = G_VALUE_INIT;
    g_value_init (&v2, G_TYPE_DOUBLE);
    g_value_set_double(&v2, 100.);
    g_hash_table_insert(dict, const_cast<char*> ("float"), &v2);

    GValue v3 = G_VALUE_INIT;
    g_value_init (&v3, G_TYPE_BOOLEAN);
    g_value_set_boolean(&v3, true);
    g_hash_table_insert(dict, const_cast<char*> ("true"), &v3);

    GValue v4 = G_VALUE_INIT;
    g_value_init (&v4, G_TYPE_BOOLEAN);
    g_value_set_boolean(&v4, false);
    g_hash_table_insert(dict, const_cast<char*> ("false"), &v4);

    GValueArray *array = g_value_array_new(0);

    GValue va = G_VALUE_INIT, tVA = G_VALUE_INIT, tVO = G_VALUE_INIT;
    g_value_init (&va, G_TYPE_STRING);
    g_value_set_static_string (&va, "one");
    g_value_array_append(array, &va);
    g_value_unset(&va);

    g_value_init (&va, G_TYPE_STRING);
    g_value_set_static_string (&va, "two");
    g_value_array_append(array, &va);
    g_value_unset(&va);

    g_value_init (&va, G_TYPE_STRING);
    g_value_set_static_string (&va, "three");
    g_value_array_append(array, &va);
    g_value_unset(&va);

    g_value_init(&tVA, G_TYPE_VALUE_ARRAY);
    g_value_set_boxed(&tVA, array);
    g_hash_table_insert(dict, const_cast<char*> ("array"), &tVA);

    GHashTable *object = g_hash_table_new(g_str_hash, g_str_equal);

    GValue vo = G_VALUE_INIT, vo1 = G_VALUE_INIT;
    g_value_init (&vo, G_TYPE_STRING);
    g_value_set_static_string (&vo, "This is item 1");
    g_hash_table_insert(object, const_cast<char*> ("item1"), &vo);

    g_value_unset(&vo1);
    g_value_init (&vo1, G_TYPE_STRING);
    g_value_set_static_string (&vo1, "This is item 2");
    g_hash_table_insert(object, const_cast<char*> ("item2"), &vo1);

    g_value_init(&tVO, G_TYPE_HASH_TABLE);
    g_value_set_boxed(&tVO, object);
    g_hash_table_insert(dict, const_cast<char*> ("object"), &tVO);

    mClient->send(dict);

    g_value_unset(&tVA); g_value_unset(&tVO);
    g_hash_table_destroy(object); g_value_array_free(array);
    g_hash_table_destroy(dict);
}

/*!
  Prepares JSON data and sends it using JsonClient.
*/
void Container::sendMessage()
{
    JsonBuilder *builder = json_builder_new ();

    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "text");
    json_builder_add_string_value (builder, "Standard text");

    json_builder_set_member_name (builder, "number");
    json_builder_add_int_value (builder, mCounter++);

    json_builder_set_member_name (builder, "int");
    json_builder_add_int_value (builder, 100);

    json_builder_set_member_name (builder, "float");
    json_builder_add_double_value (builder, 100.0);

    json_builder_set_member_name (builder, "true");
    json_builder_add_boolean_value (builder, true);

    json_builder_set_member_name (builder, "false");
    json_builder_add_boolean_value (builder, false);

    json_builder_set_member_name (builder, "array");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "one");
    json_builder_add_string_value (builder, "two");
    json_builder_add_string_value (builder, "three");
    json_builder_end_array (builder);

    json_builder_set_member_name (builder, "object");
    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "item1");
    json_builder_add_string_value (builder, "This is item 1");

    json_builder_set_member_name (builder, "item2");
    json_builder_add_string_value (builder, "This is item 2");

    json_builder_end_object (builder);

    json_builder_end_object (builder);

    JsonGenerator *generator = json_generator_new ();
    json_generator_set_root (generator, json_builder_get_root (builder));
    gchar *str = json_generator_to_data (generator, 0);

    std::string msg(str);

    g_object_unref (generator);
    g_object_unref (builder);

    g_print("Container::sendMessage sending [%s]\n", msg.c_str());
    mClient->send(msg);
}

/*!
  Processes a reply in GHashTable object.
*/
void Container::receivedMap(GHashTable * dict)
{
    gpointer p = g_hash_table_lookup(dict, "command");
    if (p)
    {
        std::string command = g_value_get_string((GValue *)p);
        if (!command.empty()) {
            g_print("Received command %s\n", command.c_str());
            if (command == "exit")
                exit(0);
            else if (command == "crash")
                exit(1);
            else if (command == "reply")
                sendMessage();
            else if (command == "flurry") {
                if (NULL != (p = g_hash_table_lookup(dict, "count")))
                {
                    int count = g_value_get_int((GValue *)p);
                    for (int n = count ; n ; --n )
                        sendMessage();
                }
            }
        }
    }

    // free dict
}

/*!
  Processes a reply in JSON format.
*/
void Container::received(const std::string& msg)
{
    g_print("Container::received [%s]\n", msg.c_str());
    JsonParser *parser;
    GError *error = NULL;

    parser = json_parser_new ();

    std::string command;
    int count = 0;

    if (json_parser_load_from_data (parser, msg.c_str(), -1, &error))
    {
        JsonNode *root;
        JsonObject *object;

        g_assert (NULL != json_parser_get_root (parser));

        root = json_parser_get_root (parser);
        if (JSON_NODE_TYPE (root) == JSON_NODE_OBJECT)
        {
            object = json_node_get_object (root);
            g_assert (object != NULL);

            if (json_object_has_member(object, "command"))
            {
                JsonNode *node = json_object_get_member(object, "command");
                if (JSON_NODE_TYPE(node) == JSON_NODE_VALUE &&
                        json_node_get_value_type(node) == G_TYPE_STRING)
                {
                    command = json_node_get_string(node);
                }
            }

            if (json_object_has_member(object, "count"))
            {
                count = json_object_get_int_member(object, "count");
            }
        }
    }
    else
    {
        g_error_free (error);
    }
    g_object_unref (parser);

    if (!command.empty()) {
        g_print("Received command %s\n", command.c_str());
        if (command == "exit")
            exit(0);
        else if (command == "crash")
            exit(1);
        else if (command == "reply")
            sendMessage();
        else if (command == "flurry") {
            for (int n = count ; n ; --n )
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
    c.sendMessageMap();

    g_main_loop_run(loop);

    return 0;
 }

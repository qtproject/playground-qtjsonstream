/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "jsonclient.h"

#include <network.h>
#include <stdio.h>
#include <json-glib/json-glib.h>

#ifndef G_VALUE_INIT
#define G_VALUE_INIT  { 0, { { 0 } } }
#endif

static std::string toJson(GHashTable *dict);
static GHashTable* toHashTable(std::string);
static GValue *getValue(JsonNode *node);

inline bool isSignalConnected(const sigc::signal_base & sig)
{
    return sig.size() > 0;
}

/*!
    \class JsonClient
    \brief The JsonClient class is used to send jsons to the JsonServer.

    Note: The JsonClient is not thread safe.
*/

/*!
  Construct a new JsonClient instance with \a registration token.
  The token will be passed in \c{{"token": registration}}.  This constructor
  is designed to work with JsonTokenAuthority.
 */
JsonClient::JsonClient(const std::string& registration)
    : mSocket(0)
{
    mRegistrationMessage = "{\"token\": \"" + registration + "\" }";
}

/*!
  Construct a new JsonClient instance.
 */
JsonClient::JsonClient()
    : mSocket(0)
{
}

/*!
  Destroy the JsonClient and shut down any active stream.
 */
JsonClient::~JsonClient()
{
    setDevice(0);
    if (mSocket)
        delete mSocket;
}

/*!
  Connect to the JsonServer over a TCP socket at \a hostname and \a port. Send the initial registration message.
  Return true if the connection is successful.
*/

bool JsonClient::connectTCP(const std::string & hostname, int port)
{
    TcpSocket *socket = new TcpSocket();
    if (socket->connectToHost(hostname, port))
    {
        if (socket->waitForConnected())
        {
            setDevice(socket);
            g_print("Sending local socket registration message %s\n", mRegistrationMessage.c_str());
            send(!mRegistrationMessage.empty() ? mRegistrationMessage : "{ }");
            return true;
        }
    }
    delete socket;
    return false;
}

/*!
  Connect to the JsonServer over a Unix local socket to \a socketname and send the initial registration message.
  Return true if the connection is successful.
 */
bool JsonClient::connectLocal(const std::string & socketname)
{
    LocalSocket *socket = new LocalSocket();
    if (socket->connectToServer(socketname))
    {
        if (socket->waitForConnected())
        {
            setDevice(socket);
            g_print("Sending local socket registration message %s\n", mRegistrationMessage.c_str());
            send(!mRegistrationMessage.empty() ? mRegistrationMessage : "{ }");
            return true;
        }
    }
    delete socket;
    return false;
}

/*!
  Send a \a message over the socket.
*/
void JsonClient::send(const std::string & _message)
{
    if (mSocket->isOpen())
    {
        std::string message(_message + "\n");
        int nBytes = mSocket->write( message.c_str(), message.size() );
        g_print("wrote %d bytes into socket\n", nBytes);
    } else {
        g_print("stream socket is not available\n");
    }
}

/*!
  Send a \a message over the socket.
*/
void JsonClient::send(GHashTable *map)
{
    send(toJson(map));
}

/*!
  \internal
*/

/*!
    Set the \a device used by the JsonStream.
    Setting the device to 0 disconnects the stream but does not close
    the device nor flush it.

    The stream does not take ownership of the device.
*/
void JsonClient::setDevice( AbstractSocket *device )
{
    if (mSocket)
    {
        if (isSignalConnected(device->readyRead))
            mSocket->readyRead.slots().begin()->disconnect();
        if (isSignalConnected(device->aboutToClose))
            mSocket->aboutToClose.slots().begin()->disconnect();
        delete mSocket;
    }

    mSocket = device;
    if (device) {
        device->readyRead.connect( sigc::mem_fun(this, &JsonClient::dataReadyOnSocket) );
        device->aboutToClose.connect( sigc::mem_fun(this, &JsonClient::socketAboutToClose) );
    }
}

void JsonClient::dataReadyOnSocket()
{
    std::string str(mSocket->readAll());
    if (isSignalConnected(messageReceived))
        messageReceived.emit(str);
    if (isSignalConnected(messageReceivedMap))
        messageReceivedMap.emit(toHashTable(str));
}

void JsonClient::socketAboutToClose()
{
    aboutToClose.emit();
}

/*!
    \fn void JsonClient::aboutToClose()
    This signal is emitted when server closes a connection.
*/

/*!
    \fn void JsonClient::messageReceived(const std::string &message)
    This signal is emitted when a \a message is received from the server.
*/

/*!
    \fn void JsonClient::messageReceivedMap(GHashTable *message)
    This signal is emitted when a \a message is received from the server.
*/

static void insertToJson(gpointer name, gpointer v, gpointer user_data)
{
    JsonBuilder *builder = (JsonBuilder *)user_data;

    if (name != NULL)
        json_builder_set_member_name (builder, (const char *)name);

    switch (G_VALUE_TYPE(v)) {
    case G_TYPE_BOOLEAN:
        json_builder_add_boolean_value (builder, g_value_get_boolean((GValue *)v));
        break;
    case G_TYPE_STRING:
        json_builder_add_string_value (builder, g_value_get_string((GValue *)v));
        break;
    case G_TYPE_INT:
        json_builder_add_int_value (builder, g_value_get_int((GValue *)v));
        break;
    case G_TYPE_INT64:
        json_builder_add_int_value (builder, g_value_get_int64((GValue *)v));
        break;
    case G_TYPE_DOUBLE:
        json_builder_add_double_value (builder, g_value_get_double((GValue *)v));
        break;
    default:
        if (G_VALUE_TYPE(v) == G_TYPE_HASH_TABLE)
        {
            json_builder_begin_object (builder);
            GHashTable *map = (GHashTable *) g_value_get_boxed((GValue *)v);
            g_hash_table_foreach (map, insertToJson, builder);
            json_builder_end_object (builder);
        }
        else if (G_VALUE_TYPE(v) == G_TYPE_VALUE_ARRAY)
        {
            json_builder_begin_array (builder);
            GValueArray *array = (GValueArray *) g_value_get_boxed((GValue *)v);
            for (guint i = 0; i < array->n_values; i++)
            {
               GValue *value = g_value_array_get_nth(array, i);
               insertToJson(0, value, builder);
            }
            json_builder_end_array (builder);
        }
        else
        {
            g_print("unhandled type %s\n", G_VALUE_TYPE_NAME(v));
            json_builder_add_null_value (builder);
        }
        break;
    }
}

static std::string toJson(GHashTable *dict)
{
    fprintf(stderr, "JSONDocument::toJson\n");
    JsonBuilder *builder = json_builder_new ();

    json_builder_begin_object (builder);

    if (dict != NULL)
    {
        g_hash_table_foreach (dict, insertToJson, builder);
    }

    json_builder_end_object (builder);

    JsonGenerator *generator = json_generator_new ();
    json_generator_set_root (generator, json_builder_get_root (builder));
    gchar *str = json_generator_to_data (generator, 0);

    std::string msg(str);

    g_object_unref (generator);
    g_object_unref (builder);

    //g_print("[%s]\n", msg.c_str());

    return msg + "\n";
}

static void parseMembers(JsonObject *,
                         const gchar *member_name,
                         JsonNode    *member_node,
                         gpointer     user_data)
{
    GHashTable *map = static_cast<GHashTable *>(user_data);
    g_hash_table_insert(map, g_strdup(member_name), getValue(member_node));
}

static void parseElements(JsonArray *, guint, JsonNode *element_node, gpointer user_data)
{
    GValueArray *array = static_cast<GValueArray *>(user_data);
    g_value_array_append(array, getValue(element_node));
}

static GValue *getValue(JsonNode *node)
{
    GValue *p = new GValue;
    GValue &ret = *p;

    switch (JSON_NODE_TYPE(node))
    {
    case JSON_NODE_OBJECT:
        {
            JsonObject *node_object;
            node_object = json_node_get_object (node);
            g_assert (node_object != NULL);

            GHashTable *object = g_hash_table_new(g_str_hash, g_str_equal);
            json_object_foreach_member(node_object, parseMembers, object);

            g_value_init(&ret, G_TYPE_HASH_TABLE);
            g_value_set_boxed(&ret, object);
        }
        break;
    case JSON_NODE_ARRAY:
        {
            JsonArray *node_array;
            node_array = json_node_get_array (node);
            g_assert (node_array != NULL);

            GValueArray *array = g_value_array_new(0);
            json_array_foreach_element(node_array, parseElements, array);

            g_value_init(&ret, G_TYPE_VALUE_ARRAY);
            g_value_set_boxed(&ret, array);
        }
        break;
    case JSON_NODE_VALUE:
        {
            switch (json_node_get_value_type(node))
            {
            case G_TYPE_INT64:
                {
                    gint64 val = json_node_get_int(node);
                    g_value_init (&ret, G_TYPE_INT64);
                    g_value_set_int64(&ret, val);
                }
                break;

            case G_TYPE_DOUBLE:
                {
                    double val = json_node_get_double(node);
                    g_value_init (&ret, G_TYPE_INT64);
                    g_value_set_double(&ret, val);
                }
                break;

            case G_TYPE_BOOLEAN:
                {
                    bool val = json_node_get_boolean(node);
                    g_value_init (&ret, G_TYPE_INT64);
                    g_value_set_boolean(&ret, val);
                }
                break;

            case G_TYPE_STRING:
                {
                    const gchar * str = json_node_get_string(node);
                    g_value_init (&ret, G_TYPE_STRING);
                    g_value_set_string(&ret, str);
                }
                break;
            default:
                break;
            }
        }
        break;
    case JSON_NODE_NULL:
        break;
    }
    return &ret;
}

static GHashTable* toHashTable(std::string msg)
{
    g_print("toHashTable\n");
    JsonParser *parser;
    GError *error = NULL;

    parser = json_parser_new ();

    GHashTable *map = g_hash_table_new(g_str_hash, g_str_equal);

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

            json_object_foreach_member(object, parseMembers, map);
        }
    }
    else
    {
        g_error_free (error);
    }
    g_object_unref (parser);

    return map;
}

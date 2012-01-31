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

#include "jsonclient.h"
#include "variant.h"

#include <network.h>
#include <stdio.h>
#include <json-glib/json-glib.h>

#ifndef G_VALUE_INIT
#define G_VALUE_INIT  { 0, { { 0 } } }
#endif

static VariantMap toVariantMap(const std::string &);
static std::string toJson(const VariantMap & map);

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

void JsonClient::send(const VariantMap & map)
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
    if (isSignalConnected(messageReceivedVariantMap))
        messageReceivedVariantMap.emit(toVariantMap(str));
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
    \fn void JsonClient::messageReceivedMap(const VariantMap &message)
    This signal is emitted when a \a message is received from the server.
*/

//////////////////////////////////////////////////////////////////////////////
// VariantMap support methods
//////////////////////////////////////////////////////////////////////////////
static void insertToJson(const gchar  *name, const Variant &v, JsonBuilder *builder)
{
    if (name != NULL)
        json_builder_set_member_name (builder, name);

    switch (v.type()) {
    case Variant::Invalid:
        json_builder_add_null_value (builder);
        break;
    case Variant::Bool:
        json_builder_add_boolean_value (builder, v.toBool());
        break;
    case Variant::String:
        json_builder_add_string_value (builder, v.toString().c_str());
        break;
    case Variant::Int:
        json_builder_add_int_value (builder, v.toInt());
        break;
    case Variant::LongLong:
        json_builder_add_int_value (builder, v.toLongLong());
        break;
    case Variant::Double:
        json_builder_add_double_value (builder, v.toDouble());
        break;
    case Variant::Map:
        json_builder_begin_object (builder);
        {
            VariantMap map = v.toMap();
            for (VariantMap::const_iterator it = map.begin(); it != map.end(); ++it) {
                insertToJson(it->first.c_str(), it->second, builder);
            }
        }
        json_builder_end_object (builder);
        break;
    case Variant::List:
        json_builder_begin_array (builder);
        {
            VariantList list = v.toList();
            for (VariantList::const_iterator it = list.begin(); it != list.end(); ++it) {
                insertToJson(NULL, *it, builder);
            }
        }
        json_builder_end_array (builder);
        break;
    }
}

std::string toJson(const VariantMap & map)
{
    fprintf(stderr, "toJson(VariantMap)\n");
    JsonBuilder *builder = json_builder_new ();

    json_builder_begin_object (builder);

    for (VariantMap::const_iterator it = map.begin(); it != map.end(); ++it) {
        insertToJson(it->first.c_str(), it->second, builder);
    }

    json_builder_end_object (builder);

    JsonGenerator *generator = json_generator_new ();
    json_generator_set_root (generator, json_builder_get_root (builder));
    gchar *str = json_generator_to_data (generator, 0);

    std::string msg(str);

    g_object_unref (generator);
    g_object_unref (builder);

    return msg + "\n";
}

static Variant getVariant(JsonNode *node);

static void parseMembers2Variant(JsonObject  * /*object*/,
                                 const gchar *member_name,
                                 JsonNode    *member_node,
                                 gpointer     user_data)
{
    VariantMap *pMap = static_cast<VariantMap *>(user_data);

    pMap->insert(member_name, getVariant(member_node));
}

static void parseElements2Variant(JsonArray  * /*array*/,
                                  guint       /*index_*/,
                                  JsonNode   *element_node,
                                  gpointer    user_data)
{
    VariantList *pList = static_cast<VariantList *>(user_data);

    pList->append(getVariant(element_node));
}

static Variant getVariant(JsonNode *node)
{
    Variant v;
    switch (JSON_NODE_TYPE(node))
    {
    case JSON_NODE_OBJECT:
        {
            VariantMap map;
            JsonObject *node_object;
            node_object = json_node_get_object (node);
            g_assert (node_object != NULL);

            json_object_foreach_member(node_object, parseMembers2Variant, &map);
            v = Variant(map);
        }
        break;
    case JSON_NODE_ARRAY:
        {
            VariantList list;
            JsonArray *node_array;
            node_array = json_node_get_array (node);
            g_assert (node_array != NULL);

            json_array_foreach_element(node_array, parseElements2Variant, &list);

            v = Variant(list);
        }
        break;
    case JSON_NODE_VALUE:
        {
            switch (json_node_get_value_type(node))
            {
            case G_TYPE_INT64:
                {
                    gint64 val = json_node_get_int(node);
                    v = Variant(val);
                }
                break;

            case G_TYPE_DOUBLE:
                {
                    double val = json_node_get_double(node);
                    v = Variant(val);
                }
                break;

            case G_TYPE_BOOLEAN:
                {
                    bool val = json_node_get_boolean(node);
                    v = Variant(val);
                }
                break;

            case G_TYPE_STRING:
                {
                    const gchar * str = json_node_get_string(node);
                    v = Variant(str);
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
    return v;
}

VariantMap toVariantMap(const std::string &json)
{
    fprintf(stderr, "fromJson\n");
    JsonParser *parser;
    GError *error = NULL;

    parser = json_parser_new ();

    VariantMap map;

    if (json_parser_load_from_data (parser, json.c_str(), -1, &error))
    {
        JsonNode *root;
        JsonObject *object;

        g_assert (NULL != json_parser_get_root (parser));

        root = json_parser_get_root (parser);
        if (JSON_NODE_TYPE (root) == JSON_NODE_OBJECT)
        {
            object = json_node_get_object (root);
            g_assert (object != NULL);

            json_object_foreach_member(object, parseMembers2Variant, &map);
        }
    }
    else
    {
        g_error_free (error);
    }
    g_object_unref (parser);

    return map;
}

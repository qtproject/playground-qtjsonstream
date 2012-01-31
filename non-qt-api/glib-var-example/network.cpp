#include "network.h"

#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <gio/gunixsocketaddress.h>

static gboolean socket_has_data_cb (GSocket *socket, GIOCondition condition, void *socketclass)
{
    gboolean ret = TRUE;
    if (NULL != socketclass)
    {
        AbstractSocket *pSocket = static_cast<AbstractSocket *>(socketclass);
        ret = pSocket->dataReceived(socket, condition);
    }
    return ret;
}

/*!
    \class AbstractSocket
    \brief The AbstractSocket provide a base functionality common to all socket types.
*/
AbstractSocket::~AbstractSocket()
{
    if (socket)
        g_object_unref(socket);
}

std::string AbstractSocket::readAll()
{
    std::string buf;
    size_t nSz = bytesAvailable();
    if (nSz > 0)
    {
        buf.resize(nSz);
        read(buf.data(), nSz);
    }
    return buf;
}

gboolean AbstractSocket::dataReceived(GSocket *socket, GIOCondition condition)
{
    const gsize kBufSize = 2048;
    GError *error = NULL;
    gsize len;
    gchar buffer[kBufSize];
    gboolean ret = TRUE;

    if ((condition & G_IO_HUP) > 0) // socket was disconnected
    {
        aboutToClose.emit();
        ret = FALSE;
    }
    else if ((condition & G_IO_IN) > 0) /* there is data */
    {
        bool hasData = false;

        do
        {
            len = g_socket_receive (socket, buffer, kBufSize, NULL, &error);
            if (error != NULL)
            {
               g_error_free (error);
               ret = FALSE;
               break;
            }
            else if (len != 0)
            {
                hasData = true;

                buffer[len] = '\0';
                data.append(buffer, len);

                g_print("RECEIVED:[%s] [%s] %d %d\n", buffer, data.c_str(), data.length(), len);
            }
        }
        while (kBufSize == len);

        if (ret && hasData)
        {
            readyRead.emit();
        }
    }

    return ret;
}

size_t AbstractSocket::read(const char * buf, size_t num)
{
    g_print("QAbstractSocket::read %d bytes has %d\n", num, data.length());

    if (buf == NULL || num <= 0)
        return 0;

    size_t len = data.length();
    if (len > 0)
    {
        int nExtra = 0;
        if (len > num)
        {
            nExtra = len - num;
            len = num;
        }

        memcpy((void *)buf, data.c_str(), len);

        if (nExtra > 0)
            data.erase(0, len);
        else
            data.erase();
        g_print("QAbstractSocket::read completed - read %d bytes left %d\n", len, data.length());
    }

    return len;
}

size_t AbstractSocket::write(const char *data, size_t len)
{
    if (socket == NULL)
        return 0;

    GError *error = NULL;

    /* send data */
    gssize wrote = g_socket_send (socket, data, len, NULL, &error);
    if (wrote != len)
    {
        g_print("QLocalSocket::write ERROR\n");
    }
    else
    {
        g_print("QLocalSocket::write - bytes written %d\n", wrote);
    }
    return wrote;
}

/*!
    \class LocalSocket
    \brief The LocalSocket provides a local socket.
*/
bool LocalSocket::connectToServer(const std::string & filename)
{
    bool ret;
    GError *error = NULL;
    GSocketAddress *address = NULL;
    GSource *source;

    struct stat st;
    if (0 != stat(filename.c_str(), &st))
    {
        return false;
    }

    g_assert (g_file_test (filename.c_str(), G_FILE_TEST_EXISTS));

    /* create socket */
    socket = g_socket_new (G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, &error);
    g_assert_no_error (error);
    g_assert (socket != NULL);
    g_socket_set_blocking (socket, FALSE);
    g_socket_set_keepalive (socket, TRUE);

    /* connect to it */
    address = g_unix_socket_address_new_with_type (filename.c_str(), -1, G_UNIX_SOCKET_ADDRESS_PATH);
    ret = g_socket_connect (socket, address, NULL, &error);
    g_assert_no_error (error);
    g_assert (ret);

    g_object_unref(address);

    /* socket has data */
    source = g_socket_create_source (socket, (GIOCondition)(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL), NULL);

    g_source_set_callback (source, (GSourceFunc) socket_has_data_cb, this, NULL);
    g_source_attach (source, NULL);

    return ret;
}

bool LocalSocket::waitForConnected()
{
    return true;
}

/*!
    \class TcpSocket
    \brief The TcpSocket provides a TCP socket.
*/
bool TcpSocket::connectToHost(const std::string& hostname, int port)
{
    bool ret = false;
    GError *error = NULL;
    GSocketAddress *address = NULL;
    GInetAddress *inet_address;
    GSource *source;

    /* create socket */
    socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, &error);
    g_assert_no_error (error);
    g_assert (socket != NULL);
    g_socket_set_blocking (socket, FALSE);
    g_socket_set_keepalive (socket, TRUE);

    /* connect to it */
    inet_address = g_inet_address_new_from_string(hostname.c_str());
    address = g_inet_socket_address_new (inet_address, port);
    ret = g_socket_connect (socket, address, NULL, &error);
    g_assert_no_error (error);
    g_assert (ret);

    g_object_unref(address);
    g_object_unref(inet_address);

    /* socket has data */
    source = g_socket_create_source (socket, (GIOCondition)(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL), NULL);

    g_source_set_callback (source, (GSourceFunc) socket_has_data_cb, this, NULL);
    g_source_attach (source, NULL);

    return ret;
}

bool TcpSocket::waitForConnected()
{
    return true;
}

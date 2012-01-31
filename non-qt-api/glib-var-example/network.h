#ifndef NETWORK_H
#define NETWORK_H

#include <glib.h>
#include <sigc++/sigc++.h>
#include <gio/gio.h>

#include <string>

class AbstractSocket
{
public:
    AbstractSocket() : socket(NULL) {}
    virtual ~AbstractSocket();

    bool isOpen() { return socket != NULL; }
    size_t bytesAvailable() { return data.length(); }
    bool atEnd() { return data.length() == 0; }
    size_t read(const char * buf, size_t num);
    std::string readAll();
    size_t write(const char *data, size_t len);

    virtual bool waitForConnected() = 0;

    gboolean dataReceived(GSocket *socket, GIOCondition condition);

    // signales
    sigc::signal<void> readyRead;
    sigc::signal<void> aboutToClose;

protected:
    std::string data;
    GSocket *socket;
};

class LocalSocket : public AbstractSocket
{
public:
    bool connectToServer(const std::string & socketname);
    bool waitForConnected();

private:
};


class TcpSocket : public AbstractSocket
{
public:
    bool connectToHost(const std::string& hostname, int port);
    bool waitForConnected();

private:
};

#endif // NETWORK_H

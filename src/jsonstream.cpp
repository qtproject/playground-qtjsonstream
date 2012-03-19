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

#include <QDebug>
#include <QDataStream>
#include <QLocalSocket>
#include <QAbstractSocket>
#include <QtEndian>
#include <qjsondocument.h>
#include <QTextCodec>

#include "jsonstream.h"
#include "jsonbuffer_p.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/****************************************************************************/
/*
 *  Note:  We do NOT do DNS resolution, so you must specify an actual host IP address.
 */

static QByteArray bsonToByteArray(BsonObject &bson)
{
    QByteArray byteArray = bson.data();
    byteArray.prepend("bson");
    return byteArray;
}

/****************************************************************************/

class JsonStreamPrivate
{
public:
    JsonStreamPrivate()
        : mDevice(0)
        , mFormat(FormatUndefined) {}

    QIODevice       *mDevice;
    JsonBuffer      *mBuffer;
    EncodingFormat   mFormat;
};

/****************************************************************************/

/*!
    \class JsonStream
    \brief The JsonStream class serializes JSON data.

    The JsonStream class is a generic interface for serializing and deserializing
    JSON data over a socket connection.  It is designed to support multiple serialization
    and deserialization formats by auto-detecting the format in use.
*/

/*!
  Constructs a \c JsonStream object using the given \a device.
 */

JsonStream::JsonStream(QIODevice *device)
    : QObject(0)
    , d_ptr(new JsonStreamPrivate())
{
    Q_D(JsonStream);
    d->mBuffer = new JsonBuffer(this);
    connect(d->mBuffer, SIGNAL(objectReceived(const QJsonObject&)),
            SLOT(objectReceived(const QJsonObject&)));
    setDevice(device);
}

/*!
  Delete the \c JsonStream object and close the connection.
  You should call \l {setDevice()} {setDevice(0)} before calling this function.
 */

JsonStream::~JsonStream()
{
}

/*!
    This function checks for the existence of a \c QIODevice and
    returns whether or not it is \c atEnd()
*/

bool JsonStream::atEnd() const
{
    Q_D(const JsonStream);
    return (d->mDevice && d->mDevice->atEnd());
}

/*!
    This function checks for the existence of a \c QIODevice and
    returns whether or not it is \c isOpen()
*/

bool JsonStream::isOpen() const
{
    Q_D(const JsonStream);
    return (d->mDevice && d->mDevice->isOpen());
}

/*!
  Return the current QIODevice used by the JsonStream
*/

QIODevice * JsonStream::device() const
{
    Q_D(const JsonStream);
    return d->mDevice;
}

/*!
    Set the \a device used by the JsonStream.
    Setting the device to 0 disconnects the stream but does not close
    the device nor flush it.

    The stream does not take ownership of the device.
*/
void JsonStream::setDevice( QIODevice *device )
{
    Q_D(JsonStream);
    if (d->mDevice) {
        disconnect(d->mDevice, SIGNAL(readyRead()), this, SLOT(dataReadyOnSocket()));
        disconnect(d->mDevice, SIGNAL(aboutToClose()), this, SIGNAL(aboutToClose()));
    }
    d->mDevice = device;
    if (device) {
        connect(device, SIGNAL(readyRead()), this, SLOT(dataReadyOnSocket()));
        connect(device, SIGNAL(aboutToClose()), this, SIGNAL(aboutToClose()));
    }
}

/*!
  Send a JsonObject \a object over the stream
*/

void JsonStream::send(const QJsonObject& object)
{
    QJsonDocument document(object);

    Q_D(JsonStream);
    switch (d->mFormat) {
    case FormatUndefined:
        d->mFormat = FormatQBJS;
        // Deliberate fall through
    case FormatQBJS:
        sendInternal( document.toBinaryData() );
        break;
    case FormatUTF8:
        sendInternal( document.toJson() );
        break;
    case FormatUTF16BE:
        sendInternal( QTextCodec::codecForName("UTF-16BE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );  // Chop off BOM
        break;
    case FormatUTF16LE:
        sendInternal( QTextCodec::codecForName("UTF-16LE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );  // Chop off BOM
        break;
    case FormatBSON:
    {
        BsonObject bson(document.toVariant().toMap());
        sendInternal(bsonToByteArray(bson));
        break;
    }
    }
}

/*!
  \internal
  Send raw QByteArray \a byteArray data over the socket.
*/

void JsonStream::sendInternal(const QByteArray& byteArray)
{
    Q_D(JsonStream);
    if (!d->mDevice) {
        qWarning() << Q_FUNC_INFO << "No device in JsonStream";
        return;
    }

    int nBytes = d->mDevice->write( byteArray.data(), byteArray.size() );
    if (QLocalSocket *socket = qobject_cast<QLocalSocket*>(d->mDevice))
        socket->flush();
    else if (QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->mDevice))
        socket->flush();
    else
        qWarning() << Q_FUNC_INFO << "Unknown socket type:" << d->mDevice->metaObject()->className();

    if (nBytes != byteArray.size())
        qCritical() << Q_FUNC_INFO << __LINE__
                    << QString::fromLatin1("Expected to write %1 bytes, actually %2.").arg(byteArray.size()).arg(nBytes);
}

/*!
  \internal
  Handle a received Qt Binary Json \a object and emit the correct signals
*/

void JsonStream::objectReceived(const QJsonObject& object)
{
    Q_D(JsonStream);
    if (d->mFormat == FormatUndefined)
        d->mFormat = d->mBuffer->format();
    emit messageReceived(object);
}

/*!
  \internal
  Extract data from the socket and extract received messages.
*/

void JsonStream::dataReadyOnSocket()
{
    Q_D(JsonStream);
    d->mBuffer->append( d->mDevice->readAll());
}

/*!
  Return the current JsonStream::EncodingFormat.
 */

EncodingFormat JsonStream::format() const
{
    Q_D(const JsonStream);
    return d->mFormat;
}

/*!
  Set the EncodingFormat to \a format.
 */

void JsonStream::setFormat( EncodingFormat format )
{
    Q_D(JsonStream);
    d->mFormat = format;
}


/*!
    \relates JsonStream

    Sends the \a map via the \a stream.
*/
JsonStream& operator<<( JsonStream& stream, const QJsonObject& map )
{
    stream.send(map);
    return stream;
}


/*!
    \fn void JsonStream::messageReceived(const QJsonObject& message)
    This signal is emitted when a new \a message has been received on the
    stream.
*/

/*!
    \fn void JsonStream::aboutToClose()
    This signal is emitted when the underlying \c QIODevice is about to close.
*/

#include "moc_jsonstream.cpp"

QT_END_NAMESPACE_JSONSTREAM

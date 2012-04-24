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
        , mFormat(FormatUndefined)
        , mReadBufferSize(0)
        , mWriteBufferSize(0)
        , mLastError(JsonStream::NoError) {}

    QIODevice       *mDevice;
    JsonBuffer      *mBuffer;
    EncodingFormat   mFormat;
    qint64           mReadBufferSize;
    qint64           mWriteBufferSize;
    JsonStream::JsonStreamError  mLastError;
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
    connect(d->mBuffer, SIGNAL(readyReadMessage()), SLOT(messageReceived()));
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
     \enum JsonStream::JsonStreamError
     \value NoError
        No error has occurred.
     \value WriteFailedNoConnection
        Can't write because there is no established connection.
     \value MaxReadBufferSizeExceeded
        Maximum read buffer size exceeded. Can't continue and connection will be closed.
     \value MaxWriteBufferSizeExceeded
        Maximum write buffer size exceeded. Operation can be retried after write buffer maximum size increase or after data in
        the buffer will be processed by QIODevice.
     \value WriteFailed
         Write error occurred ( QIODevice::write() returned -1 ).
     \value WriteFailedReturnedZero
         Write error occurred ( QIODevice::write() returned 0 ).
 */

/*!
    Returns the error the last operation produced, or NoError error if the last operation did not produce an error.
*/
JsonStream::JsonStreamError JsonStream::lastError() const
{
    return d_ptr->mLastError;
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
        disconnect(d->mDevice, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
        disconnect(d->mDevice, SIGNAL(aboutToClose()), this, SIGNAL(aboutToClose()));
        d->mBuffer->clear();
    }
    d->mDevice = device;
    if (device) {
        connect(device, SIGNAL(readyRead()), this, SLOT(dataReadyOnSocket()));
        connect(device, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
        connect(device, SIGNAL(aboutToClose()), this, SIGNAL(aboutToClose()));
    }
}

/*!
  Send a JsonObject \a object over the stream.
  Returns \b true if the entire message was sent or buffered or \b false otherwise.

  JsonStream does not have a write buffer of its own.  Rather, it uses the
  write buffer of the \l{device()}.  It will not cause that buffer to grow
  larger than \l{writeBufferSize()} at any time.  If this would occur, this
  method will return \b false.
*/

bool JsonStream::send(const QJsonObject& object)
{
    bool bRet(false);
    QJsonDocument document(object);

    Q_D(JsonStream);
    switch (d->mFormat) {
    case FormatUndefined:
        d->mFormat = FormatQBJS;
        // Deliberate fall through
    case FormatQBJS:
        bRet = sendInternal( document.toBinaryData() );
        break;
    case FormatUTF8:
        bRet = sendInternal( document.toJson() );
        break;
    case FormatUTF16BE:
        bRet = sendInternal( QTextCodec::codecForName("UTF-16BE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );  // Chop off BOM
        break;
    case FormatUTF16LE:
        bRet = sendInternal( QTextCodec::codecForName("UTF-16LE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );  // Chop off BOM
        break;
    case FormatUTF32BE:
        bRet = sendInternal( QTextCodec::codecForName("UTF-32BE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(4) );  // Chop off BOM
        break;
    case FormatUTF32LE:
        bRet = sendInternal( QTextCodec::codecForName("UTF-32LE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(4) );  // Chop off BOM
        break;
    case FormatBSON:
    {
        BsonObject bson(document.toVariant().toMap());
        bRet = sendInternal(bsonToByteArray(bson));
        break;
    }
    }
    return bRet;
}

/*!
  \internal
  Send raw QByteArray \a byteArray data over the socket.
*/
bool JsonStream::sendInternal(const QByteArray& byteArray)
{
    Q_D(JsonStream);
    if (!isOpen()) {
        d->mLastError = WriteFailedNoConnection;
        qWarning() << Q_FUNC_INFO << "No device in JsonStream";
        return false;
    }
    d->mLastError = NoError;

    int nBytes = 0;
    if (d->mWriteBufferSize <= 0 || d->mDevice->bytesToWrite() + byteArray.size() <= d->mWriteBufferSize) {
        for (int nSz = byteArray.size(); nSz > 0; ) {
            int nWrite = d->mDevice->write( byteArray.constData() + nBytes, nSz);
            if (nWrite <= 0) {
                // write error
                d->mLastError = (nWrite < 0 ? WriteFailed : WriteFailedReturnedZero);
                qWarning() << Q_FUNC_INFO << __LINE__
                           << QString::fromLatin1("Write error. QIODevice::write() returned %1 (%2).")
                              .arg(nWrite).arg(d->mDevice->errorString());
                break;
            }
            nBytes += nWrite;
            nSz -= nWrite;
        }
    }
    else
    {
        d->mLastError = MaxWriteBufferSizeExceeded;
    }

    if (QLocalSocket *socket = qobject_cast<QLocalSocket*>(d->mDevice))
        socket->flush();
    else if (QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->mDevice))
        socket->flush();
    else
        qWarning() << Q_FUNC_INFO << "Unknown socket type:" << d->mDevice->metaObject()->className();

    bool bFail;
    if ((bFail = (nBytes != byteArray.size())))
        qCritical() << Q_FUNC_INFO << __LINE__
                    << QString::fromLatin1("Expected to write %1 bytes, actually %2.").arg(byteArray.size()).arg(nBytes);
    return !bFail;
}

/*!
  \internal
  Handle a received readyReadMessage signal and emit the correct signals
*/

void JsonStream::messageReceived()
{
    Q_D(JsonStream);
    if (d->mFormat == FormatUndefined)
        d->mFormat = d->mBuffer->format();
    emit readyReadMessage();
}

/*!
  \internal
  Extract data from the socket and extract received messages.
*/

void JsonStream::dataReadyOnSocket()
{
    Q_D(JsonStream);
    d->mLastError = NoError;
    if (d->mReadBufferSize > 0) {
        while (d->mDevice->bytesAvailable() + d->mBuffer->size() > d->mReadBufferSize) {
            // can't fit all data into a read buffer - read a part that fits
            d->mBuffer->append(d->mDevice->read(d->mReadBufferSize - d->mBuffer->size()));

            // if the read buffer is full then emit readBufferOverflow and allow user to increase the buffer size
            if (d->mBuffer->size() == d->mReadBufferSize) {
                emit readBufferOverflow(d->mDevice->bytesAvailable() + d->mBuffer->size());
                if (d->mBuffer->size() == d->mReadBufferSize) {
                    // still can't read anything - close connection
                    d->mLastError = MaxReadBufferSizeExceeded;
                    d->mDevice->close();
                    return;
                }
                else if (0 == d->mReadBufferSize) {
                    // user removed buffer size limitation
                    break;
                }
            }
        }
    }
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
  Returns a maximum size of the inbound message buffer.
 */
qint64 JsonStream::readBufferSize() const
{
    Q_D(const JsonStream);
    return d->mReadBufferSize;
}

/*!
  Sets a maximum size of the inbound message buffer to \a sz thus capping a size
  of an inbound message.
 */
void JsonStream::setReadBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonStream);
        d->mReadBufferSize = sz;
    }
}

/*!
  Returns a maximum size of the outbound message buffer.  A value of 0
  means the buffer size is unlimited.
 */
qint64 JsonStream::writeBufferSize() const
{
    Q_D(const JsonStream);
    return d->mWriteBufferSize;
}

/*!
  Sets a maximum size of the outbound message buffer to \a sz thus capping a size
  of an outbound message.  A value of 0 means the buffer size is unlimited.
 */
void JsonStream::setWriteBufferSize(qint64 sz)
{
    if (sz >= 0) {
        Q_D(JsonStream);
        d->mWriteBufferSize = sz;
    }
}

/*!
  Returns a number of bytes currently in the write buffer.  Effectively,
  if \l{writeBufferSize()} is not unlimited,  the largest message you can
  send at any one time is (\l{writeBufferSize()} - \b bytesToWrite()) bytes.
 */
qint64 JsonStream::bytesToWrite() const
{
    Q_D(const JsonStream);
    return (d->mDevice ? d->mDevice->bytesToWrite() : 0);
}

/*!
  Returns a JSON object that has been received.  If no message is
  available, an empty JSON object is returned.
 */
QJsonObject JsonStream::readMessage()
{
    Q_D(JsonStream);
    return d->mBuffer->readMessage();
}

/*!
  Returns \b true if a message is available to be read via \l{readMessage()}
  or \b false otherwise.
 */
bool JsonStream::messageAvailable()
{
    Q_D(const JsonStream);
    return d->mBuffer->messageAvailable();
}

/*!
  internal
 */
void JsonStream::setThreadProtection(bool enable) const
{
    Q_D(const JsonStream);
    d->mBuffer->setThreadProtection(enable);
}

/*! \fn JsonStream::bytesWritten(qint64 bytes)

    This signal is emitted every time a payload of data has been
    written to the device. The \a bytes argument is set to the number
    of bytes that were written in this payload.

    If a previous call to \l{send()} returned \b false, you should re-send
    the message when this signal is emitted, as the write buffer may have been
    emptied enough to hold the new message.
*/

/*!
    \fn void JsonStream::readyReadMessage()

    This signal is emitted once every time new data arrives on the \l{device()}
    and a message is ready. \l{readMessage()} should be used to retrieve a message
    and \l{messageAvailable()} to check for next available messages.
    The client code may look like this:

    \code
    ...
    connect(jsonstream, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    ...

    void Foo::processMessages()
    {
        while (jsonstream->messageAvailable()) {
            QJsonObject obj = jsonstream->readMessage();
            <process message>
        }
    }
    \endcode

    \b readyReadMessage() is not emitted recursively; if you reenter the event loop
    inside a slot connected to the \b readyReadMessage() signal, the signal will not
    be reemitted.

    \sa readMessage(), messageAvailable()
*/

/*!
    \fn void JsonStream::aboutToClose()
    This signal is emitted when the underlying \c QIODevice is about to close.
*/

/*! \fn JsonStream::readBufferOverflow(qint64 bytes)

  This signal is emitted when the read buffer is full of data that has been read
  from the \l{device()}, \a bytes additional bytes are available on the device,
  but the message is not complete.  The \l{readBufferSize()} mayb e increased
  to a sufficient size in a slot connected to this signal, in which case more
  data will be read into the read buffer.  If the buffer size  is not increased,
  the connection is closed.
 */

#include "moc_jsonstream.cpp"

QT_END_NAMESPACE_JSONSTREAM

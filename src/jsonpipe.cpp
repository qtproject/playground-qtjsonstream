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
#include <QtEndian>
#include <QSocketNotifier>
#include <QElapsedTimer>
#include <qjsondocument.h>
#include <qjsonobject.h>

#include <sys/select.h>
#include <stdio.h>
#include <errno.h>

#include "jsonpipe.h"
#include "jsonbuffer_p.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/****************************************************************************/

/*!
    \class JsonPipe
    \brief The JsonPipe class serializes JSON data.

    The JsonPipe class is a generic interface for serializing and deserializing
    JSON data over pipe connections.  It is designed to support multiple serialization
    and deserialization formats by auto-detecting the format in use.
*/

/*!
  Constructs a \c JsonPipe object with an optional \a parent.
 */

JsonPipe::JsonPipe(QObject *parent)
    : QObject(parent)
    , mIn(0)
    , mOut(0)
    , mFormat(FormatUndefined)
{
    mInBuffer = new JsonBuffer(this);
    connect(mInBuffer, SIGNAL(objectReceived(const QJsonObject&)),
            SLOT(objectReceived(const QJsonObject&)));
}

/*!
  Delete the \c JsonPipe object
 */

JsonPipe::~JsonPipe()
{
}

/*!
  Return true if writing should be possible
*/

bool JsonPipe::writeEnabled() const
{
    return (mOut != NULL);
}

/*!
  Return true if more data may be read
*/

bool JsonPipe::readEnabled() const
{
    return (mIn != NULL);
}

/*!
  Set the current file descriptors
*/

void JsonPipe::setFds(int in_fd, int out_fd)
{
    if (mIn)
        delete mIn;
    if (mOut)
        delete mOut;

    mIn = new QSocketNotifier(in_fd, QSocketNotifier::Read, this);
    mOut = new QSocketNotifier(out_fd, QSocketNotifier::Write, this);
    connect(mIn, SIGNAL(activated(int)), SLOT(inReady(int)));
    connect(mOut, SIGNAL(activated(int)), SLOT(outReady(int)));
    mIn->setEnabled(true);
    mOut->setEnabled(mOutBuffer.size() > 0);
}

void JsonPipe::inReady(int fd)
{
    mIn->setEnabled(false);
    int n = mInBuffer->copyFromFd(fd);
    if (n <= 0) {
        mInBuffer->clear();
        mIn->deleteLater();
        mIn = NULL;
        emit error( (n < 0) ? ReadFailed : ReadAtEnd );
    }
    else
        mIn->setEnabled(true);
}

/*!
  \internal

  Return number of byte written
 */
int JsonPipe::writeInternal(int fd)
{
    if (!mOutBuffer.size())
        return 0;

    int n = ::write(fd, mOutBuffer.data(), mOutBuffer.size());
    if (n <= 0) {
        mOut->deleteLater();
        mOut = NULL;
        // ### TODO: This emits errors in the middle of waitForBytesWritten.
        // ### This could cause problems 'cause it gets called in destructors
        emit error(n < 0 ? WriteFailed : WriteAtEnd);
    }
    else if (n < mOutBuffer.size())
        mOutBuffer = mOutBuffer.mid(n);
    else
        mOutBuffer.clear();
    return n;
}

void JsonPipe::outReady(int)
{
    Q_ASSERT(mOut);
    mOut->setEnabled(false);
    if (mOutBuffer.size()) {
        writeInternal(mOut->socket());
        if (mOut && !mOutBuffer.isEmpty())
            mOut->setEnabled(true);
    }
}

/*!
  Send a JsonObject \a object over the pipe
*/

bool JsonPipe::send(const QJsonObject& object)
{
    if (!mOut)
        return false;

    QJsonDocument document(object);

    switch (mFormat) {
    case FormatUndefined:
        mFormat = FormatQBJS;
        // Deliberate fall through
    case FormatQBJS:
        mOutBuffer.append(document.toBinaryData());
        break;
    case FormatUTF8:
        mOutBuffer.append(document.toJson());
        break;
    case FormatBSON:
    {
        BsonObject bson(document.toVariant().toMap());
        mOutBuffer.append("bson");
        mOutBuffer.append(bson.data());
        break;
    }
    }
    if (mOutBuffer.size())
        mOut->setEnabled(true);
    return true;
}

/*!
  \internal
  Handle a received Qt Binary Json \a object and emit the correct signals
*/

void JsonPipe::objectReceived(const QJsonObject& object)
{
    if (mFormat == FormatUndefined)
        mFormat = mInBuffer->format();
    emit messageReceived(object);
}

/*!
  Return the current JsonPipe::EncodingFormat.
 */

EncodingFormat JsonPipe::format() const
{
    return mFormat;
}

/*!
  Set the EncodingFormat to \a format.
 */

void JsonPipe::setFormat( EncodingFormat format )
{
    mFormat = format;
}

/*
  Blocks until all of the output buffer has been written to the pipe.
  We return true if and only if there was data to be written and it
  was successfully written.
 */

bool JsonPipe::waitForBytesWritten(int msecs)
{
    if (!mOut || mOutBuffer.isEmpty())
        return false;

    mOut->setEnabled(false);

    QElapsedTimer stopWatch;
    stopWatch.start();

    while (mOut && !mOutBuffer.isEmpty()) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(mOut->socket(),&wfds);

        int timeout = msecs - stopWatch.elapsed();
        struct timeval tv;
        struct timeval *tvptr = ((msecs > 0 && timeout > 0) ? &tv : NULL);
        if (tvptr) {
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
        }

        int retval = ::select(mOut->socket() + 1, NULL, &wfds, NULL, tvptr);
        if (retval == -1 && errno == EINTR)
            continue;
        if (retval <= 0)
            break;
        writeInternal(mOut->socket());
    }

    if (mOut && !mOutBuffer.isEmpty())
        mOut->setEnabled(true);
    return mOutBuffer.isEmpty();
}

/*!
    Sends the \a map via the pipe.
    The QVariant types allowed are restricted to basic types supported
    by the BsonObject which is in principle bool, int, long, QString and
    arrays and maps of them.

    \sa BsonObject
*/
JsonPipe& operator<<( JsonPipe& s, const QJsonObject& map )
{
    s.send(map);
    return s;
}

/*!
    \fn void JsonPipe::messageReceived(const QJsonObject& message)
    This signal is emitted when a new \a message has been received on the
    pipe.
*/

/*!
    \fn void JsonPipe::aboutToClose()
    This signal is emitted when the underlying \c QIODevice is about to close.

    \sa QIODevice
*/

#include "moc_jsonpipe.cpp"

QT_END_NAMESPACE_JSONSTREAM

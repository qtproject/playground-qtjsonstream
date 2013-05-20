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

#include <QDebug>
#include <QtEndian>
#include <QSocketNotifier>
#include <QElapsedTimer>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <QTextCodec>

#include <sys/select.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h> // for ::write

#include "qjsonpipe.h"
#include "qjsonbuffer_p.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/****************************************************************************/

class QJsonPipePrivate
{
public:
    QJsonPipePrivate()
        : mIn(0)
        , mOut(0)
        , mFormat(FormatUndefined) {}

    QJsonBuffer      *mInBuffer;
    QByteArray       mOutBuffer;
    QSocketNotifier *mIn;
    QSocketNotifier *mOut;
    EncodingFormat   mFormat;
};

/****************************************************************************/

/*!
    \class QJsonPipe
    \inmodule QtJsonStream
    \brief The QJsonPipe class serializes JSON data.

    The QJsonPipe class is a generic interface for serializing and deserializing
    JSON data over pipe connections.  It is designed to support multiple serialization
    and deserialization formats by auto-detecting the format in use.
*/

/*!
    \enum QJsonPipe::PipeError

    This enum is passed in the \l{error()} signal indicating a problem
    with a pipe connection.

    \value WriteFailed  Unable to write to the outgoing pipe
    \value WriteAtEnd   The outgoing pipe has been closed
    \value ReadFailed   Unable to read from the incoming pipe
    \value ReadAtEnd    The incoming pipe has been closed
 */

/*!
  Constructs a \c QJsonPipe object with an optional \a parent.
 */

QJsonPipe::QJsonPipe(QObject *parent)
    : QObject(parent)
    , d_ptr(new QJsonPipePrivate())
{
    Q_D(QJsonPipe);
    d->mInBuffer = new QJsonBuffer(this);
    connect(d->mInBuffer, SIGNAL(readyReadMessage()), SLOT(processMessages()));
}

/*!
  Delete the \c QJsonPipe object
 */

QJsonPipe::~QJsonPipe()
{
}

/*!
  Return true if writing should be possible
*/

bool QJsonPipe::writeEnabled() const
{
    Q_D(const QJsonPipe);
    return (d->mOut != NULL);
}

/*!
  Return true if more data may be read
*/

bool QJsonPipe::readEnabled() const
{
    Q_D(const QJsonPipe);
    return (d->mIn != NULL);
}

/*!
  Set the current file descriptors.  The input descriptor is set to \a
  in_fd and the output descriptor is set to \a out_fd.
*/

void QJsonPipe::setFds(int in_fd, int out_fd)
{
    Q_D(QJsonPipe);
    if (d->mIn)
        delete d->mIn;
    if (d->mOut)
        delete d->mOut;

    d->mIn = new QSocketNotifier(in_fd, QSocketNotifier::Read, this);
    d->mOut = new QSocketNotifier(out_fd, QSocketNotifier::Write, this);
    connect(d->mIn, SIGNAL(activated(int)), SLOT(inReady(int)));
    connect(d->mOut, SIGNAL(activated(int)), SLOT(outReady(int)));
    d->mIn->setEnabled(true);
    d->mOut->setEnabled(d->mOutBuffer.size() > 0);
}

/*!
  \internal
 */

void QJsonPipe::inReady(int fd)
{
    Q_D(QJsonPipe);
    d->mIn->setEnabled(false);
    int n = d->mInBuffer->copyFromFd(fd);
    if (n <= 0) {
        d->mInBuffer->clear();
        d->mIn->deleteLater();
        d->mIn = NULL;
        emit error( (n < 0) ? ReadFailed : ReadAtEnd );
    }
    else
        d->mIn->setEnabled(true);
}

/*!
  \internal

  Return number of byte written
 */
int QJsonPipe::writeInternal(int fd)
{
    Q_D(QJsonPipe);
    if (!d->mOutBuffer.size())
        return 0;

    int n = ::write(fd, d->mOutBuffer.data(), d->mOutBuffer.size());
    if (n <= 0) {
        d->mOut->deleteLater();
        d->mOut = NULL;
        // ### TODO: This emits errors in the middle of waitForBytesWritten.
        // ### This could cause problems 'cause it gets called in destructors
        emit error(n < 0 ? WriteFailed : WriteAtEnd);
    }
    else if (n < d->mOutBuffer.size())
        d->mOutBuffer = d->mOutBuffer.mid(n);
    else
        d->mOutBuffer.clear();
    return n;
}

/*!
  \internal
*/

void QJsonPipe::outReady(int)
{
    Q_D(QJsonPipe);
    Q_ASSERT(d->mOut);
    d->mOut->setEnabled(false);
    if (d->mOutBuffer.size()) {
        writeInternal(d->mOut->socket());
        if (d->mOut && !d->mOutBuffer.isEmpty())
            d->mOut->setEnabled(true);
    }
}

/*!
  Send a QJsonObject \a object over the pipe.  Return true if
  the object could be added to the output buffer and false if there is
  no output buffer.
*/

bool QJsonPipe::send(const QJsonObject& object)
{
    Q_D(QJsonPipe);
    if (!d->mOut)
        return false;

    QJsonDocument document(object);

    switch (d->mFormat) {
    case FormatUndefined:
        d->mFormat = FormatQBJS;
        // Deliberate fall through
    case FormatQBJS:
        d->mOutBuffer.append(document.toBinaryData());
        break;
    case FormatUTF8:
        d->mOutBuffer.append(document.toJson());
        break;
    case FormatUTF16BE:
        d->mOutBuffer.append( QTextCodec::codecForName("UTF-16BE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );
        break;
    case FormatUTF16LE:
        d->mOutBuffer.append( QTextCodec::codecForName("UTF-16LE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(2) );
        break;
    case FormatUTF32BE:
        d->mOutBuffer.append( QTextCodec::codecForName("UTF-32BE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(4) );
        break;
    case FormatUTF32LE:
        d->mOutBuffer.append( QTextCodec::codecForName("UTF-32LE")->fromUnicode(QString::fromUtf8(document.toJson())).mid(4) );
        break;
    case FormatBSON:
    {
        BsonObject bson(document.toVariant().toMap());
        d->mOutBuffer.append("bson");
        d->mOutBuffer.append(bson.data());
        break;
    }
    }
    if (d->mOutBuffer.size())
        d->mOut->setEnabled(true);
    return true;
}

/*!
  \internal
  Handle a received Qt Binary QJson \a object and emit the correct signals
*/

void QJsonPipe::objectReceived(const QJsonObject& object)
{
    Q_D(QJsonPipe);
    if (d->mFormat == FormatUndefined)
        d->mFormat = d->mInBuffer->format();
    emit messageReceived(object);
}

/*!
  \internal
*/
void QJsonPipe::processMessages()
{
    Q_D(QJsonPipe);
    d->mInBuffer->setEnabled(false);
    while (d->mInBuffer->messageAvailable()) {
        QJsonObject obj = d->mInBuffer->readMessage();
        if (!obj.isEmpty())
            objectReceived(obj);
    }
    d->mInBuffer->setEnabled(true);
}

/*!
  Return the current QJsonPipe::EncodingFormat.
 */

EncodingFormat QJsonPipe::format() const
{
    Q_D(const QJsonPipe);
    return d->mFormat;
}

/*!
  Set the EncodingFormat to \a format.
 */

void QJsonPipe::setFormat( EncodingFormat format )
{
    Q_D(QJsonPipe);
    d->mFormat = format;
}

/*!
  Blocks until all of the output buffer has been written to the pipe.
  We return true if and only if there was data to be written and it
  was successfully written.  The \a msecs parameter is the maximum
  number of milliseconds to block before giving up.
 */

bool QJsonPipe::waitForBytesWritten(int msecs)
{
    Q_D(QJsonPipe);
    if (!d->mOut || d->mOutBuffer.isEmpty())
        return false;

    d->mOut->setEnabled(false);

    QElapsedTimer stopWatch;
    stopWatch.start();

    while (d->mOut && !d->mOutBuffer.isEmpty()) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(d->mOut->socket(),&wfds);

        int timeout = msecs - stopWatch.elapsed();
        struct timeval tv;
        struct timeval *tvptr = ((msecs > 0 && timeout > 0) ? &tv : NULL);
        if (tvptr) {
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
        }

        int retval = ::select(d->mOut->socket() + 1, NULL, &wfds, NULL, tvptr);
        if (retval == -1 && errno == EINTR)
            continue;
        if (retval <= 0)
            break;
        writeInternal(d->mOut->socket());
    }

    if (d->mOut && !d->mOutBuffer.isEmpty())
        d->mOut->setEnabled(true);
    return d->mOutBuffer.isEmpty();
}

/*!
  \relates QJsonPipe

    Sends the \a map via the \a pipe.
*/

QJsonPipe& operator<<( QJsonPipe& pipe, const QJsonObject& map )
{
    pipe.send(map);
    return pipe;
}

/*!
    \fn void QJsonPipe::messageReceived(const QJsonObject& message)
    This signal is emitted when a new \a message has been received on the
    pipe.
*/

/*!
    \fn void QJsonPipe::error(PipeError err)
    This signal is emitted when there is a read or write pipe error \a
    err.
*/

#include "moc_qjsonpipe.cpp"

QT_END_NAMESPACE_JSONSTREAM

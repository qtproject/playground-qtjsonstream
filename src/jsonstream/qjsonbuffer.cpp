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
#include <QJsonDocument>
#include <QTextCodec>
#include <QMutexLocker>

#include <unistd.h> // for ::read

#include "qjsonbuffer_p.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

template <typename T>
inline bool isjsonws(T c)
{
    return c == '\n' || c == ' ' || c == '\t' || c == '\r';
}

/*!
  \class QJsonBuffer
  \brief The QJsonBuffer class parses data received into appropriate QJson messages

  The QJsonBuffer class wraps an internal buffer.  As you append
  data, the QJsonBuffer object parses for received QJson objects and
  raises an appropriate signal for each received object.

  \warning This class does not support JSON arrays.  Arrays that are received will be ignored.
*/

/*!
  Construct a QJsonBuffer object with the given \a parent
*/

QJsonBuffer::QJsonBuffer(QObject *parent)
    : QObject(parent)
    , mFormat(FormatUndefined)
    , mParserState(ParseNormal)
    , mParserDepth(0)
    , mParserOffset(0)
    , mParserStartOffset(-1)
    , mEmittedReadyRead(false)
    , mMessageAvailable(false)
    , mMessageSize(0)
    , mEnabled(true)
    , mThreadProtection(false)
{
}

/*!
    \fn bool QJsonBuffer::isEnabled() const

    Returns true if \l{readyReadMessage()} signal notifier is enabled; otherwise returns false.

    \sa setEnabled(), readyReadMessage()
*/

/*!
    \fn void QJsonBuffer::setEnabled(bool enable)

    If \a enable is true, \l{readyReadMessage()} signal notifier is enabled;
    otherwise the notifier is disabled.

    The notifier is enabled by default, i.e. it emits the \l{readyReadMessage()}
    signal whenever a new message is ready.

    The notifier should normally be disabled while user is reading existing messages.

    \sa isEnabled(), readyReadMessage()
*/

/*!
    \fn int QJsonBuffer::size() const

    Returns the size of the buffer in bytes.
*/

/*!
  Append the contents of a byte array \a data onto the buffer.
  During the execution of this function, the \l{readyReadMessage()}
  signal may be raised.
*/

void QJsonBuffer::append(const QByteArray& data)
{
    {
        QScopedPointer<QMutexLocker> locker(createLocker());
        mBuffer.append(data.data(), data.size());
    }
    if (0 < size())
        processMessages();
}

/*!
  Append the \a data pointer with length \a len onto the QJsonBuffer.
  During the execution of this function, the \l{readyReadMessage()}
  signal may be raised.
*/

void QJsonBuffer::append(const char *data, int len)
{
    {
        QScopedPointer<QMutexLocker> locker(createLocker());
        mBuffer.append(data, len);
    }
    if (0 < size())
        processMessages();
}

/*!
  Copy data from a file descriptor \a fd into the buffer.
  This function tries to eliminate extra data copy operations.
  It assumes that the file descriptor is ready to read and
  it does not try to read all of the data.

  Returns the number of bytes read or -1 for an error condition.
 */

int QJsonBuffer::copyFromFd(int fd)
{
    const int maxcopy = 1024;
    QScopedPointer<QMutexLocker> locker(createLocker());
    uint oldSize = mBuffer.size();
    mBuffer.resize(oldSize + maxcopy);
    int n = ::read(fd, mBuffer.data()+oldSize, maxcopy);
    if (n > 0) {
        mBuffer.resize(oldSize+n);
        if (!locker.isNull())
            locker->unlock();
        processMessages();
    }
    else
        mBuffer.resize(oldSize);
    return n;
}

/*!
    Clear the contents of the buffer.
 */

void QJsonBuffer::clear()
{
    QScopedPointer<QMutexLocker> locker(createLocker());
    mBuffer.clear();
    resetParser();
}

/*!
  \internal
*/

bool QJsonBuffer::scanUtf( int c )
{
    switch (mParserState) {
    case ParseNormal:
        if ( c == '{' ) {
            if ( mParserDepth == 0 )
                mParserStartOffset = mParserOffset;
            mParserDepth += 1;
        }
        else if ( c == '}' && mParserDepth > 0 ) {
            mParserDepth -= 1;
            if ( mParserDepth == 0 ) {
                return true;
            }
        }
        else if ( c == '"' ) {
            mParserState = ParseInString;
        }
        break;
    case ParseInString:
        if ( c == '"' ) {
            mParserState = ParseNormal;
        } else if ( c == '\\' ) {
            mParserState = ParseInBackslash;
        }
        break;
    case ParseInBackslash:
        mParserState = ParseInString;
        break;
    }
    return false;
}

void QJsonBuffer::resetParser()
{
    mParserState  = ParseNormal;
    mParserDepth  = 0;
    mParserOffset = 0;
    mParserStartOffset = -1;
    mMessageAvailable = false;
    mMessageSize = 0;
}

/*!
  \internal
*/

void QJsonBuffer::processMessages()
{
    // do not process anything if disabled or if control is still inside readyReadMessage() slot
    if (mEnabled && !mEmittedReadyRead) {
        mEmittedReadyRead = true;
        if (messageAvailable()) {
            emit readyReadMessage();
        }
        mEmittedReadyRead = false;
    }
}

/*!
  \internal
*/
bool QJsonBuffer::messageAvailable()
{
    QScopedPointer<QMutexLocker> locker(createLocker());
    if (mMessageAvailable) {
        // already found - no need to check again
        return true;
    }

    if (mBuffer.size() < 4) {
        // buffer too small for a json message
        return false;
    }

    if (mFormat == FormatUndefined && mBuffer.size() >= 4) {
        if (strncmp("bson", mBuffer.constData(), 4) == 0)
            mFormat = FormatBSON;
        else if (QJsonDocument::BinaryFormatTag == *((uint *) mBuffer.constData()))
            mFormat = FormatQBJS;
        else {
            uchar u0 = mBuffer.at(0), u1 = mBuffer.at(1), u2 = mBuffer.at(2), u3 = mBuffer.at(3);
            // has a BOM?
            if (u0 == 0xFF && u1 == 0xFE) { // utf-32 le or utf-16 le + BOM
                mFormat = (u2 == 0 && u3 == 0 ) ? FormatUTF32LE : FormatUTF16LE;
                mParserOffset++;
            }
            else if (u0 == 0xFE && u1 == 0xFF) { // utf16 be + BOM
                mFormat = FormatUTF16BE;
                mParserOffset++;
            }
            else if (u0 == 0x00 && u1 == 0x00 && u2 == 0xFE && u3 == 0xFF ) { // utf-32 be + BOM
                mFormat = FormatUTF32BE;
                mParserOffset++;
            }
            else if (u0 == 0xEF && u1 == 0xBB && u2 == 0xBF) { // utf8 + BOM
                mFormat = FormatUTF8;
                mParserOffset+=3;
            }
            // no BOM
            else if (u0 == 0 && u1 != 0 && u2 == 0 && u3 != 0 ) {
                mFormat = FormatUTF16BE;
            }
            else if (u0 != 0 && u1 == 0 && u2 != 0 && u3 == 0 ) {
                mFormat = FormatUTF16LE;
            }
            else if (u0 == 0 && u1 == 0 && u2 == 0 && u3 != 0 ) {
                mFormat = FormatUTF32BE;
            }
            else if (u0 != 0 && u1 == 0 && u2 == 0 && u3 == 0 ) {
                mFormat = FormatUTF32LE;
            }
            else {
                mFormat = FormatUTF8;
            }
        }
    }

    switch (mFormat) {
    case FormatUndefined:
        break;
    case FormatUTF8:
        for (  ; mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            char c = mBuffer.at(mParserOffset);
            if (mMessageAvailable) {
                if (!isjsonws(c))
                    break;
            }
            else if (scanUtf(c)) {
                mMessageAvailable = true;
            }
        }
        break;
    case FormatUTF16BE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromBigEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (mMessageAvailable) {
                if (!isjsonws(c))
                    break;
            }
            else if (scanUtf(c)) {
                mMessageAvailable = true;
            }
        }
        break;
    case FormatUTF16LE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromLittleEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (mMessageAvailable) {
                if (!isjsonws(c))
                    break;
            }
            else if (scanUtf(c)) {
                mMessageAvailable = true;
            }
        }
        break;
    case FormatUTF32BE:
        for (  ; 4 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int32_t c = qFromBigEndian(reinterpret_cast<const int32_t *>(mBuffer.constData())[mParserOffset]);
            if (mMessageAvailable) {
                if (!isjsonws(c))
                    break;
            }
            else if (scanUtf(c)) {
                mMessageAvailable = true;
            }
        }
        break;
    case FormatUTF32LE:
        for (  ; 4 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int32_t c = qFromLittleEndian(reinterpret_cast<const int32_t *>(mBuffer.constData())[mParserOffset]);
            if (mMessageAvailable) {
                if (!isjsonws(c))
                    break;
            }
            else if (scanUtf(c)) {
                mMessageAvailable = true;
            }
        }
        break;
    case FormatBSON:
        if (mBuffer.size() >= 8) {
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.constData())[1]);
            if (mBuffer.size() >= message_size + 4) {
                mMessageSize = message_size;
                mMessageAvailable = true;
            }
        }
        break;
    case FormatQBJS:
        if (mBuffer.size() >= 12) {
            // ### TODO: Should use 'sizeof(Header)'
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.constData())[2]) + 8;
            if (mBuffer.size() >= message_size) {
                mMessageSize = message_size;
                mMessageAvailable = true;
            }
        }
        break;
    }
    return mMessageAvailable;
}

/*!
  \internal
*/
QJsonObject QJsonBuffer::readMessage()
{
    QJsonObject obj;
    if (messageAvailable()) {
        QScopedPointer<QMutexLocker> locker(createLocker());
        switch (mFormat) {
        case FormatUndefined:
            break;
        case FormatUTF8:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset, mParserOffset - mParserStartOffset);
                obj = QJsonDocument::fromJson(msg).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset);
                resetParser();
            }
            break;
        case FormatUTF16BE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16BE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*2);
                resetParser();
            }
            break;
        case FormatUTF16LE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16LE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*2);
                resetParser();
            }
            break;
        case FormatUTF32BE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 4, 4*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-32BE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*4);
                resetParser();
            }
            break;
        case FormatUTF32LE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 4, 4*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-32LE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*4);
                resetParser();
            }
            break;
        case FormatBSON:
            if (mMessageSize > 0) {
                QByteArray msg = rawData(4, mMessageSize);
                obj = QJsonDocument::fromVariant(BsonObject(msg).toMap()).object();
                mBuffer.remove(0, mMessageSize+4);
                mMessageSize = 0;
            }
            break;
        case FormatQBJS:
            if (mMessageSize > 0) {
                QByteArray msg = rawData(0, mMessageSize);
                obj = QJsonDocument::fromBinaryData(msg).object();
                mBuffer.remove(0, mMessageSize);
                mMessageSize = 0;
            }
            break;
        }
        mMessageAvailable = false;
    }
    return obj;
}

/*!
  Return the current encoding format used by the receive buffer
*/

EncodingFormat QJsonBuffer::format() const
{
    return mFormat;
}

QMutexLocker *QJsonBuffer::createLocker()
{
    return mThreadProtection ? new QMutexLocker(&mMutex) : 0;
}

/*!
    \fn void QJsonBuffer::readyReadMessage()

    This signal is emitted once every time new data is appended to the buffer
    and a message is ready. \b readMessage() should be used to retrieve a message
    and \b messageAvailable() to check for next available messages.
    The client code may look like this:

    \code
    ...
    connect(jsonbuffer, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    ...

    void Foo::processMessages()
    {
        while (jsonbuffer->messageAvailable()) {
            QJsonObject obj = jsonbuffer->readMessage();
            <process message>
        }
    }
    \endcode

    \b readyReadMessage() is not emitted recursively; if you reenter the event loop
    inside a slot connected to the \b readyReadMessage() signal, the signal will not
    be reemitted.
*/

#include "moc_qjsonbuffer_p.cpp"

QT_END_NAMESPACE_JSONSTREAM

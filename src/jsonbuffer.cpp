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

#include "jsonbuffer_p.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class JsonBuffer
  \brief The JsonBuffer class parses data received into appropriate Json messages

  The JsonBuffer class wraps an internal buffer.  As you append
  data, the JsonBuffer object parses for received Json objects and
  raises an appropriate signal for each received object.

  \warning This class does not support JSON arrays.  Arrays that are received will be ignored.
*/

/*!
  Construct a JsonBuffer object with the given \a parent
*/

JsonBuffer::JsonBuffer(QObject *parent)
    : QObject(parent)
    , mFormat(FormatUndefined)
    , mParserState(ParseNormal)
    , mParserDepth(0)
    , mParserOffset(0)
    , mParserStartOffset(-1)
{
}

/*!
  Append the contents of a byte array \a data onto the buffer.
  During the execution of this function, the objectReceived
  signal may be raised.
*/

void JsonBuffer::append(const QByteArray& data)
{
    mBuffer.append(data.data(), data.size());
    processMessages();
}

/*!
  Append the \a data pointer with length \a len onto the JsonBuffer.
  During the execution of this function, objectReceived
  signal may be raised.
*/

void JsonBuffer::append(const char *data, int len)
{
    mBuffer.append(data, len);
    processMessages();
}

/*!
  Copy data from a file descriptor \a fd into the buffer.
  This function tries to eliminate extra data copy operations.
  It assumes that the file descriptor is ready to read and
  it does not try to read all of the data.

  Returns the number of bytes read or -1 for an error condition.
 */

int JsonBuffer::copyFromFd(int fd)
{
    const int maxcopy = 1024;
    uint oldSize = mBuffer.size();
    mBuffer.resize(oldSize + maxcopy);
    int n = ::read(fd, mBuffer.data()+oldSize, maxcopy);
    if (n > 0) {
        mBuffer.resize(oldSize+n);
        processMessages();
    }
    else
        mBuffer.resize(oldSize);
    return n;
}

/*!
    Clear the contents of the buffer.
 */

void JsonBuffer::clear()
{
    mBuffer.clear();
}

/*!
  \internal
*/

bool JsonBuffer::scanUtf( int c )
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
                mParserOffset++;
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

void JsonBuffer::resetParser()
{
    mParserState  = ParseNormal;
    mParserDepth  = 0;
    mParserOffset = 0;
    mParserStartOffset = -1;
}

/*!
  \internal
*/

void JsonBuffer::processMessages()
{
    if (mFormat == FormatUndefined && mBuffer.size() >= 4) {
        if (strncmp("bson", mBuffer.data(), 4) == 0)
            mFormat = FormatBSON;
        else if (QJsonDocument::BinaryFormatTag == *((uint *) mBuffer.data()))
            mFormat = FormatQBJS;
        else if (mBuffer.at(0) == 0 &&
                 mBuffer.at(1) != 0 &&
                 mBuffer.at(2) == 0 &&
                 mBuffer.at(3) != 0 )
            mFormat = FormatUTF16BE;
        else if (mBuffer.at(0) != 0 &&
                 mBuffer.at(1) == 0 &&
                 mBuffer.at(2) != 0 &&
                 mBuffer.at(3) == 0 )
            mFormat = FormatUTF16LE;
        else
            mFormat = FormatUTF8;
    }

    switch (mFormat) {
    case FormatUndefined:
        break;
    case FormatUTF8:
        for (  ; mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            char c = mBuffer.at(mParserOffset);
            if (scanUtf(c)) {
                QByteArray msg = mBuffer.mid(mParserStartOffset, mParserOffset - mParserStartOffset);
                QJsonObject obj = QJsonDocument::fromJson(msg).object();
                if (!obj.isEmpty())
                    emit objectReceived(obj);
                mBuffer = mBuffer.mid(mParserOffset);
                resetParser();
            }
        }
        break;
    case FormatUTF16BE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromBigEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (scanUtf(c)) {
                QByteArray msg = mBuffer.mid(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16BE")->toUnicode(msg);
                QJsonObject obj = QJsonDocument::fromJson(s.toUtf8()).object();
                if (!obj.isEmpty())
                    emit objectReceived(obj);
                mBuffer = mBuffer.mid(mParserOffset*2);
                resetParser();
            }
        }
        break;
    case FormatUTF16LE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromLittleEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (scanUtf(c)) {
                QByteArray msg = mBuffer.mid(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16LE")->toUnicode(msg);
                QJsonObject obj = QJsonDocument::fromJson(s.toUtf8()).object();
                if (!obj.isEmpty())
                    emit objectReceived(obj);
                mBuffer = mBuffer.mid(mParserOffset*2);
                resetParser();
            }
        }
        break;
    case FormatBSON:
        while (mBuffer.size() >= 8) {
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.data())[1]);
            if (mBuffer.size() < message_size + 4)
                break;
            QByteArray msg = mBuffer.mid(4, message_size);
            QJsonObject obj = QJsonDocument::fromVariant(BsonObject(msg).toMap()).object();
            if (!obj.isEmpty())
                emit objectReceived(obj);
            mBuffer = mBuffer.mid(message_size+4);
        }
        break;
    case FormatQBJS:
        while (mBuffer.size() >= 12) {
            // ### TODO: Should use 'sizeof(Header)'
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.data())[2]) + 8;
            if (mBuffer.size() < message_size)
                break;
            QByteArray msg = mBuffer.left(message_size);
            QJsonObject obj = QJsonDocument::fromBinaryData(msg).object();
            if (!obj.isEmpty())
                emit objectReceived(obj);
            mBuffer = mBuffer.mid(message_size);
        }
        break;
    }
}

/*!
  Return the current encoding format used by the receive buffer
*/

EncodingFormat JsonBuffer::format() const
{
    return mFormat;
}

/*!
    \fn void JsonBuffer::objectReceived(const QJsonObject& object)
    This signal is emitted when a new Qt Binary Json \a object has been received on the
    stream.
*/

#include "moc_jsonbuffer_p.cpp"

QT_END_NAMESPACE_JSONSTREAM

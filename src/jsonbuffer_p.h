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

#ifndef _JSON_BUFFER_H
#define _JSON_BUFFER_H

#include <QObject>
#include <QByteArray>
#include <QJsonObject>

#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT JsonBuffer : public QObject
{
    Q_OBJECT
public:
    JsonBuffer(QObject *parent=0);
    void append(const QByteArray& data);
    void append(const char* data, int len);
    int  copyFromFd(int fd);
    void clear();

    EncodingFormat  format() const;

    bool messageAvailable();
    QJsonObject readMessage();

    int size() const { return mBuffer.size(); }

    inline bool isEnabled() const { return mEnabled; }
    inline void setEnabled(bool enable) { mEnabled = enable; }

signals:
    void readyReadMessage();

private:
    void processMessages();
    bool scanUtf(int c);
    void resetParser();
    QByteArray rawData(int _start, int _len) const;

private:
    enum UTF8ParsingState { ParseNormal, ParseInString, ParseInBackslash };

    EncodingFormat   mFormat;
    QByteArray       mBuffer;
    UTF8ParsingState mParserState;
    int              mParserDepth;
    int              mParserOffset;
    int              mParserStartOffset;
    bool             mEmittedReadyRead;
    bool             mMessageAvailable;
    int              mMessageSize;
    bool             mEnabled;
};

inline QByteArray JsonBuffer::rawData(int _start, int _len) const
{
    return QByteArray::fromRawData(mBuffer.constData() + _start, _len);
}

QT_END_NAMESPACE_JSONSTREAM

#endif  // _JSON_BUFFER_H

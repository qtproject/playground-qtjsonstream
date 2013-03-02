/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtAddOn.JsonStream module of the Qt.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef _JSON_STREAM_H
#define _JSON_STREAM_H

#include <QIODevice>
#include <QJsonObject>
#include "qjsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonBuffer;

class QJsonStreamPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonStream : public QObject
{
    Q_OBJECT
public:
    QJsonStream(QIODevice *device = 0);
    virtual ~QJsonStream();

    bool atEnd() const;
    bool isOpen() const;

    QIODevice *device() const;
    void       setDevice( QIODevice *device );

    bool send(const QJsonObject& message);

    EncodingFormat format() const;
    void           setFormat(EncodingFormat format);

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64);

    qint64 writeBufferSize() const;
    void setWriteBufferSize(qint64 sz);

    qint64 bytesToWrite() const;

    bool messageAvailable();
    QJsonObject readMessage();

    enum QJsonStreamError
    {
        NoError = 0,
        WriteFailedNoConnection,
        MaxReadBufferSizeExceeded,
        MaxWriteBufferSizeExceeded,
        WriteFailed,
        WriteFailedReturnedZero
    };

    QJsonStreamError lastError() const;

signals:
    void bytesWritten(qint64);
    void readyReadMessage();
    void aboutToClose();
    void readBufferOverflow(qint64);

protected slots:
    void dataReadyOnSocket();
    void messageReceived();

protected:
    bool sendInternal(const QByteArray& byteArray);

private:
    friend class QJsonConnectionProcessor;
    void setThreadProtection(bool) const;

private:
    Q_DECLARE_PRIVATE(QJsonStream)
    QScopedPointer<QJsonStreamPrivate> d_ptr;
};

QT_END_NAMESPACE_JSONSTREAM

#endif  // _JSON_STREAM_H

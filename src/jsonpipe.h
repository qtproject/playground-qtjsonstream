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

#ifndef _JSON_PIPE_H
#define _JSON_PIPE_H

#include <QObject>
#include <QJsonObject>
#include "jsonstream-global.h"

class QSocketNotifier;

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonBuffer;

class JsonPipePrivate;
class Q_ADDON_JSONSTREAM_EXPORT JsonPipe : public QObject
{
    Q_OBJECT
public:
    JsonPipe(QObject *parent = 0);
    virtual ~JsonPipe();

    bool writeEnabled() const;
    bool readEnabled() const;

    Q_INVOKABLE bool send(const QJsonObject& message);

    EncodingFormat format() const;
    void           setFormat(EncodingFormat format);

    void setFds(int in_fd, int out_fd);

    enum PipeError { WriteFailed, WriteAtEnd, ReadFailed, ReadAtEnd };
    Q_ENUMS(PipeError);

    bool waitForBytesWritten(int msecs = 30000);

signals:
    void messageReceived(const QJsonObject& message);
    void error(PipeError);

protected slots:
    void objectReceived(const QJsonObject& object);
    void inReady(int fd);
    void outReady(int fd);

protected:
    void sendInternal(const QByteArray& byteArray);

private:
    int writeInternal(int fd);

private:
    Q_DECLARE_PRIVATE(JsonPipe)
    QScopedPointer<JsonPipePrivate> d_ptr;
};

JsonPipe& operator<<( JsonPipe&, const QJsonObject& );

QT_END_NAMESPACE_JSONSTREAM

#endif  // _JSON_PIPE_H

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

#ifndef _JSON_PIPE_H
#define _JSON_PIPE_H

#include <QObject>
#include <QJsonObject>
#include "qjsonstream-global.h"

class QSocketNotifier;

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonBuffer;

class QJsonPipePrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonPipe : public QObject
{
    Q_OBJECT
public:
    QJsonPipe(QObject *parent = 0);
    virtual ~QJsonPipe();

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
    void processMessages();
    void objectReceived(const QJsonObject& object);
    void inReady(int fd);
    void outReady(int fd);

private:
    int writeInternal(int fd);

private:
    Q_DECLARE_PRIVATE(QJsonPipe)
    QScopedPointer<QJsonPipePrivate> d_ptr;
};

QJsonPipe& operator<<(QJsonPipe&, const QJsonObject& );

QT_END_NAMESPACE_JSONSTREAM

#endif  // _JSON_PIPE_H

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

#ifndef _JSON_SERVER_H
#define _JSON_SERVER_H

#include <QObject>
#include <QJsonObject>

#include "qjsonstream-global.h"
#include "qjsonschemaerror.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class QJsonAuthority;
class QJsonSchemaValidator;

class QJsonServerPrivate;
class Q_ADDON_JSONSTREAM_EXPORT QJsonServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ValidatorFlags validatorFlags READ validatorFlags WRITE setValidatorFlags)

public:
    QJsonServer(QObject *parent = 0);
    virtual ~QJsonServer();

    bool listen(int port, QJsonAuthority *authority = 0);
    bool listen(const QString& socketname,QJsonAuthority *authority = 0);

    QStringList connections() const;

    void enableQueuing(const QString &identifier);
    void disableQueuing(const QString &identifier);
    bool isQueuingEnabled(const QString &identifier) const;
    void clearQueue(const QString &identifier);

    void enableMultipleConnections(const QString& identifier);
    void disableMultipleConnections(const QString& identifier);

    // schema validation
    enum ValidatorFlag {
        NoValidation = 0x0,
        DropIfInvalid = 0x1,
        WarnIfInvalid = 0x2,
        ApplyDefaultValues = 0x4 // TODO
    };
    Q_DECLARE_FLAGS(ValidatorFlags, ValidatorFlag)

    ValidatorFlags validatorFlags() const;
    void setValidatorFlags(ValidatorFlags);

    QJsonSchemaValidator *inboundValidator();
    QJsonSchemaValidator *outboundValidator();

public slots:
    bool hasConnection(const QString &identifier) const;
    bool send(const QString &identifier, const QJsonObject& message);
    void broadcast(const QJsonObject& message);
    void removeConnection(const QString &identifier);

signals:
    void connectionAdded(const QString &identifier);
    void connectionRemoved(const QString &identifier);
    void messageReceived(const QString &identifier, const QJsonObject &message);
    void authorizationFailed();

    void inboundMessageValidationFailed(const QJsonObject &message, const QtAddOn::QtJsonStream::QJsonSchemaError &error);
    void outboundMessageValidationFailed(const QJsonObject &message, const QtAddOn::QtJsonStream::QJsonSchemaError &error);

protected slots:
    virtual void handleClientAuthorized(const QString &identifier);
    virtual void handleAuthorizationFailed();
    virtual void receiveMessage(const QString &identifier, const QJsonObject &message);
    virtual void clientDisconnected(const QString& identifier);

private slots:
    void handleLocalConnection();

private:
    void initSchemaValidation();

private:
    Q_DECLARE_PRIVATE(QJsonServer)
    QScopedPointer<QJsonServerPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QJsonServer::ValidatorFlags)

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_SERVER_H

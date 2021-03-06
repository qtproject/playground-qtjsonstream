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

#ifndef SCHEMAMANAGER_P_H
#define SCHEMAMANAGER_P_H

#include <QtCore/qstring.h>
#include <QtCore/qpair.h>
#include <QtCore/qmap.h>

#include "schemaobject_p.h"

#include "qjsonschema-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class SchemaManagerBase
{

};

//FIXME This can have better performance
template<class T, class TT>
class SchemaManager : public SchemaManagerBase
{
public:
    typedef typename TT::Service TypesService;

    inline bool contains(const QString &name) const;
    inline T value(const QString &name) const;
    inline SchemaValidation::Schema<TT> schema(const QString &name, TypesService *service);
    inline T take(const QString &name);
    inline T insert(const QString &name, T &schema);

    inline T validate(const QString &schemaName, T object);

    QStringList names() const { return m_schemas.keys(); }
    inline QMap<QString, T> schemas() const;

    bool isEmpty() const { return m_schemas.isEmpty(); }
    void clear() { m_schemas.clear(); }

private:
    typedef QPair<T, SchemaValidation::Schema<TT> > MapSchemaPair;
    inline T ensureCompiled(const QString &schemaName, MapSchemaPair *pair, TypesService *service);

    QMap<QString, MapSchemaPair> m_schemas;
};

QT_END_NAMESPACE_JSONSTREAM

#include "schemamanager_impl_p.h"

#endif // SCHEMAMANAGER_P_H

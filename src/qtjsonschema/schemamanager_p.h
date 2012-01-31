/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef SCHEMAMANAGER_P_H
#define SCHEMAMANAGER_P_H

#include <QtCore/qstring.h>
#include <QtCore/qpair.h>
#include <QtCore/qmap.h>

#include "schemaobject_p.h"

#include "jsonschema-global.h"

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

private:
    typedef QPair<T, SchemaValidation::Schema<TT> > MapSchemaPair;
    inline T ensureCompiled(const QString &schemaName, MapSchemaPair *pair, TypesService *service);

    QMap<QString, MapSchemaPair> m_schemas;
};

QT_END_NAMESPACE_JSONSTREAM

#include "schemamanager_impl_p.h"

#endif // SCHEMAMANAGER_P_H

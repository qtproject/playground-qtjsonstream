/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation
**
****************************************************************************/


#include "qt-bson_p.h"
#include <QJSValueIterator>

BsonObject::BsonObject(QJSValue sv)
    : d(new BsonData())
{
    if (sv.isArray()) {
        d->mBsonType = bson_array;
    }
    if (sv.isObject()) {
        QJSValueIterator it(sv);
        while (it.hasNext()) {
            it.next();
            QJSValue element = it.value();
            QVariant v = element.toVariant();
            if (element.isCallable()) {
                qDebug() << it.name() << v;
                v = element.toString();
            }
            insert(it.name().toLatin1(), v);
        }
    }
}

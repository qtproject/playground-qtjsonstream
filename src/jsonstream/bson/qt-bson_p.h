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

#ifndef JsondbBson_H
#define JsondbBson_H

#include <QByteArray>
#include <QDebug>
#include <QObject>
#include <QSharedData>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "bson_p.h"

class BsonObject;
QDebug operator<<(QDebug d, BsonObject bson);

class BsonData : public QSharedData {
public:
    BsonData();
    ~BsonData();
private:
    bson_buffer mBsonBuffer;
    bson        mBson;
    bson_type   mBsonType; // bson_object or bson_array
    QSharedPointer<BsonData> p;
    QSet<QString> mAddedKeys;

    friend class BsonObject;
    friend QDebug operator<<(QDebug d, BsonObject bson);
};

typedef QList<BsonObject> BsonList;

class BsonObject {
public:
    BsonObject();
    BsonObject(char *data, int size);
    BsonObject(const QByteArray &data);
    BsonObject(const BsonObject &);
    BsonObject(const QVariantMap &);
    BsonObject(const QVariantList &);
private:
    BsonObject(bson_iterator *it, const BsonObject *parent, bson_type bt);
public:

    QVariantMap toMap();
    QVariantList toList();
    QList<BsonObject> toBsonList();
    QByteArray data();
    int size(); // same as count()
    int count();
    int dataSize(); // bytes of data after encoding
    QVariant::Type type() const { return (d->mBsonType == bson_object) ? QVariant::Map : QVariant::List; }
    BsonObject &insert(const QString &key, int);
    BsonObject &insert(const QString &key, quint32);
    BsonObject &insert(const QString &key, double);
    BsonObject &insert(const QString &key, bool);
    BsonObject &insert(const QString &key, const char *s);
    BsonObject &insert(const QString &key, const QString &);
    BsonObject &insert(const QString &key, const QLatin1String &);
    BsonObject &insert(const QString &key, const QVariant &);
    BsonObject &insert(const QString &key, const QVariantMap &);
    BsonObject &insert(const QString &key, const QVariantList &);
    BsonObject &insert(const QString &key, const QStringList &);
    BsonObject &insert(const QString &key, BsonObject);
    BsonObject &insert(const QString &key, const BsonList &);

    //bool isValid() { return true; };
    bool isEmpty();
    QList<QString> keys();
    bool contains(const QString &);
    //bool remove(const QByteArray &);
    BsonObject subObject(const QString &key);
    QList<BsonObject> subList(const QString &key);
    QVariant value(const QString &key);
    int valueInt(const QString &key, int default_value);
    QString valueString(const QString &key);

    void dump();

    class const_iterator {
    public:
      bool operator!=(const_iterator) const;
      void operator++();
      QByteArray key();
      QVariant value();
    };
    const_iterator begin() const;
    const_iterator end() const;

protected:
    bool start();
    void finish();
    QVariant elementToVariant(bson_type bt, bson_iterator *it);

    QSharedPointer<BsonData> d;
    friend QDebug operator<<(QDebug d, BsonObject bson);
};


#if 0
class BsonList : public BsonObject {
public:
    class const_iterator {
    public:
        bool operator!=(const_iterator other) { return (mIndex != other.mIndex); };
        void operator++() { mIndex++; };
        QVariant operator*() { return mList->value(QByteArray::number(mIndex)); }
    private:
    const_iterator(BsonList *l, int index) : mList(l), mIndex(index) {}
    private:
        BsonList *mList;
        int mIndex;
        friend class BsonList;
    };

    BsonList();
    BsonList(const QVariantList&);
    int length();
    BsonList mid(int start, int len=0);
    void append(QVariant v);
    void append(BsonObject o);
    const_iterator begin() { return const_iterator(this, 0); };
    const_iterator end() { return const_iterator(this, count()); };

};
#endif

QDebug operator<<(QDebug, BsonObject);
Q_DECLARE_METATYPE(BsonObject)

#endif

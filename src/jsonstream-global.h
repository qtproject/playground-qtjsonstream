/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef JSONSTREAM_GLOBAL_H
#define JSONSTREAM_GLOBAL_H

#include "qglobal.h"

#if defined(QT_ADDON_JSONSTREAM_LIB)
#  define Q_ADDON_JSONSTREAM_EXPORT Q_DECL_EXPORT
#else
#  define Q_ADDON_JSONSTREAM_EXPORT Q_DECL_IMPORT
#endif

#if defined(QT_NAMESPACE)
#  define QT_BEGIN_NAMESPACE_JSONSTREAM namespace QT_NAMESPACE { namespace QtAddOn { namespace JsonStream {
#  define QT_END_NAMESPACE_JSONSTREAM } } }
#  define QT_USE_NAMESPACE_JSONSTREAM using namespace QT_NAMESPACE::QtAddOn::JsonStream;
#  define QT_PREPEND_NAMESPACE_JSONSTREAM(name) ::QT_NAMESPACE::QtAddOn::JsonStream::name
#else
#  define QT_BEGIN_NAMESPACE_JSONSTREAM namespace QtAddOn { namespace JsonStream {
#  define QT_END_NAMESPACE_JSONSTREAM } }
#  define QT_USE_NAMESPACE_JSONSTREAM using namespace QtAddOn::JsonStream;
#  define QT_PREPEND_NAMESPACE_JSONSTREAM(name) ::QtAddOn::JsonStream::name
#endif

#define QT_JSONSTREAM_DECLARE_METATYPE_PTR(name)  Q_DECLARE_METATYPE(QtAddOn::JsonStream::name *)
#define QT_JSONSTREAM_DECLARE_METATYPE_CONST_PTR(name)  Q_DECLARE_METATYPE(const QtAddOn::JsonStream::name *)


QT_BEGIN_NAMESPACE_JSONSTREAM

enum EncodingFormat { FormatUndefined, FormatUTF8, FormatBSON, FormatQBJS, FormatUTF16BE, FormatUTF16LE, FormatUTF32BE, FormatUTF32LE };

QT_END_NAMESPACE_JSONSTREAM

#endif // JSONSTREAM_GLOBAL_H

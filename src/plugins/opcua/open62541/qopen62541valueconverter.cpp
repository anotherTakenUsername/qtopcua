/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtOpcUa module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopen62541valueconverter.h"
#include "qopen62541utils.h"

#include <QDateTime>
#include <qopen62541.h>

#include <QtCore/QDebug>

// TODO
QOpcUa::Types QOpen62541ValueConverter::qvariantTypeToQOpcUaType(QVariant::Type type)
{
    switch (type) {
    case QVariant::Bool:
        return QOpcUa::Boolean;
    case QVariant::Int:
        return QOpcUa::Int32;
    case QVariant::UInt:
        return QOpcUa::UInt32;
    case QVariant::Double:
        return QOpcUa::Double;
    //return QOpcUa::Float;
    case QVariant::String:
        return QOpcUa::String;
    //return QOpcUa::LocalizedText;
    case QVariant::DateTime:
        return QOpcUa::DateTime;
    //return QOpcUa::UInt16;
    //return QOpcUa::Int16;
    //return QOpcUa::UInt64;
    //return QOpcUa::Int64;
    //return QOpcUa::Byte;
    //return QOpcUa::SByte;
    //return QOpcUa::ByteString;
    //return QOpcUa::XmlElement;
    //return QOpcUa::NodeId;
    //return QOpcUa::Undefined;
    }

    return QOpcUa::Undefined;
}

UA_Variant QOpen62541ValueConverter::toOpen62541Variant(const QVariant &value, QOpcUa::Types type)
{
    UA_Variant open62541value;
    UA_Variant_init(&open62541value);

    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.isEmpty())
            return open62541value;

        QOpcUa::Types listValueType = type == QOpcUa::Undefined ? qvariantTypeToQOpcUaType(list.at(0).type()) : type;
        const UA_DataType *dt;
        void *arr;
        switch (listValueType) {
        case QOpcUa::Boolean:
            dt = &UA_TYPES[UA_TYPES_BOOLEAN];
            break;
        case QOpcUa::Int32:
            dt = &UA_TYPES[UA_TYPES_INT32];
            break;
        case QOpcUa::UInt32:
            dt = &UA_TYPES[UA_TYPES_UINT32];
            break;
        case QOpcUa::Double:
            dt = &UA_TYPES[UA_TYPES_DOUBLE];
            break;
        case QOpcUa::Float:
            dt = &UA_TYPES[UA_TYPES_FLOAT];
            break;
        case QOpcUa::String:
            dt = &UA_TYPES[UA_TYPES_STRING];
            break;
        case QOpcUa::LocalizedText:
            dt = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
            break;
        case QOpcUa::DateTime:
            dt = &UA_TYPES[UA_TYPES_DATETIME];
            break;
        case QOpcUa::UInt16:
            dt = &UA_TYPES[UA_TYPES_UINT16];
            break;
        case QOpcUa::Int16:
            dt = &UA_TYPES[UA_TYPES_INT16];
            break;
        case QOpcUa::UInt64:
            dt = &UA_TYPES[UA_TYPES_UINT64];
            break;
        case QOpcUa::Int64:
            dt = &UA_TYPES[UA_TYPES_INT64];
            break;
        case QOpcUa::Byte:
            dt = &UA_TYPES[UA_TYPES_BYTE];
            break;
        case QOpcUa::SByte:
            dt = &UA_TYPES[UA_TYPES_SBYTE];
            break;
        case QOpcUa::ByteString:
            dt = &UA_TYPES[UA_TYPES_BYTESTRING];
            break;
        case QOpcUa::XmlElement:
            dt = &UA_TYPES[UA_TYPES_XMLELEMENT];
            break;
        case QOpcUa::NodeId:
            dt = &UA_TYPES[UA_TYPES_NODEID];
            break;
        default:
            qWarning() << "Trying to create array of undefined type:" << listValueType;
            return open62541value;
        }
        arr = UA_Array_new(list.size(), dt);

        for (int i = 0; i < list.size(); ++i) {
            switch (listValueType) {
            case QOpcUa::Boolean: {
                UA_Boolean *bA = reinterpret_cast<UA_Boolean *>(arr);
                bA[i] = list[i].toBool();
                break;
            }
            case QOpcUa::Int32: {
                UA_Int32 *p = reinterpret_cast<UA_Int32 *>(arr);
                p[i] = list[i].toInt();
                break;
            }
            case QOpcUa::UInt32: {
                UA_UInt32 *p = reinterpret_cast<UA_UInt32 *>(arr);
                p[i] = list[i].toUInt();
                break;
            }
            case QOpcUa::Double: {
                UA_Double *p = reinterpret_cast<UA_Double *>(arr);
                p[i] = list[i].toDouble();
                break;
            }
            case QOpcUa::Float: {
                UA_Float *p = reinterpret_cast<UA_Float *>(arr);
                p[i] = list[i].toFloat();
                break;
            }
            case QOpcUa::String:
            case QOpcUa::ByteString: {
                UA_String *p = reinterpret_cast<UA_String *>(arr);
                p[i] = UA_String_fromChars(list[i].toString().toLocal8Bit().constData());
                break;
            }
            case QOpcUa::LocalizedText: {
                UA_LocalizedText *p = reinterpret_cast<UA_LocalizedText *>(arr);
                UA_String_init(&p[i].locale);
                p[i].text = UA_String_fromChars(list[i].toString().toLocal8Bit().constData());
                break;
            }
            case QOpcUa::DateTime: {
                UA_DateTime *p = reinterpret_cast<UA_DateTime *>(arr);
                QDateTime dt = list[i].toDateTime();
                p[i] = UA_MSEC_TO_DATETIME * dt.toMSecsSinceEpoch();
                break;
            }
            case QOpcUa::UInt16: {
                UA_UInt16 *p = reinterpret_cast<UA_UInt16 *>(arr);
                p[i] = list[i].toUInt();
                break;
            }
            case QOpcUa::Int16: {
                UA_Int16 *p = reinterpret_cast<UA_Int16 *>(arr);
                p[i] = list[i].toInt();
                break;
            }
            case QOpcUa::UInt64: {
                UA_UInt64 *p = reinterpret_cast<UA_UInt64 *>(arr);
                p[i] = list[i].toULongLong();
                break;
            }
            case QOpcUa::Int64: {
                UA_Int64 *p = reinterpret_cast<UA_Int64 *>(arr);
                p[i] = list[i].toLongLong();
                break;
            }
            case QOpcUa::Byte:
            case QOpcUa::SByte: {
                UA_SByte *p = reinterpret_cast<UA_SByte *>(arr);
                p[i] = *reinterpret_cast<const UA_SByte *>(list[i].constData());
                break;
            }
            case QOpcUa::NodeId: {
                UA_NodeId *p = reinterpret_cast<UA_NodeId *>(arr);
                p[i] = Open62541Utils::nodeIdFromQString(list[i].toString());
                break;
            }
            case QOpcUa::XmlElement:
            case QOpcUa::Undefined:
            default:
                qWarning() << "VariantList conversion to Open62541 for typeIndex" << listValueType << " not implemented";
                qFatal("");
            }
        }
        UA_Variant_setArray(&open62541value, arr, list.size(), dt);
        return open62541value;
    }

    QOpcUa::Types valueType = type == QOpcUa::Undefined ? qvariantTypeToQOpcUaType(value.type()) : type;

    switch (valueType) {
    case QOpcUa::Boolean: {
        UA_Boolean tmpValue = value.toBool();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    }
    case QOpcUa::SByte: {
        UA_SByte tmpValue = *static_cast<const UA_SByte *>(value.constData());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_SBYTE]);
        break;
    }
    case QOpcUa::Byte: {
        UA_Byte tmpValue = *static_cast<const UA_Byte *>(value.constData());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_BYTE]);
        break;
    }
    case QOpcUa::Int16: {
        UA_Int16 tmpValue = value.toInt();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_INT16]);
        break;
    }
    case QOpcUa::UInt16: {
        UA_UInt16 tmpValue = value.toUInt();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_UINT16]);
        break;
    }
    case QOpcUa::Int32: {
        UA_Int32 tmpValue = value.toInt();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_INT32]);
        break;
    }
    case QOpcUa::UInt32: {
        UA_UInt32 tmpValue = value.toUInt();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    }
    case QOpcUa::Int64: {
        UA_Int64 tmpValue = value.toLongLong();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_INT64]);
        break;
    }
    case QOpcUa::UInt64: {
        UA_UInt64 tmpValue = value.toULongLong();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_UINT64]);
        break;
    }
    case QOpcUa::Float: {
        UA_Float tmpValue = value.toFloat();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_FLOAT]);
        break;
    }
    case QOpcUa::Double: {
        UA_Double tmpValue = value.toDouble();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    }
    case QOpcUa::DateTime: {
        UA_DateTime tmpValue = UA_MSEC_TO_DATETIME * value.toDateTime().toMSecsSinceEpoch();
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_DATETIME]);
        break;
    }
    case QOpcUa::String: {
        UA_String tmpValue = UA_String_fromChars(value.toString().toLocal8Bit().constData());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_STRING]);
        break;
    }
    case QOpcUa::LocalizedText: {
        UA_LocalizedText tmpValue;
        UA_String_init(&tmpValue.locale);
        tmpValue.text = UA_String_fromChars(value.toString().toLocal8Bit().constData());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    }
    case QOpcUa::ByteString: {
        QByteArray str = value.toString().toLocal8Bit();
        UA_ByteString tmpValue = UA_BYTESTRING(str.data());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_BYTESTRING]);
        break;
    }
    case QOpcUa::NodeId: {
        UA_NodeId tmpValue = Open62541Utils::nodeIdFromQString(value.toString());
        UA_Variant_setScalarCopy(&open62541value, &tmpValue, &UA_TYPES[UA_TYPES_NODEID]);
        break;
    }
    default:
        qWarning() << "Variant conversion to Open62541 for typeIndex" << type << " not implemented";
    }

    return open62541value;
}

#define SIMPLE_TYPE_CASE(uat, qmt, qtt) \
    case uat:\
    return QVariant(qmt, static_cast<qtt *>(value.data))

#define SIMPLE_TYPE_CASE_LIST(uat, qmt, qtt) \
    case uat:\
    list.append(QVariant(qmt, &static_cast<qtt *>(value.data)[i]));\
    break

QVariant QOpen62541ValueConverter::toQVariant(const UA_Variant &value)
{
    if (value.arrayLength > 0) {
        QVariantList list;
        for (size_t i = 0; i < value.arrayLength; ++i) {
            switch (value.type->typeIndex) {
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_BOOLEAN, QMetaType::Bool, bool);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_SBYTE, QMetaType::SChar, char);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_BYTE, QMetaType::UChar, uchar);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_INT16, QMetaType::Short, qint16);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_UINT16, QMetaType::UShort, quint16);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_INT32, QMetaType::Int, qint32);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_UINT32, QMetaType::UInt, quint32);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_INT64, QMetaType::LongLong, qint64);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_UINT64, QMetaType::ULongLong, quint64);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_FLOAT, QMetaType::Float, float);
            SIMPLE_TYPE_CASE_LIST(UA_TYPES_DOUBLE, QMetaType::Double, double);
            case UA_TYPES_BYTESTRING: {
                UA_ByteString *ua_bs = (UA_ByteString *) value.data;
                QByteArray arr = QByteArray(reinterpret_cast<char *>(ua_bs[i].data), ua_bs[i].length);
                list.append(QVariant(QMetaType::QByteArray, &arr));
                break;
            }
            case UA_TYPES_STRING: {
                UA_String *ua_str = (UA_String *) value.data;
                list.append(QVariant(QString::fromLocal8Bit(reinterpret_cast<char*>(ua_str[i].data),
                                                            ua_str[i].length)));
                break;
            }
            case UA_TYPES_LOCALIZEDTEXT: {
                UA_LocalizedText *ua_lt = (UA_LocalizedText *) value.data;
                list.append(QVariant(QString::fromLocal8Bit(reinterpret_cast<char*>(ua_lt[i].text.data),
                                                            ua_lt[i].text.length)));
                break;
            }
            case UA_TYPES_DATETIME: {
                UA_DateTime *dt = (UA_DateTime *) value.data;
                list.append(QVariant(QDateTime::fromMSecsSinceEpoch(dt[i] * UA_DATETIME_TO_MSEC)));
                break;
            }
            case UA_TYPES_NODEID: {
                UA_NodeId *uan = (UA_NodeId *) value.data;
                const QString nstr = Open62541Utils::nodeIdToQString(uan[i]);
                list.append(QVariant(nstr));
                break;
            }
            default:
                qWarning() << "Variant list conversion from Open62541 for typeIndex" << value.type->typeIndex << " not implemented";
                return QVariant();
            }
        }
        QVariant v(list);
        return v;
    }

    switch (value.type->typeIndex) {
    SIMPLE_TYPE_CASE(UA_TYPES_BOOLEAN, QMetaType::Bool, bool);
    SIMPLE_TYPE_CASE(UA_TYPES_SBYTE, QMetaType::SChar, char);
    SIMPLE_TYPE_CASE(UA_TYPES_BYTE, QMetaType::UChar, uchar);
    SIMPLE_TYPE_CASE(UA_TYPES_INT16, QMetaType::Short, qint16);
    SIMPLE_TYPE_CASE(UA_TYPES_UINT16, QMetaType::UShort, quint16);
    SIMPLE_TYPE_CASE(UA_TYPES_INT32, QMetaType::Int, qint32);
    SIMPLE_TYPE_CASE(UA_TYPES_UINT32, QMetaType::UInt, quint32);
    SIMPLE_TYPE_CASE(UA_TYPES_INT64, QMetaType::LongLong, qint64);
    SIMPLE_TYPE_CASE(UA_TYPES_UINT64, QMetaType::ULongLong, quint64);
    SIMPLE_TYPE_CASE(UA_TYPES_FLOAT, QMetaType::Float, float);
    SIMPLE_TYPE_CASE(UA_TYPES_DOUBLE, QMetaType::Double, double);
    case UA_TYPES_STRING: {
        UA_String *ua_str = (UA_String*) value.data;
        return QVariant(QString::fromLocal8Bit(reinterpret_cast<char*>(ua_str->data), ua_str->length));
    }
    case UA_TYPES_LOCALIZEDTEXT: {
        UA_LocalizedText *ua_lt = (UA_LocalizedText *) value.data;
        return QVariant(QString::fromLocal8Bit(reinterpret_cast<char*>(ua_lt->text.data),
                                               ua_lt->text.length));
    }
    case UA_TYPES_NODEID: {
        UA_NodeId *uan = (UA_NodeId *) value.data;
        return Open62541Utils::nodeIdToQString(*uan);
    }
    case UA_TYPES_DATETIME:
        return QVariant(QDateTime::fromMSecsSinceEpoch(*reinterpret_cast<UA_DateTime *>(value.data) * UA_DATETIME_TO_MSEC));
    default:
        qWarning() << "Variant conversion from Open62541 for typeIndex" << value.type->typeIndex << " not implemented";
        return QVariant();
    }
    return QVariant();
}

QOpcUa::Types QOpen62541ValueConverter::toQOpcUaVariantType(quint8 typeIndex)
{
    switch (typeIndex) {
    case UA_TYPES_BOOLEAN:
        return QOpcUa::Boolean;
    case UA_TYPES_SBYTE:
        return QOpcUa::SByte;
    case UA_TYPES_BYTE:
        return QOpcUa::Byte;
    case UA_TYPES_INT16:
        return QOpcUa::Int16;
    case UA_TYPES_UINT16:
        return QOpcUa::UInt16;
    case UA_TYPES_INT32:
        return QOpcUa::Int32;
    case UA_TYPES_UINT32:
        return QOpcUa::UInt32;
    case UA_TYPES_INT64:
        return QOpcUa::Int64;
    case UA_TYPES_UINT64:
        return QOpcUa::UInt64;
    default:
        qWarning() << "Variant conversion to Qt type " << typeIndex << " not implemented";
        return QOpcUa::Undefined;
    }
    return QOpcUa::Undefined;
}

QString QOpen62541ValueConverter::toQString(UA_String value)
{
    return QString::fromUtf8((const char*) value.data, value.length);
}
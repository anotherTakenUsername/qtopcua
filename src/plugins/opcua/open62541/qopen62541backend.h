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

#include "qopen62541client.h"
#include <private/qopcuabackend_p.h>

#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QOpen62541Node;

class Open62541AsyncBackend : public QOpcUaBackend
{
    Q_OBJECT
public:
    Open62541AsyncBackend(QOpen62541Client *parent);

public Q_SLOTS:
    void connectToEndpoint(const QUrl &url);
    void disconnectFromEndpoint();

    // Node functions
    QStringList childrenIds(const UA_NodeId *parentNode);
    void readAttributes(uintptr_t handle, UA_NodeId id, QOpcUaNode::NodeAttributes attr);

    void writeAttribute(uintptr_t handle, UA_NodeId id, QOpcUaNode::NodeAttribute attrId, QVariant value, QOpcUa::Types type);
    void writeAttributes(uintptr_t handle, UA_NodeId id, QOpcUaNode::AttributeMap toWrite, QOpcUa::Types valueAttributeType);

    // Subscription
    UA_UInt32 createSubscription(int interval);
    void deleteSubscription(UA_UInt32 id);
    void updatePublishSubscriptionRequests() const;
    void activateSubscriptionTimer(int timeout);
    void removeSubscriptionTimer(int timeout);
public:
    QOpen62541Client *m_clientImpl;
    UA_Client *m_uaclient;
    QTimer *m_subscriptionTimer;
    QSet<int> m_subscriptionIntervals;
};

QT_END_NAMESPACE

/****************************************************************************
**
** Copyright (C) 2015 basysKom GmbH, opensource@basyskom.com
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

#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaMonitoredEvent>
#include <QtOpcUa/QOpcUaMonitoredValue>
#include <QtOpcUa/QOpcUaNode>
#include <QtOpcUa/QOpcUaProvider>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

class OpcuaConnector
{
public:
    OpcuaConnector(QOpcUaClient *client, const QString &endPoint)
        : opcuaClient(client)
    {
        QVERIFY(opcuaClient != nullptr);
        QSignalSpy connectedSpy(opcuaClient, &QOpcUaClient::connected);
        QSignalSpy disconnectedSpy(opcuaClient, &QOpcUaClient::disconnected);
        QSignalSpy stateSpy(opcuaClient, &QOpcUaClient::stateChanged);

        QTest::qWait(500);
        opcuaClient->connectToEndpoint(QUrl(endPoint));
        QTRY_VERIFY2(opcuaClient->state() == QOpcUaClient::Connected, "Could not connect to server");

        QCOMPARE(connectedSpy.count(), 1); // one connected signal fired
        QCOMPARE(disconnectedSpy.count(), 0); // zero disconnected signals fired
        QCOMPARE(stateSpy.count(), 2);

        QCOMPARE(stateSpy.at(0).at(0).value<QOpcUaClient::ClientState>(),
                 QOpcUaClient::ClientState::Connecting);
        QCOMPARE(stateSpy.at(1).at(0).value<QOpcUaClient::ClientState>(),
                 QOpcUaClient::ClientState::Connected);

        stateSpy.clear();
        connectedSpy.clear();
        disconnectedSpy.clear();

        QVERIFY(opcuaClient->url() == QUrl(endPoint));
    }

    ~OpcuaConnector()
    {
        QSignalSpy connectedSpy(opcuaClient, &QOpcUaClient::connected);
        QSignalSpy disconnectedSpy(opcuaClient, &QOpcUaClient::disconnected);
        QSignalSpy stateSpy(opcuaClient, &QOpcUaClient::stateChanged);

        QVERIFY(opcuaClient != nullptr);
        if (opcuaClient->state() == QOpcUaClient::Connected) {

            opcuaClient->disconnectFromEndpoint();

            QTRY_VERIFY(opcuaClient->state() == QOpcUaClient::Disconnected);

            QCOMPARE(connectedSpy.count(), 0);
            QCOMPARE(disconnectedSpy.count(), 1);
            QCOMPARE(stateSpy.count(), 2);
            QCOMPARE(stateSpy.at(0).at(0).value<QOpcUaClient::ClientState>(),
                     QOpcUaClient::ClientState::Closing);
            QCOMPARE(stateSpy.at(1).at(0).value<QOpcUaClient::ClientState>(),
                     QOpcUaClient::ClientState::Disconnected);
        }

        opcuaClient = nullptr;
    }

    QOpcUaClient *opcuaClient;
};

const QString readWriteNode = QStringLiteral("ns=3;s=TestNode.ReadWrite");
const QVector<QString> xmlElements = {
    QStringLiteral("<?xml version=\"1\" encoding=\"UTF-8\"?>"),
    QStringLiteral("<?xml version=\"2\" encoding=\"UTF-8\"?>"),
    QStringLiteral("<?xml version=\"3\" encoding=\"UTF-8\"?>")};
const QVector<QOpcUa::QLocalizedText> localizedTexts = {
    QOpcUa::QLocalizedText("en", "English"),
    QOpcUa::QLocalizedText("de", "German"),
    QOpcUa::QLocalizedText("fr", "French")};
const int numberOfOperations = 1000;

#define defineDataMethod(name) void name()\
{\
    QTest::addColumn<QOpcUaClient *>("opcuaClient");\
    for (auto *client: m_clients)\
        QTest::newRow(client->backend().toLatin1().constData()) << client;\
}

class Tst_QOpcUaClient: public QObject
{
    Q_OBJECT

public:
    Tst_QOpcUaClient();

private slots:
    void initTestCase();
    void cleanupTestCase();

    // connect & disconnect
    defineDataMethod(connectToInvalid_data)
    void connectToInvalid();
    defineDataMethod(secureConnect_data)
    void secureConnect();
    defineDataMethod(secureConnectToInvalid_data)
    void secureConnectToInvalid();
    defineDataMethod(connectAndDisconnect_data)
    void connectAndDisconnect();

    // Password
    defineDataMethod(connectInvalidPassword_data)
    void connectInvalidPassword();
    defineDataMethod(connectAndDisconnectPassword_data)
    void connectAndDisconnectPassword();

    defineDataMethod(readInvalidNode_data)
    void readInvalidNode();
    defineDataMethod(writeInvalidNode_data)
    void writeInvalidNode();
    defineDataMethod(writeMultipleAttributes_data)
    void writeMultipleAttributes();

    defineDataMethod(getRootNode_data)
    void getRootNode();
    defineDataMethod(getChildren_data)
    void getChildren();
    defineDataMethod(childrenIdsString_data)
    void childrenIdsString();
    defineDataMethod(childrenIdsGuidNodeId_data)
    void childrenIdsGuidNodeId();
    defineDataMethod(childrenIdsOpaqueNodeId_data)
    void childrenIdsOpaqueNodeId();

    defineDataMethod(dataChangeSubscription_data)
    void dataChangeSubscription();
    defineDataMethod(dataChangeSubscriptionInvalidNode_data)
    void dataChangeSubscriptionInvalidNode();
    defineDataMethod(methodCall_data)
    void methodCall();
    defineDataMethod(eventSubscription_data)
    void eventSubscription();
    defineDataMethod(eventSubscribeInvalidNode_data)
    void eventSubscribeInvalidNode();
    defineDataMethod(readRange_data)
    void readRange();
    defineDataMethod(readEui_data)
    void readEui();
    defineDataMethod(malformedNodeString_data)
    void malformedNodeString();

    void multipleClients();
    defineDataMethod(nodeClass_data)
    void nodeClass();
    defineDataMethod(writeArray_data)
    void writeArray();
    defineDataMethod(readArray_data)
    void readArray();
    defineDataMethod(writeScalar_data)
    void writeScalar();
    defineDataMethod(readScalar_data)
    void readScalar();

    defineDataMethod(stringCharset_data)
    void stringCharset();

private:
    QString envOrDefault(const char *env, QString def)
    {
        return qEnvironmentVariableIsSet(env) ? qgetenv(env).constData() : def;
    }

    QString m_endpoint;
    QOpcUaProvider m_opcUa;
    QStringList m_backends;
    QVector<QOpcUaClient *> m_clients;
    QProcess m_serverProcess;
};

#define READ_MANDATORY_BASE_NODE(NODE) \
    { \
    QSignalSpy readFinishedSpy(NODE.data(), &QOpcUaNode::readFinished);\
    NODE->readAttributes(QOpcUaNode::mandatoryBaseAttributes()); \
    readFinishedSpy.wait(); \
    QCOMPARE(readFinishedSpy.count(), 1); \
    QVERIFY(readFinishedSpy.at(0).at(0).value<QOpcUaNode::NodeAttributes>() == QOpcUaNode::mandatoryBaseAttributes()); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::NodeId)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::NodeClass)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::BrowseName)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::DisplayName)) == true); \
    }

#define READ_MANDATORY_VARIABLE_NODE(NODE) \
    { \
    QSignalSpy readFinishedSpy(NODE.data(), &QOpcUaNode::readFinished);\
    NODE->readAttributes(QOpcUaNode::mandatoryBaseAttributes() | QOpcUaNode::NodeAttribute::Value); \
    readFinishedSpy.wait(); \
    QCOMPARE(readFinishedSpy.count(), 1); \
    QVERIFY(readFinishedSpy.at(0).at(0).value<QOpcUaNode::NodeAttributes>() == (QOpcUaNode::mandatoryBaseAttributes() | QOpcUaNode::NodeAttribute::Value)); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::NodeId)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::NodeClass)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::BrowseName)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::DisplayName)) == true); \
    QVERIFY(QOpcUa::isSuccessStatus(NODE->attributeError(QOpcUaNode::NodeAttribute::Value)) == true); \
    }

#define WRITE_VALUE_ATTRIBUTE(NODE, VALUE, TYPE) \
{ \
    QSignalSpy resultSpy(NODE.data(), &QOpcUaNode::attributeWritten); \
    NODE->writeAttribute(QOpcUaNode::NodeAttribute::Value, VALUE, TYPE); \
    resultSpy.wait(); \
    QCOMPARE(resultSpy.size(), 1); \
    QCOMPARE(resultSpy.at(0).at(0).value<QOpcUaNode::NodeAttribute>(), QOpcUaNode::NodeAttribute::Value); \
    QCOMPARE(resultSpy.at(0).at(1).toUInt(), (uint)0); \
}

Tst_QOpcUaClient::Tst_QOpcUaClient()
{
    qRegisterMetaType<QOpcUaClient::ClientState>("ClientState");

    m_backends = QOpcUaProvider::availableBackends();
}

void Tst_QOpcUaClient::initTestCase()
{
    for (const auto &backend: m_backends) {
        QOpcUaClient *client = m_opcUa.createClient(backend);
        QVERIFY2(client != nullptr,
                 QString("Loading backend failed: %1").arg(backend).toLatin1().data());
        client->setParent(this);
        qDebug() << "Using SDK plugin:" << client->backend();
        m_clients.append(client);
    }

    if (qEnvironmentVariableIsEmpty("OPCUA_HOST") && qEnvironmentVariableIsEmpty("OPCUA_PORT")) {
        const QString testServerPath = QDir::currentPath()
                                     + QLatin1String("/../../open62541-testserver/open62541-testserver")
#ifdef Q_OS_WIN
                                     + QLatin1String(".exe")
#endif
                ;
        if (!QFile::exists(testServerPath)) {
            qDebug() << "Server Path:" << testServerPath;
            QSKIP("all auto tests rely on an open62541-based test-server");
        }

        m_serverProcess.start(testServerPath);
        QVERIFY2(m_serverProcess.waitForStarted(), qPrintable(m_serverProcess.errorString()));
        // Let the server come up
        QTest::qSleep(2000);
    }
    QString host = envOrDefault("OPCUA_HOST", "localhost");
    QString port = envOrDefault("OPCUA_PORT", "43344");
    m_endpoint = QString("opc.tcp://%1:%2").arg(host).arg(port);
    qDebug() << "Using endpoint:" << m_endpoint;
}

void Tst_QOpcUaClient::connectToInvalid()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    opcuaClient->connectToEndpoint(QUrl("opc.tcp:127.0.0.1:1234"));
    QVERIFY(opcuaClient->state() == QOpcUaClient::Connecting);

    for (int i = 0; i < 10; ++i) {
        QTest::qWait(50);
        if (opcuaClient->state() == QOpcUaClient::Disconnected)
            break;
        QVERIFY(opcuaClient->state() == QOpcUaClient::Connecting);
    }
    QVERIFY(opcuaClient->state() == QOpcUaClient::Connected ||
            opcuaClient->state() == QOpcUaClient::Disconnected);

    QUrl url = opcuaClient->url();
    QVERIFY(url == QUrl("opc.tcp:127.0.0.1:1234"));
}

void Tst_QOpcUaClient::secureConnect()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    QSKIP("Secure connections are not supported by open62541-based testserver");
    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("Secure connections are not supported with the freeopcua backend");
    if (opcuaClient->backend() == QLatin1String("open62541"))
        QSKIP("Secure connections are not supported with the open62541 backend");

    QVERIFY(opcuaClient != 0);

    opcuaClient->secureConnectToEndpoint(QUrl(m_endpoint));
    QVERIFY(opcuaClient->state() == QOpcUaClient::Connecting);
    QTRY_VERIFY2(opcuaClient->state() == QOpcUaClient::Connected, "Could not connect to server");

    QVERIFY(opcuaClient->url() == QUrl(m_endpoint));

    opcuaClient->disconnectFromEndpoint();
    QTRY_VERIFY2(opcuaClient->state() == QOpcUaClient::Disconnected, "Could not disconnect from server");
}

void Tst_QOpcUaClient::secureConnectToInvalid()
{
    QFETCH(QOpcUaClient *, opcuaClient);

    QSKIP("Secure connections are not supported by open62541-based testserver");
    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("Secure connections are not supported with the freeopcua backend");
    if (opcuaClient->backend() == QLatin1String("open62541"))
        QSKIP("Secure connections are not supported with the open62541 backend");

    opcuaClient->secureConnectToEndpoint(QUrl("opc.tcp:127.0.0.1:1234"));
    QVERIFY(opcuaClient->state() == QOpcUaClient::Connecting);

    for (int i = 0; i < 10; ++i) {
        QTest::qWait(50);
        if (opcuaClient->state() == QOpcUaClient::Disconnected)
            break;
        QVERIFY(opcuaClient->state() == QOpcUaClient::Connecting);
    }
    QVERIFY(opcuaClient->state() == QOpcUaClient::Connected ||
            opcuaClient->state() == QOpcUaClient::Disconnected);

    QUrl url = opcuaClient->url();
    QVERIFY(url == QUrl("opc.tcp:127.0.0.1:1234"));
}

void Tst_QOpcUaClient::connectAndDisconnect()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);
}

void Tst_QOpcUaClient::connectInvalidPassword()
{
    QFETCH(QOpcUaClient *, opcuaClient);

    QUrl url(m_endpoint);
    url.setUserName("invaliduser");
    url.setPassword("wrongpassword");

    QSignalSpy connectSpy(opcuaClient, &QOpcUaClient::stateChanged);

    opcuaClient->connectToEndpoint(url);
    connectSpy.wait();

    QVERIFY(connectSpy.count() == 2);
    QVERIFY(connectSpy.at(0).at(0) == QOpcUaClient::Connecting);
    QVERIFY(connectSpy.at(1).at(0) == QOpcUaClient::Disconnected);

    QVERIFY(opcuaClient->url() == url);
    QVERIFY(opcuaClient->error() == QOpcUaClient::AccessDenied);
}

void Tst_QOpcUaClient::connectAndDisconnectPassword()
{
    QFETCH(QOpcUaClient *, opcuaClient);

    QUrl url(m_endpoint);
    url.setUserName("user1");
    url.setPassword("password");

    QSignalSpy connectSpy(opcuaClient, &QOpcUaClient::stateChanged);

    opcuaClient->connectToEndpoint(url);
    connectSpy.wait();

    QVERIFY(connectSpy.count() == 2);
    QVERIFY(connectSpy.at(0).at(0) == QOpcUaClient::Connecting);
    QVERIFY(connectSpy.at(1).at(0) == QOpcUaClient::Connected);

    QVERIFY(opcuaClient->url() == url);
    QVERIFY(opcuaClient->error() == QOpcUaClient::NoError);

    connectSpy.clear();
    opcuaClient->disconnectFromEndpoint();
    connectSpy.wait();
    QVERIFY(connectSpy.count() == 2);
    QVERIFY(connectSpy.at(0).at(0) == QOpcUaClient::Closing);
    QVERIFY(connectSpy.at(1).at(0) == QOpcUaClient::Disconnected);
}

void Tst_QOpcUaClient::readInvalidNode()
{
    QFETCH(QOpcUaClient*, opcuaClient);

    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=0;s=doesnotexist"));
    QVERIFY(node != 0);
    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::DisplayName).value<QOpcUa::QLocalizedText>().text, QString());

    QSignalSpy readFinishedSpy(node.data(), &QOpcUaNode::readFinished);

    node->readAttributes(QOpcUaNode::mandatoryBaseAttributes());
    readFinishedSpy.wait();

    QCOMPARE(readFinishedSpy.count(), 1);
    QCOMPARE(readFinishedSpy.at(0).at(0).value<QOpcUaNode::NodeAttributes>(), QOpcUaNode::mandatoryBaseAttributes());

    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::DisplayName), QVariant());
    QCOMPARE(node->attributeError(QOpcUaNode::NodeAttribute::DisplayName), QOpcUa::UaStatusCode::BadNodeIdUnknown);
    QVERIFY(QOpcUa::errorCategory(node->attributeError(QOpcUaNode::NodeAttribute::DisplayName)) == QOpcUa::ErrorCategory::NodeError);
    QVERIFY(QOpcUa::isSuccessStatus(node->attributeError(QOpcUaNode::NodeAttribute::DisplayName)) == false);
}

void Tst_QOpcUaClient::writeInvalidNode()
{
    QFETCH(QOpcUaClient*, opcuaClient);

    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=0;s=doesnotexist"));
    QVERIFY(node != 0);

    QSignalSpy responseSpy(node.data(),&QOpcUaNode::attributeWritten);
    bool result = node->writeAttribute(QOpcUaNode::NodeAttribute::Value, 10, QOpcUa::Types::Int32);

    QCOMPARE(result, true);

    responseSpy.wait();

    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(responseSpy.at(0).at(0).value<QOpcUaNode::NodeAttribute>(), QOpcUaNode::NodeAttribute::Value);
    QCOMPARE(responseSpy.at(0).at(1).value<quint32>(), QOpcUa::UaStatusCode::BadNodeIdUnknown);
}

void Tst_QOpcUaClient::writeMultipleAttributes()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node(readWriteNode));
    QVERIFY(node != 0);

    WRITE_VALUE_ATTRIBUTE(node, QVariant(double(0)), QOpcUa::Types::Double);

    QOpcUaNode::AttributeMap map;
    map[QOpcUaNode::NodeAttribute::DisplayName] = QVariant(QLatin1String("NewDisplayName"));
    map[QOpcUaNode::NodeAttribute::Value] = QVariant(double(23.5));

    QSignalSpy writeSpy(node.data(), &QOpcUaNode::attributeWritten);

    node->writeAttributes(map);

    writeSpy.wait();
    if (writeSpy.size() < 2)
        writeSpy.wait();

    QVERIFY(writeSpy.size() == 2);
    QVERIFY(writeSpy.at(0).at(0).value<QOpcUaNode::NodeAttribute>() == QOpcUaNode::NodeAttribute::DisplayName);
    QVERIFY(writeSpy.at(0).at(1).value<QOpcUa::UaStatusCode>() == QOpcUa::UaStatusCode::BadUserAccessDenied);
    QVERIFY(node->attributeError(QOpcUaNode::NodeAttribute::DisplayName) == QOpcUa::UaStatusCode::BadUserAccessDenied);
    QVERIFY(QOpcUa::errorCategory(node->attributeError(QOpcUaNode::NodeAttribute::DisplayName)) == QOpcUa::ErrorCategory::PermissionError);

    QVERIFY(writeSpy.at(1).at(0).value<QOpcUaNode::NodeAttribute>() == QOpcUaNode::NodeAttribute::Value);
    QVERIFY(writeSpy.at(1).at(1).value<QOpcUa::UaStatusCode>() == QOpcUa::UaStatusCode::Good);
    QVERIFY(node->attributeError(QOpcUaNode::NodeAttribute::Value) == QOpcUa::UaStatusCode::Good);
    QVERIFY(node->attribute(QOpcUaNode::NodeAttribute::Value) == double(23.5));
}

void Tst_QOpcUaClient::getRootNode()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> root(opcuaClient->node("ns=0;i=84"));
    QVERIFY(root != 0);

    READ_MANDATORY_BASE_NODE(root)
    QVERIFY(root->attribute(QOpcUaNode::NodeAttribute::DisplayName).value<QOpcUa::QLocalizedText>().text == QLatin1String("Root"));

    QString nodeId = root->nodeId();
    QCOMPARE(nodeId, QStringLiteral("ns=0;i=84"));
}

void Tst_QOpcUaClient::getChildren()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=1;s=Large.Folder"));
    QVERIFY(node != 0);
    READ_MANDATORY_BASE_NODE(node)
    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::DisplayName).value<QOpcUa::QLocalizedText>().text, QLatin1String("Large_Folder"));
    QCOMPARE(node->childrenIds().size(), 1001);
}

void Tst_QOpcUaClient::childrenIdsString()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=testStringIdsFolder"));
    QVERIFY(node != 0);
    QStringList childrenIds = node->childrenIds();
    QCOMPARE(childrenIds.size(), 1);
    QCOMPARE(childrenIds.at(0), "ns=3;s=theStringId");
}

void Tst_QOpcUaClient::childrenIdsGuidNodeId()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=testGuidIdsFolder"));
    QVERIFY(node != 0);
    const QStringList childrenIds = node->childrenIds();
    QCOMPARE(childrenIds.size(), 1);
    QCOMPARE(childrenIds.at(0), "ns=3;g=08081e75-8e5e-319b-954f-f3a7613dc29b");
}

void Tst_QOpcUaClient::childrenIdsOpaqueNodeId()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=testOpaqueIdsFolder"));
    QVERIFY(node != 0);
    const QStringList childrenIds = node->childrenIds();
    QCOMPARE(childrenIds.size(), 1);
    QCOMPARE(childrenIds.at(0), "ns=3;b=UXQgZnR3IQ==");
}

void Tst_QOpcUaClient::dataChangeSubscription()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node(readWriteNode));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, QVariant(double(0)), QOpcUa::Types::Double);
    READ_MANDATORY_VARIABLE_NODE(node);
    QTRY_COMPARE(node->attribute(QOpcUaNode::NodeAttribute::Value), 0);

    QScopedPointer<QOpcUaSubscription> subscription(opcuaClient->createSubscription(100));
    QScopedPointer<QOpcUaMonitoredValue> monitoredValue(subscription->addValue(node.data()));
    QVERIFY(monitoredValue != nullptr);
    if (!monitoredValue)
        QFAIL("can not monitor value");

    QSignalSpy valueSpy(monitoredValue.data(), &QOpcUaMonitoredValue::valueChanged);

    WRITE_VALUE_ATTRIBUTE(node, QVariant(double(42)), QOpcUa::Types::Double);

    valueSpy.wait();
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), double(42));
}

void Tst_QOpcUaClient::dataChangeSubscriptionInvalidNode()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> noDataNode(opcuaClient->node("ns=0;i=84"));
    QVERIFY(noDataNode != 0);
    QScopedPointer<QOpcUaSubscription> subscription(opcuaClient->createSubscription(100));
    QOpcUaMonitoredValue *result = subscription->addValue(noDataNode.data());
    QVERIFY(result == 0);
}

void Tst_QOpcUaClient::methodCall()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QSKIP("Method calls are not implemented in open62541-based testserver");
    QVector<QOpcUa::TypedVariant> args;
    QVector<QVariant> ret;
    for (int i = 0; i < 2; i++)
        args.push_back(QOpcUa::TypedVariant(double(4), QOpcUa::Double));

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=TestFolder"));
    QVERIFY(node != 0);

    bool success = node->call("ns=0;s=IDoNotExist", &args, &ret);
    QVERIFY(success == false);

    success = node->call("ns=3;s=Test.Method.Multiply", &args, &ret);
    QVERIFY(success == true);
    QVERIFY(ret.size() == 1);
    QVERIFY(ret[0].type() == QVariant::Double && ret[0].value<double>() == 16);
}

void Tst_QOpcUaClient::eventSubscription()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QSKIP("Events are not implemented in the open62541-based testserver");
    if (opcuaClient->backend() == QLatin1String("freeopcua")) {
        QSKIP("Event subscriptions do not yet work with the freeopcua backend");
    }
    if (opcuaClient->backend() == QLatin1String("open62541")) {
        QSKIP("Event subscriptions do not yet work with the open62541 backend");
    }

    QScopedPointer<QOpcUaNode> triggerNode(opcuaClient->node("ns=3;s=TriggerNode"));
    QVERIFY(triggerNode != 0);

    QScopedPointer<QOpcUaSubscription> subscription(opcuaClient->createSubscription(100));
    QOpcUaMonitoredEvent *monitoredEvent = subscription->addEvent(triggerNode.data());
    QVERIFY(monitoredEvent != 0);

    if (!monitoredEvent)
        QFAIL("can not monitor event");

    QSignalSpy monitorSpy(monitoredEvent, &QOpcUaMonitoredEvent::newEvent);

    QScopedPointer<QOpcUaNode> triggerVariable(opcuaClient->node("ns=3;s=TriggerVariable"));
    QVERIFY(triggerVariable != 0);
    WRITE_VALUE_ATTRIBUTE(triggerVariable, QVariant(double(0)), QOpcUa::Types::Double);
    WRITE_VALUE_ATTRIBUTE(triggerVariable, QVariant(double(1)), QOpcUa::Types::Double);

    QVERIFY(monitorSpy.wait());
    QVector<QVariant> val = monitorSpy.at(0).at(0).toList().toVector();
    QCOMPARE(val.size(), 3);
    QCOMPARE(val.at(0).type(), QVariant::String);
    QCOMPARE(val.at(1).type(), QVariant::String);
    QCOMPARE(val.at(2).type(), QVariant::Int);

    delete monitoredEvent;
}

void Tst_QOpcUaClient::eventSubscribeInvalidNode()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> noEventNode(opcuaClient->node(readWriteNode));
    QVERIFY(noEventNode != 0);
    QScopedPointer<QOpcUaSubscription> subscription(opcuaClient->createSubscription(100));
    QOpcUaMonitoredEvent *monitoredEvent = subscription->addEvent(noEventNode.data());
    QVERIFY(monitoredEvent == 0);
}

void Tst_QOpcUaClient::readRange()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QSKIP("No ranges supported in open62541-based testserver");
    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("Ranges are not yet implemented in the freeopcua backend");
    if (opcuaClient->backend() == QLatin1String("open62541"))
        QSKIP("Ranges are not yet implemented in the open62541 backend");


    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=ACControl.CurrentTemp.EURange"));
    QVERIFY(node != 0);
    QPair<double, double> range = node->readEuRange();
    QCOMPARE(range.first, 0);
    QCOMPARE(range.second, 100);
}

void Tst_QOpcUaClient::readEui()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QSKIP("No engineering unit information supported in open62541-based testserver");
    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("Engineering unit information are not yet implemented in the freeopcua backend");
    if (opcuaClient->backend() == QLatin1String("open62541"))
        QSKIP("Engineering unit information are not yet implemented in the open62541 backend");

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=3;s=ACControl.CurrentTemp.EngineeringUnits"));
    QVERIFY(node != 0);

    QPair<QString, QString> range = node->readEui();
    QVERIFY(range.first == QString::fromUtf8("°C") && range.second == "Degree fahrenheit");
}

void Tst_QOpcUaClient::malformedNodeString()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> invalidNode(opcuaClient->node("justsomerandomstring"));
    QVERIFY(invalidNode == 0);

    invalidNode.reset(opcuaClient->node("ns=a;i=b"));
    QVERIFY(invalidNode == 0);

    invalidNode.reset(opcuaClient->node("ns=;i="));
    QVERIFY(invalidNode == 0);

    invalidNode.reset(opcuaClient->node("ns=0;x=123"));
    QVERIFY(invalidNode == 0);

    invalidNode.reset(opcuaClient->node("ns=0,i=31;"));
    QVERIFY(invalidNode == 0);

    invalidNode.reset(opcuaClient->node("ns:0;i:31;"));
    QVERIFY(invalidNode == 0);
}

void Tst_QOpcUaClient::multipleClients()
{
    QScopedPointer<QOpcUaClient> a(m_opcUa.createClient(m_backends[0]));
    a->connectToEndpoint(m_endpoint);
    QTRY_VERIFY2(a->state() == QOpcUaClient::Connected, "Could not connect to server");
    QScopedPointer<QOpcUaNode> node(a->node(readWriteNode));
    READ_MANDATORY_VARIABLE_NODE(node);
    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::Value).toDouble(), 42.0);
    QScopedPointer<QOpcUaClient> b(m_opcUa.createClient(m_backends[0]));
    b->connectToEndpoint(m_endpoint);
    QTRY_VERIFY2(b->state() == QOpcUaClient::Connected, "Could not connect to server");
    node.reset(b->node(readWriteNode));
    READ_MANDATORY_VARIABLE_NODE(node);
    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::Value).toDouble(), 42.0);
    QScopedPointer<QOpcUaClient> d(m_opcUa.createClient(m_backends[0]));
    d->connectToEndpoint(m_endpoint);
    QTRY_VERIFY2(d->state() == QOpcUaClient::Connected, "Could not connect to server");
    node.reset(d->node(readWriteNode));
    READ_MANDATORY_VARIABLE_NODE(node);
    QCOMPARE(node->attribute(QOpcUaNode::NodeAttribute::Value).toDouble(), 42.0);
    d->disconnectFromEndpoint();
    QTRY_VERIFY2(d->state() == QOpcUaClient::Disconnected, "Could not disconnect from server");
    a->disconnectFromEndpoint();
    QTRY_VERIFY2(a->state() == QOpcUaClient::Disconnected, "Could not disconnect from server");
    b->disconnectFromEndpoint();
    QTRY_VERIFY2(b->state() == QOpcUaClient::Disconnected, "Could not disconnect from server");
}

void Tst_QOpcUaClient::nodeClass()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    // Root -> Types -> ReferenceTypes -> References
    {
        QScopedPointer<QOpcUaNode> refNode(opcuaClient->node("ns=0;i=31"));
        QVERIFY(refNode != 0);
        READ_MANDATORY_BASE_NODE(refNode)
        QCOMPARE(refNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::ReferenceType);
    }

    // Root -> Types -> DataTypes -> BaseDataTypes -> Boolean
    {
        QScopedPointer<QOpcUaNode> dataTypeNode(opcuaClient->node("ns=0;i=1"));
        QVERIFY(dataTypeNode != 0);
        READ_MANDATORY_BASE_NODE(dataTypeNode)
        QCOMPARE(dataTypeNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::DataType);
    }

    // Root -> Types -> DataTypes -> ObjectTypes -> BaseObjectTypes -> FolderType
    {
        QScopedPointer<QOpcUaNode> objectTypeNode(opcuaClient->node("ns=0;i=61"));
        QVERIFY(objectTypeNode != 0);
        READ_MANDATORY_BASE_NODE(objectTypeNode)
        QCOMPARE(objectTypeNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::ObjectType);
    }

    // Root -> Types -> DataTypes -> VariableTypes -> BaseVariableType -> PropertyType
    {
        QScopedPointer<QOpcUaNode> variableTypeNode(opcuaClient->node("ns=0;i=68"));
        QVERIFY(variableTypeNode != 0);
        READ_MANDATORY_BASE_NODE(variableTypeNode)
        QCOMPARE(variableTypeNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::VariableType);
    }

    // Root -> Objects
    {
        QScopedPointer<QOpcUaNode> objectNode(opcuaClient->node("ns=0;i=85"));
        QVERIFY(objectNode != 0);
        READ_MANDATORY_BASE_NODE(objectNode)
        QCOMPARE(objectNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::Object);
    }

    // Root -> Objects -> Server -> NamespaceArray
    {
        QScopedPointer<QOpcUaNode> variableNode(opcuaClient->node("ns=0;i=2255"));
        QVERIFY(variableNode != 0);
        READ_MANDATORY_BASE_NODE(variableNode)
        QCOMPARE(variableNode->attribute(QOpcUaNode::NodeAttribute::NodeClass).value<QOpcUaNode::NodeClass>(), QOpcUaNode::NodeClass::Variable);
    }
}

void Tst_QOpcUaClient::writeArray()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QVariantList list;

    list.append(true);
    list.append(false);
    list.append(true);
    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Boolean"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Boolean);

    list.clear();
    list.append(std::numeric_limits<quint8>::min());
    list.append(std::numeric_limits<quint8>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Byte"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Byte);

    list.clear();
    list.append(QDateTime::currentDateTime());
    list.append(QDateTime::currentDateTime());
    list.append(QDateTime::currentDateTime());
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.DateTime"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::DateTime);

    list.clear();
    list.append(23.5);
    list.append(23.6);
    list.append(23.7);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Double"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Double);

    list.clear();
    list.append(23.5);
    list.append(23.6);
    list.append(23.7);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Float"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Float);

    list.clear();
    list.append(std::numeric_limits<qint16>::min());
    list.append(std::numeric_limits<qint16>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int16"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Int16);

    list.clear();
    list.append(std::numeric_limits<qint32>::min());
    list.append(std::numeric_limits<qint32>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int32"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Int32);

    list.clear();
    list.append(std::numeric_limits<qint64>::min());
    list.append(std::numeric_limits<qint64>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int64"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Int64);

    list.clear();
    list.append(std::numeric_limits<qint8>::min());
    list.append(std::numeric_limits<qint8>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.SByte"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::SByte);

    list.clear();
    list.append("Test1");
    list.append("Test2");
    list.append("Test3");
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.String"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::String);

    list.clear();
    list.append(std::numeric_limits<quint16>::min());
    list.append(std::numeric_limits<quint16>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt16"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::UInt16);

    list.clear();
    list.append(std::numeric_limits<quint32>::min());
    list.append(std::numeric_limits<quint32>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt32"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::UInt32);

    list.clear();
    list.append(std::numeric_limits<quint64>::min());
    list.append(std::numeric_limits<quint64>::max());
    list.append(10);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt64"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::UInt64);

    list.clear();
    list.append(QVariant::fromValue(localizedTexts[0]));
    list.append(QVariant::fromValue(localizedTexts[1]));
    list.append(QVariant::fromValue(localizedTexts[2]));
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.LocalizedText"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::LocalizedText);

    list.clear();
    list.append("abc");
    list.append("def");
    QByteArray withNull("gh");
    withNull.append('\0');
    withNull.append("i");
    list.append(withNull);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.ByteString"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::ByteString);

    list.clear();
    list.append(QUuid("e0bd5ccd-f571-4545-9352-61a0f8cb9216"));
    list.append(QUuid("460ebe04-89d8-42f3-a0e0-7b45940f1a4e4"));
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Guid"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Guid);

    list.clear();
    list.append("ns=0;i=0");
    list.append("ns=0;i=1");
    list.append("ns=0;i=2");
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.NodeId"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::NodeId);

    list.clear();
    list.append(QVariant::fromValue(QOpcUa::QQualifiedName(0, "Test0")));
    list.append(QVariant::fromValue(QOpcUa::QQualifiedName(1, "Test1")));
    list.append(QVariant::fromValue(QOpcUa::QQualifiedName(2, "Test2")));
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.QualifiedName"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::QualifiedName);

    list.clear();
    list.append(QOpcUa::UaStatusCode::Good);
    list.append(QOpcUa::UaStatusCode::BadUnexpectedError);
    list.append(QOpcUa::UaStatusCode::BadInternalError);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.StatusCode"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::Types::StatusCode);

    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("XmlElement support is not yet implemented in the freeopcua library");

    list.clear();
    list.append(xmlElements[0]);
    list.append(xmlElements[1]);
    list.append(xmlElements[2]);
    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Arrays.XmlElement"));
    QVERIFY(node != 0);
    WRITE_VALUE_ATTRIBUTE(node, list, QOpcUa::XmlElement);
}

void Tst_QOpcUaClient::readArray()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> booleanArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Boolean"));
    QVERIFY(booleanArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(booleanArrayNode);
    QVariant booleanArray = booleanArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(booleanArray.type() == QVariant::List);
    QVERIFY(booleanArray.toList().length() == 3);
    QCOMPARE(booleanArray.toList()[0].type(), QVariant::Bool);
    QCOMPARE(booleanArray.toList()[0].toBool(), true);
    QCOMPARE(booleanArray.toList()[1].toBool(), false);
    QCOMPARE(booleanArray.toList()[2].toBool(), true);

    QScopedPointer<QOpcUaNode> int32ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int32"));
    QVERIFY(int32ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(int32ArrayNode);
    QVariant int32Array = int32ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int32Array.type() == QVariant::List);
    QVERIFY(int32Array.toList().length() == 3);
    QCOMPARE(int32Array.toList()[0].type(), QVariant::Int);
    QCOMPARE(int32Array.toList()[0].toInt(), std::numeric_limits<qint32>::min());
    QCOMPARE(int32Array.toList()[1].toInt(), std::numeric_limits<qint32>::max());
    QCOMPARE(int32Array.toList()[2].toInt(), 10);

    QScopedPointer<QOpcUaNode> uint32ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt32"));
    QVERIFY(uint32ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(uint32ArrayNode);
    QVariant uint32Array = uint32ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(uint32Array.type() == QVariant::List);
    QVERIFY(uint32Array.toList().length() == 3);
    QCOMPARE(uint32Array.toList()[0].type(), QVariant::UInt);
    QCOMPARE(uint32Array.toList()[0].toUInt(), std::numeric_limits<quint32>::min());
    QCOMPARE(uint32Array.toList()[1].toUInt(), std::numeric_limits<quint32>::max());
    QCOMPARE(uint32Array.toList()[2].toUInt(), quint32(10));

    QScopedPointer<QOpcUaNode> doubleArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Double"));
    QVERIFY(doubleArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(doubleArrayNode);
    QVariant doubleArray = doubleArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(doubleArray.type() == QVariant::List);
    QVERIFY(doubleArray.toList().length() == 3);
    QCOMPARE(doubleArray.toList()[0].type(), QVariant::Double);
    QCOMPARE(doubleArray.toList()[0].toDouble(), double(23.5));
    QCOMPARE(doubleArray.toList()[1].toDouble(), double(23.6));
    QCOMPARE(doubleArray.toList()[2].toDouble(), double(23.7));

    QScopedPointer<QOpcUaNode> floatArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Float"));
    QVERIFY(floatArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(floatArrayNode);
    QVariant floatArray = floatArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(floatArray.type() == QVariant::List);
    QVERIFY(floatArray.toList().length() == 3);
    QVERIFY(floatArray.toList()[0].userType() == QMetaType::Float);
    QCOMPARE(floatArray.toList()[0].toFloat(), float(23.5));
    QCOMPARE(floatArray.toList()[1].toFloat(), float(23.6));
    QCOMPARE(floatArray.toList()[2].toFloat(), float(23.7));

    QScopedPointer<QOpcUaNode> stringArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.String"));
    QVERIFY(stringArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(stringArrayNode);
    QVariant stringArray = stringArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(stringArray.type() == QVariant::List);
    QVERIFY(stringArray.toList().length() == 3);
    QCOMPARE(stringArray.toList()[0].type(), QVariant::String);
    QCOMPARE(stringArray.toList()[0].toString(), QStringLiteral("Test1"));
    QCOMPARE(stringArray.toList()[1].toString(), QStringLiteral("Test2"));
    QCOMPARE(stringArray.toList()[2].toString(), QStringLiteral("Test3"));

    QScopedPointer<QOpcUaNode> dateTimeArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.DateTime"));
    QVERIFY(dateTimeArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(dateTimeArrayNode);
    QVariant dateTimeArray = dateTimeArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(dateTimeArray.type() == QVariant::List);
    QVERIFY(dateTimeArray.toList().length() == 3);
    QCOMPARE(dateTimeArray.toList()[0].type(), QVariant::DateTime);

    QScopedPointer<QOpcUaNode> ltArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.LocalizedText"));
    QVERIFY(ltArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(ltArrayNode);
    QVariant ltArray = ltArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(ltArray.type() == QVariant::List);
    QVERIFY(ltArray.toList().length() == 3);
    QVERIFY(ltArray.toList()[0].value<QOpcUa::QLocalizedText>() == localizedTexts[0]);
    QVERIFY(ltArray.toList()[1].value<QOpcUa::QLocalizedText>() == localizedTexts[1]);
    QVERIFY(ltArray.toList()[2].value<QOpcUa::QLocalizedText>() == localizedTexts[2]);

    QScopedPointer<QOpcUaNode> uint16ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt16"));
    QVERIFY(uint16ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(uint16ArrayNode);
    QVariant uint16Array = uint16ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(uint16Array.type() == QVariant::List);
    QVERIFY(uint16Array.toList().length() == 3);
    QVERIFY(uint16Array.toList()[0].userType() == QMetaType::UShort);
    QVERIFY(uint16Array.toList()[0] == std::numeric_limits<quint16>::min());
    QVERIFY(uint16Array.toList()[1] == std::numeric_limits<quint16>::max());
    QVERIFY(uint16Array.toList()[2] == quint16(10));

    QScopedPointer<QOpcUaNode> int16ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int16"));
    QVERIFY(int16ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(int16ArrayNode);
    QVariant int16Array = int16ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int16Array.type() == QVariant::List);
    QVERIFY(int16Array.toList().length() == 3);
    QVERIFY(int16Array.toList()[0].userType() == QMetaType::Short);
    QVERIFY(int16Array.toList()[0] == std::numeric_limits<qint16>::min());
    QVERIFY(int16Array.toList()[1] == std::numeric_limits<qint16>::max());
    QVERIFY(int16Array.toList()[2] == qint16(10));

    QScopedPointer<QOpcUaNode> uint64ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.UInt64"));
    QVERIFY(uint64ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(uint64ArrayNode);
    QVariant uint64Array = uint64ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QCOMPARE(uint64Array.type(), QVariant::List);
    QCOMPARE(uint64Array.toList().length(), 3);
    QCOMPARE(uint64Array.toList()[0].type(), QVariant::ULongLong);
    QVERIFY(uint64Array.toList()[0] == std::numeric_limits<quint64>::min());
    QVERIFY(uint64Array.toList()[1] == std::numeric_limits<quint64>::max());
    QVERIFY(uint64Array.toList()[2] == quint64(10));

    QScopedPointer<QOpcUaNode> int64ArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Int64"));
    QVERIFY(int64ArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(int64ArrayNode);
    QVariant int64Array = int64ArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int64Array.type() == QVariant::List && int64Array.toList().length() == 3);
    QCOMPARE(int64Array.toList()[0].type(), QVariant::LongLong);
    QVERIFY(int64Array.toList()[0] == std::numeric_limits<qint64>::min());
    QVERIFY(int64Array.toList()[1] == std::numeric_limits<qint64>::max());
    QVERIFY(int64Array.toList()[2] == qint64(10));

    QScopedPointer<QOpcUaNode> byteArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Byte"));
    QVERIFY(byteArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(byteArrayNode);
    QVariant byteArray = byteArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(byteArray.type() == QVariant::List);
    QVERIFY(byteArray.toList().length() == 3);
    QVERIFY(byteArray.toList()[0].userType() == QMetaType::UChar);
    QVERIFY(byteArray.toList()[0] == std::numeric_limits<quint8>::min());
    QVERIFY(byteArray.toList()[1] == std::numeric_limits<quint8>::max());
    QVERIFY(byteArray.toList()[2] == quint8(10));

    QScopedPointer<QOpcUaNode> byteStringArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.ByteString"));
    QVERIFY(byteStringArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(byteStringArrayNode);
    QVariant byteStringArray = byteStringArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(byteStringArray.type() == QVariant::List);
    QVERIFY(byteStringArray.toList().length() == 3);
    QVERIFY(byteStringArray.toList()[0].userType() == QMetaType::QByteArray);
    QVERIFY(byteStringArray.toList()[0] == "abc");
    QVERIFY(byteStringArray.toList()[1] == "def");
    QByteArray withNull("gh");
    withNull.append('\0');
    withNull.append("i");
    QVERIFY(byteStringArray.toList()[2] == withNull);

    QScopedPointer<QOpcUaNode> guidArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.Guid"));
    QVERIFY(guidArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(guidArrayNode);
    QVariant guidArray = guidArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(guidArray.type() == QVariant::List);
    QVERIFY(guidArray.toList().length() == 2);
    QCOMPARE(guidArray.toList()[0], QUuid("e0bd5ccd-f571-4545-9352-61a0f8cb9216}"));
    QCOMPARE(guidArray.toList()[1], QUuid("460ebe04-89d8-42f3-a0e0-7b45940f1a4e4"));

    QScopedPointer<QOpcUaNode> sbyteArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.SByte"));
    QVERIFY(sbyteArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(sbyteArrayNode);
    QVariant sbyteArray = sbyteArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(sbyteArray.type() == QVariant::List);
    QVERIFY(sbyteArray.toList().length() == 3);
    QVERIFY(sbyteArray.toList()[0].userType() == QMetaType::SChar);
    QVERIFY(sbyteArray.toList()[0] == std::numeric_limits<qint8>::min());
    QVERIFY(sbyteArray.toList()[1] == std::numeric_limits<qint8>::max());
    QVERIFY(sbyteArray.toList()[2] == qint8(10));

    QScopedPointer<QOpcUaNode> nodeIdArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.NodeId"));
    QVERIFY(nodeIdArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(nodeIdArrayNode);
    QVariant nodeIdArray = nodeIdArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(nodeIdArray.type() == QVariant::List);
    QVERIFY(nodeIdArray.toList().length() == 3);
    QCOMPARE(nodeIdArray.toList()[0].type(), QVariant::String);
    QCOMPARE(nodeIdArray.toList()[0].toString(), QStringLiteral("ns=0;i=0"));
    QCOMPARE(nodeIdArray.toList()[1].toString(), QStringLiteral("ns=0;i=1"));
    QCOMPARE(nodeIdArray.toList()[2].toString(), QStringLiteral("ns=0;i=2"));

    QScopedPointer<QOpcUaNode> qualifiedNameArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.QualifiedName"));
    QVERIFY(nodeIdArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(qualifiedNameArrayNode)
    QVariant qualifiedNameArray = qualifiedNameArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(qualifiedNameArray.type() == QVariant::List);
    QVERIFY(qualifiedNameArray.toList().length() == 3);
    QVERIFY(qualifiedNameArray.toList()[0].value<QOpcUa::QQualifiedName>()  == QOpcUa::QQualifiedName(0, "Test0"));
    QVERIFY(qualifiedNameArray.toList()[1].value<QOpcUa::QQualifiedName>()  == QOpcUa::QQualifiedName(1, "Test1"));
    QVERIFY(qualifiedNameArray.toList()[2].value<QOpcUa::QQualifiedName>()  == QOpcUa::QQualifiedName(2, "Test2"));

    QScopedPointer<QOpcUaNode> statusCodeArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.StatusCode"));
    QVERIFY(statusCodeArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(statusCodeArrayNode);
    QVariant statusCodeArray = statusCodeArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(statusCodeArray.type() == QVariant::List);
    QVERIFY(statusCodeArray.toList().length() == 3);
    QCOMPARE(statusCodeArray.toList()[0].value<QOpcUa::UaStatusCode>(), QOpcUa::UaStatusCode::Good);
    QCOMPARE(statusCodeArray.toList()[1].value<QOpcUa::UaStatusCode>(), QOpcUa::UaStatusCode::BadUnexpectedError);
    QCOMPARE(statusCodeArray.toList()[2].value<QOpcUa::UaStatusCode>(), QOpcUa::UaStatusCode::BadInternalError);

    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("XmlElement support is not yet implemented in the freeopcua backend");

    QScopedPointer<QOpcUaNode> xmlElementArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.XmlElement"));
    QVERIFY(nodeIdArrayNode != 0);
    READ_MANDATORY_VARIABLE_NODE(xmlElementArrayNode)
    QVariant xmlElementArray = xmlElementArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(xmlElementArray.type() == QVariant::List);
    QVERIFY(xmlElementArray.toList().length() == 3);
    QCOMPARE(xmlElementArray.toList()[0].type(), QVariant::String);
    QCOMPARE(xmlElementArray.toList()[0].toString(), xmlElements[0]);
    QCOMPARE(xmlElementArray.toList()[1].toString(), xmlElements[1]);
    QCOMPARE(xmlElementArray.toList()[2].toString(), xmlElements[2]);
}

void Tst_QOpcUaClient::writeScalar()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> booleanNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Boolean"));
    QVERIFY(booleanNode != 0);
    WRITE_VALUE_ATTRIBUTE(booleanNode, true, QOpcUa::Types::Boolean);

    QScopedPointer<QOpcUaNode> int32Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int32"));
    QVERIFY(int32Node != 0);
    WRITE_VALUE_ATTRIBUTE(int32Node, std::numeric_limits<qint32>::min(), QOpcUa::Types::Int32);

    QScopedPointer<QOpcUaNode> uint32Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt32"));
    QVERIFY(uint32Node != 0);
    WRITE_VALUE_ATTRIBUTE(uint32Node, std::numeric_limits<quint32>::max(), QOpcUa::UInt32);

    QScopedPointer<QOpcUaNode> doubleNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Double"));
    QVERIFY(doubleNode != 0);
    WRITE_VALUE_ATTRIBUTE(doubleNode, 42, QOpcUa::Double);

    QScopedPointer<QOpcUaNode> floatNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Float"));
    QVERIFY(floatNode != 0);
    WRITE_VALUE_ATTRIBUTE(floatNode, 42, QOpcUa::Float);

    QScopedPointer<QOpcUaNode> stringNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.String"));
    QVERIFY(stringNode != 0);
    WRITE_VALUE_ATTRIBUTE(stringNode, "QOpcUa Teststring", QOpcUa::String);

    QScopedPointer<QOpcUaNode> dtNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.DateTime"));
    QVERIFY(dtNode != 0);
    WRITE_VALUE_ATTRIBUTE(dtNode, QDateTime::currentDateTime(), QOpcUa::DateTime);

    QScopedPointer<QOpcUaNode> uint16Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt16"));
    QVERIFY(uint16Node != 0);
    WRITE_VALUE_ATTRIBUTE(uint16Node, std::numeric_limits<quint16>::max(), QOpcUa::UInt16);

    QScopedPointer<QOpcUaNode> int16Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int16"));
    QVERIFY(int16Node != 0);
    WRITE_VALUE_ATTRIBUTE(int16Node, std::numeric_limits<qint16>::min(), QOpcUa::Int16);

    QScopedPointer<QOpcUaNode> uint64Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt64"));
    QVERIFY(uint64Node != 0);
    WRITE_VALUE_ATTRIBUTE(uint64Node, std::numeric_limits<quint64>::max(), QOpcUa::UInt64);

    QScopedPointer<QOpcUaNode> int64Node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int64"));
    QVERIFY(int64Node != 0);
    WRITE_VALUE_ATTRIBUTE(int64Node, std::numeric_limits<qint64>::min(), QOpcUa::Int64);

    QScopedPointer<QOpcUaNode> byteNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Byte"));
    QVERIFY(byteNode != 0);
    WRITE_VALUE_ATTRIBUTE(byteNode, std::numeric_limits<quint8>::max(), QOpcUa::Byte);

    QScopedPointer<QOpcUaNode> sbyteNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.SByte"));
    QVERIFY(sbyteNode != 0);
    WRITE_VALUE_ATTRIBUTE(sbyteNode, std::numeric_limits<qint8>::min(), QOpcUa::SByte);

    QScopedPointer<QOpcUaNode> ltNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.LocalizedText"));
    QVERIFY(ltNode != 0);
    WRITE_VALUE_ATTRIBUTE(ltNode, QVariant::fromValue(localizedTexts[0]), QOpcUa::LocalizedText);

    QByteArray withNull("gh");
    withNull.append('\0');
    withNull.append("i");

    QVariant data = withNull;

    QScopedPointer<QOpcUaNode> byteStringNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.ByteString"));
    QVERIFY(byteStringNode != 0);
    WRITE_VALUE_ATTRIBUTE(byteStringNode, data, QOpcUa::ByteString);

    QScopedPointer<QOpcUaNode> guidNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Guid"));
    QVERIFY(guidNode != 0);
    data = QUuid("e0bd5ccd-f571-4545-9352-61a0f8cb9216");
    WRITE_VALUE_ATTRIBUTE(guidNode, data, QOpcUa::Guid);

    QScopedPointer<QOpcUaNode> nodeIdNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.NodeId"));
    QVERIFY(nodeIdNode != 0);
    WRITE_VALUE_ATTRIBUTE(nodeIdNode, "ns=42;s=Test", QOpcUa::NodeId);

    QScopedPointer<QOpcUaNode> qualifiedNameNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.QualifiedName"));
    QVERIFY(qualifiedNameNode != 0);
    WRITE_VALUE_ATTRIBUTE(qualifiedNameNode, QVariant::fromValue(QOpcUa::QQualifiedName(0, QLatin1String("Test0"))), QOpcUa::QualifiedName);

    QScopedPointer<QOpcUaNode> statusCodeNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.StatusCode"));
    QVERIFY(statusCodeNode != 0);
    WRITE_VALUE_ATTRIBUTE(statusCodeNode, QOpcUa::UaStatusCode::BadInternalError, QOpcUa::StatusCode);

    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("XmlElement support is not yet implemented in the freeopcua backend");

    QScopedPointer<QOpcUaNode> xmlElementNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.XmlElement"));
    QVERIFY(xmlElementNode != 0);
    WRITE_VALUE_ATTRIBUTE(xmlElementNode, xmlElements[0], QOpcUa::XmlElement);
}

void Tst_QOpcUaClient::readScalar()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> node(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Boolean"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant booleanScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(booleanScalar.isValid());
    QCOMPARE(booleanScalar.type(), QVariant::Bool);
    QCOMPARE(booleanScalar.toBool(), true);

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int32"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant int32Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int32Scalar.isValid());
    QCOMPARE(int32Scalar.type(), QVariant::Int);
    QCOMPARE(int32Scalar.toInt(), std::numeric_limits<qint32>::min());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt32"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant uint32Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(uint32Scalar.isValid());
    QCOMPARE(uint32Scalar.type(), QVariant::UInt);
    QCOMPARE(uint32Scalar.toUInt(), std::numeric_limits<quint32>::max());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Double"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant doubleScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(doubleScalar.isValid());
    QCOMPARE(doubleScalar.type(), QVariant::Double);
    QCOMPARE(doubleScalar.toDouble(), double(42));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Float"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant floatScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(floatScalar.isValid());
    QVERIFY(floatScalar.userType() == QMetaType::Float);
    QCOMPARE(floatScalar.toFloat(), float(42));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.String"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant stringScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(stringScalar.isValid());
    QCOMPARE(stringScalar.type(), QVariant::String);
    QCOMPARE(stringScalar.toString(), QStringLiteral("QOpcUa Teststring"));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.DateTime"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant dateTimeScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QCOMPARE(dateTimeScalar.type(), QVariant::DateTime);
    QVERIFY(dateTimeScalar.isValid());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.LocalizedText"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant ltScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(ltScalar.isValid());
    QVERIFY(ltScalar.value<QOpcUa::QLocalizedText>() == localizedTexts[0]);

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt16"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant uint16Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(uint16Scalar.isValid());
    QVERIFY(uint16Scalar.userType() == QMetaType::UShort);
    QVERIFY(uint16Scalar == std::numeric_limits<quint16>::max());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int16"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant int16Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int16Scalar.isValid());
    QVERIFY(int16Scalar.userType() == QMetaType::Short);
    QVERIFY(int16Scalar == std::numeric_limits<qint16>::min());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.UInt64"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant uint64Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(uint64Scalar.isValid());
    QCOMPARE(uint64Scalar.type(), QVariant::ULongLong);
    QVERIFY(uint64Scalar == std::numeric_limits<quint64>::max());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Int64"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant int64Scalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(int64Scalar.isValid());
    QCOMPARE(int64Scalar.type(), QVariant::LongLong);
    QVERIFY(int64Scalar == std::numeric_limits<qint64>::min());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Byte"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant byteScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(byteScalar.isValid());
    QVERIFY(byteScalar.userType() == QMetaType::UChar);
    QVERIFY(byteScalar == std::numeric_limits<quint8>::max());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.SByte"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant sbyteScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(sbyteScalar.isValid());
    QVERIFY(sbyteScalar.userType() == QMetaType::SChar);
    QVERIFY(sbyteScalar == std::numeric_limits<qint8>::min());

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.ByteString"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant byteStringScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(byteStringScalar.isValid());
    QVERIFY(byteStringScalar.userType() == QMetaType::QByteArray);
    QByteArray withNull("gh");
    withNull.append('\0');
    withNull.append("i");
    QVERIFY(byteStringScalar == withNull);

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.Guid"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant guidScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(guidScalar.isValid());
    QVERIFY(guidScalar.userType() == QMetaType::QUuid);
    QCOMPARE(guidScalar, QUuid("e0bd5ccd-f571-4545-9352-61a0f8cb9216"));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.NodeId"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant nodeIdScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(nodeIdScalar.isValid());
    QCOMPARE(nodeIdScalar.type(), QVariant::String);
    QCOMPARE(nodeIdScalar.toString(), QStringLiteral("ns=42;s=Test"));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.QualifiedName"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node)
    QVariant qualifiedNameScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(qualifiedNameScalar.value<QOpcUa::QQualifiedName>() == QOpcUa::QQualifiedName(0, "Test0"));

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.StatusCode"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node);
    QVariant statusCodeScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(statusCodeScalar.isValid());
    QCOMPARE(statusCodeScalar.value<QOpcUa::UaStatusCode>(), QOpcUa::UaStatusCode::BadInternalError);

    if (opcuaClient->backend() == QLatin1String("freeopcua"))
        QSKIP("XmlElement support is not yet implemented in the freeopcua backend");

    node.reset(opcuaClient->node("ns=2;s=Demo.Static.Scalar.XmlElement"));
    QVERIFY(node != 0);
    READ_MANDATORY_VARIABLE_NODE(node)
    QVariant xmlElementScalar = node->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(xmlElementScalar.isValid());
    QCOMPARE(xmlElementScalar.type(), QVariant::String);
    QCOMPARE(xmlElementScalar.toString(), xmlElements[0]);
}

void Tst_QOpcUaClient::stringCharset()
{
    QFETCH(QOpcUaClient *, opcuaClient);
    OpcuaConnector connector(opcuaClient, m_endpoint);

    QScopedPointer<QOpcUaNode> stringScalarNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.String"));
    QScopedPointer<QOpcUaNode> localizedScalarNode(opcuaClient->node("ns=2;s=Demo.Static.Scalar.LocalizedText"));
    QScopedPointer<QOpcUaNode> stringArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.String"));
    QScopedPointer<QOpcUaNode> localizedArrayNode(opcuaClient->node("ns=2;s=Demo.Static.Arrays.LocalizedText"));

    QVERIFY(stringScalarNode != 0);
    QVERIFY(localizedScalarNode != 0);
    QVERIFY(stringArrayNode != 0);
    QVERIFY(localizedArrayNode != 0);

    QString testString = QString::fromUtf8("🞀🞁🞂🞃");
    QOpcUa::QLocalizedText lt1("en", testString);
    QOpcUa::QLocalizedText lt2("de", testString);

    WRITE_VALUE_ATTRIBUTE(stringScalarNode, testString, QOpcUa::String);
    WRITE_VALUE_ATTRIBUTE(localizedScalarNode, QVariant::fromValue(localizedTexts[0]), QOpcUa::LocalizedText);

    QVariantList l;
    l.append(testString);
    l.append(testString);

    WRITE_VALUE_ATTRIBUTE(stringArrayNode, l, QOpcUa::String);

    l.clear();
    l.append(QVariant::fromValue(lt1));
    l.append(QVariant::fromValue(lt2));

    WRITE_VALUE_ATTRIBUTE(localizedArrayNode, l, QOpcUa::LocalizedText);

    READ_MANDATORY_VARIABLE_NODE(stringArrayNode);
    READ_MANDATORY_VARIABLE_NODE(localizedArrayNode);
    READ_MANDATORY_VARIABLE_NODE(stringScalarNode);
    READ_MANDATORY_VARIABLE_NODE(localizedScalarNode);

    QVariant result = stringScalarNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(result.toString() == testString);
    result = localizedScalarNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(result.value<QOpcUa::QLocalizedText>() == localizedTexts[0]);

    result = stringArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(result.type() == QVariant::List);
    QVERIFY(result.toList().length() == 2);
    QVERIFY(result.toList()[0].type() == QVariant::String);
    QVERIFY(result.toList()[0].toString() == testString);
    QVERIFY(result.toList()[1].type() == QVariant::String);
    QVERIFY(result.toList()[1].toString() == testString);

    result = localizedArrayNode->attribute(QOpcUaNode::NodeAttribute::Value);
    QVERIFY(result.type() == QVariant::List);
    QVERIFY(result.toList().length() == 2);
    QVERIFY(result.toList()[0].value<QOpcUa::QLocalizedText>() == lt1);
    QVERIFY(result.toList()[1].value<QOpcUa::QLocalizedText>() == lt2);
}

void Tst_QOpcUaClient::cleanupTestCase()
{
    if (m_serverProcess.state() == QProcess::Running) {
        m_serverProcess.kill();
        m_serverProcess.waitForFinished(2000);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTEST_SET_MAIN_SOURCE_PATH

    // run tests for all available backends
    QStringList availableBackends = QOpcUaProvider::availableBackends();
    if (availableBackends.empty()) {
        qDebug("No OPCUA backends found, skipping tests.");
        return EXIT_SUCCESS;
    }

    Tst_QOpcUaClient tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_client.moc"


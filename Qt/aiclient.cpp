#include "aiclient.h"
#include <QDebug>

AIClient::AIClient(const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_host(host)
    , m_port(port)
    , m_expectedResultSize(0)
    , m_receivingResult(false)
{
    connect(m_socket, &QTcpSocket::connected,
            this, &AIClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &AIClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &AIClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &AIClient::onErrorOccurred);
}

AIClient::~AIClient()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool AIClient::connectToAI()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }

    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        return m_socket->waitForConnected(3000);
    }

    m_socket->connectToHost(m_host, m_port);
    return m_socket->waitForConnected(3000);
}

bool AIClient::sendImageToAI(const QByteArray &imageData)
{
    if (!connectToAI()) {
        emit errorOccurred("AI 서버 연결 실패");
        return false;
    }

    // 크기 전송 (4 bytes, little-endian)
    quint32 size = imageData.size();
    QByteArray sizeData(4, 0);
    qToLittleEndian(size, reinterpret_cast<uchar*>(sizeData.data()));

    // 데이터 전송
    m_socket->write(sizeData);
    m_socket->write(imageData);
    m_socket->flush();

    m_receiveBuffer.clear();
    m_expectedResultSize = 0;
    m_receivingResult = false;

    return true;
}

bool AIClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void AIClient::onConnected()
{
    qInfo() << "✅ AI 서버 연결됨:" << m_host << ":" << m_port;
    emit connected();
}

void AIClient::onDisconnected()
{
    qInfo() << "❌ AI 서버 연결 종료";
    emit disconnected();
}

void AIClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_receiveBuffer.append(data);

    // 1. 결과 크기 읽기 (4 bytes)
    if (!m_receivingResult && m_receiveBuffer.size() >= 4) {
        m_expectedResultSize = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(m_receiveBuffer.constData())
        );
        m_receiveBuffer.remove(0, 4);
        m_receivingResult = true;
    }

    // 2. 결과 데이터 수신 완료 확인
    if (m_receivingResult && m_receiveBuffer.size() >= static_cast<int>(m_expectedResultSize)) {
        QByteArray resultData = m_receiveBuffer.left(m_expectedResultSize);
        m_receiveBuffer.remove(0, m_expectedResultSize);

        // JSON 파싱
        QJsonDocument doc = QJsonDocument::fromJson(resultData);
        if (!doc.isNull() && doc.isObject()) {
            emit resultReceived(doc.object());
        } else {
            emit errorOccurred("JSON 파싱 실패");
        }

        // 다음 결과를 위한 초기화
        m_receivingResult = false;
        m_expectedResultSize = 0;
    }
}

void AIClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    QString error = QString("AI 서버 에러: %1").arg(m_socket->errorString());
    qWarning() << error;
    emit errorOccurred(error);
}

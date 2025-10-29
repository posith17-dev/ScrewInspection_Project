#include "server.h"
#include "aiclient.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDir>

Server::Server(quint16 mfcPort, QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_mfcClient(nullptr)
    , m_aiClient(new AIClient("127.0.0.1", 6666, this))
    , m_mfcPort(mfcPort)
    , m_expectedImageSize(0)
    , m_receivingImage(false)
    , m_currentCameraId(0)
{
    // AI 클라이언트 시그널 연결
    connect(m_aiClient, &AIClient::resultReceived,
            this, &Server::onAIResultReceived);
    connect(m_aiClient, &AIClient::errorOccurred,
            this, &Server::errorOccurred);

    // 통계 타이머 (1분마다)
    m_statisticsTimer = new QTimer(this);
    connect(m_statisticsTimer, &QTimer::timeout,
            this, &Server::onStatisticsTimer);
    m_statisticsTimer->start(60000); // 1분

    // 데이터베이스 연결
    if (!connectDatabase()) {
        qWarning() << "⚠️  MySQL 연결 실패. CSV 모드로 동작합니다.";
    }
}

Server::~Server()
{
    stopServer();
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool Server::connectDatabase()
{
    m_database = QSqlDatabase::addDatabase("QMYSQL");
    m_database.setHostName("localhost");
    m_database.setPort(3306);
    m_database.setDatabaseName("screw_inspection");
    m_database.setUserName("inspector");
    m_database.setPassword("1234");  // ← 실제 비밀번호로 변경

    if (!m_database.open()) {
        qWarning() << "❌ MySQL 연결 실패:" << m_database.lastError().text();
        qWarning() << "   CSV 백업 모드로 계속 진행합니다.";
        return false;
    }

    qInfo() << "✅ MySQL 연결 성공: screw_inspection";

    // 테이블 존재 확인
    QSqlQuery query(m_database);
    if (!query.exec("SELECT 1 FROM inspection_results LIMIT 1")) {
        qWarning() << "⚠️  inspection_results 테이블이 없습니다.";
        qWarning() << "   DATABASE_DESIGN.sql을 먼저 실행하세요.";
        return false;
    }

    return true;
}

bool Server::startServer()
{
    if (!m_tcpServer->listen(QHostAddress::Any, m_mfcPort)) {
        QString error = QString("서버 시작 실패: %1").arg(m_tcpServer->errorString());
        emit errorOccurred(error);
        qCritical() << error;
        return false;
    }

    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &Server::onNewConnection);

    qInfo() << "========================================";
    qInfo() << "Qt 서버 시작! (MySQL 연동)";
    qInfo() << "MFC 포트:" << m_mfcPort;
    qInfo() << "AI 서버: 127.0.0.1:6666";
    qInfo() << "데이터베이스:" << (m_database.isOpen() ? "연결됨" : "CSV 모드");
    qInfo() << "클라이언트 연결 대기 중...";
    qInfo() << "========================================";

    emit serverStarted();
    return true;
}

void Server::stopServer()
{
    if (m_mfcClient) {
        m_mfcClient->disconnectFromHost();
        m_mfcClient->deleteLater();
        m_mfcClient = nullptr;
    }

    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    m_statisticsTimer->stop();

    emit serverStopped();
    qInfo() << "서버 종료됨";
}

void Server::onNewConnection()
{
    if (m_mfcClient) {
        m_mfcClient->disconnectFromHost();
        m_mfcClient->deleteLater();
    }

    m_mfcClient = m_tcpServer->nextPendingConnection();

    connect(m_mfcClient, &QTcpSocket::readyRead,
            this, &Server::onClientReadyRead);
    connect(m_mfcClient, &QTcpSocket::disconnected,
            this, &Server::onClientDisconnected);

    QString clientAddress = m_mfcClient->peerAddress().toString();
    qInfo() << "✅ MFC 클라이언트 연결:" << clientAddress;
    emit clientConnected(clientAddress);

    m_receiveBuffer.clear();
    m_expectedImageSize = 0;
    m_receivingImage = false;
}

void Server::onClientReadyRead()
{
    if (!m_mfcClient) return;

    QByteArray data = m_mfcClient->readAll();
    m_receiveBuffer.append(data);

    // 1. 이미지 크기 읽기
    if (!m_receivingImage && m_receiveBuffer.size() >= 4) {
        m_expectedImageSize = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(m_receiveBuffer.constData())
            );
        m_receiveBuffer.remove(0, 4);
        m_receivingImage = true;

        qInfo() << "📥 이미지 크기:" << m_expectedImageSize << "bytes";
        m_processingStartTime = QDateTime::currentDateTime();
    }

    // 2. 이미지 데이터 수신 완료
    if (m_receivingImage && m_receiveBuffer.size() >= static_cast<int>(m_expectedImageSize)) {
        QByteArray imageData = m_receiveBuffer.left(m_expectedImageSize);
        m_receiveBuffer.remove(0, m_expectedImageSize);

        qInfo() << "✅ 이미지 수신 완료";
        qInfo() << "🤖 AI 서버로 전송 중...";

        // AI 서버로 전송
        if (m_aiClient->sendImageToAI(imageData)) {
            qInfo() << "✅ AI 서버로 전송 완료";
        } else {
            QString error = "AI 서버 전송 실패";
            qCritical() << error;
            emit errorOccurred(error);
        }

        m_receivingImage = false;
        m_expectedImageSize = 0;
    }
}

void Server::onClientDisconnected()
{
    qInfo() << "❌ MFC 클라이언트 연결 종료";
    emit clientDisconnected();

    if (m_mfcClient) {
        m_mfcClient->deleteLater();
        m_mfcClient = nullptr;
    }

    m_receiveBuffer.clear();
    m_expectedImageSize = 0;
    m_receivingImage = false;
}

void Server::onAIResultReceived(const QJsonObject &result)
{
    qInfo() << "📥 AI 결과 수신:";
    qInfo() << "   분류:" << result["classification"].toString();
    qInfo() << "   결과:" << result["result"].toString();
    qInfo() << "   신뢰도:" << QString::number(result["confidence"].toDouble() * 100, 'f', 1) + "%";

    // 처리 시간 계산
    qint64 processingTime = m_processingStartTime.msecsTo(QDateTime::currentDateTime());

    // 결과에 추가 정보 추가
    QJsonObject enrichedResult = result;
    enrichedResult["camera_id"] = m_currentCameraId;
    enrichedResult["processing_time_ms"] = processingTime;
    enrichedResult["batch_number"] = m_currentBatchNumber;
    enrichedResult["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 데이터베이스에 저장
    if (m_database.isOpen()) {
        if (saveInspectionResult(enrichedResult)) {
            qInfo() << "✅ DB 저장 완료";
            updateDailyStatistics(enrichedResult);
            checkAndCreateAlarm(enrichedResult);
            updateBatchInfo(enrichedResult);
        }
    }

    // CSV 백업
    // saveToCSV(enrichedResult);

    // MFC로 결과 전송
    sendResultToMFC(enrichedResult);

    emit inspectionCompleted(enrichedResult);
}

bool Server::saveInspectionResult(const QJsonObject &result)
{
    QSqlQuery query(m_database);

    query.prepare(
        "INSERT INTO inspection_results ("
        "timestamp, classification, is_defect, confidence, result, "
        "camera_id, processing_time_ms, batch_number"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(result["classification"].toString());
    query.addBindValue(result["is_defect"].toBool());
    query.addBindValue(result["confidence"].toDouble());
    query.addBindValue(result["result"].toString());
    query.addBindValue(result["camera_id"].toInt());
    query.addBindValue(result["processing_time_ms"].toInt());
    query.addBindValue(result["batch_number"].toString());

    if (!query.exec()) {
        qWarning() << "DB 저장 실패:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Server::updateDailyStatistics(const QJsonObject &result)
{
    // 트리거가 자동으로 처리하지만, 수동으로도 가능
    QSqlQuery query(m_database);

    query.prepare(
        "INSERT INTO daily_statistics ("
        "date, total_inspections, defect_count, normal_count"
        ") VALUES (CURDATE(), 1, ?, ?) "
        "ON DUPLICATE KEY UPDATE "
        "total_inspections = total_inspections + 1, "
        "defect_count = defect_count + ?, "
        "normal_count = normal_count + ?, "
        "defect_rate = (defect_count * 100.0 / total_inspections), "
        "avg_confidence = (SELECT AVG(confidence) FROM inspection_results WHERE DATE(timestamp) = CURDATE())"
        );

    int isDefect = result["is_defect"].toBool() ? 1 : 0;
    int isNormal = result["is_defect"].toBool() ? 0 : 1;

    query.addBindValue(isDefect);
    query.addBindValue(isNormal);
    query.addBindValue(isDefect);
    query.addBindValue(isNormal);

    if (!query.exec()) {
        qWarning() << "통계 업데이트 실패:" << query.lastError().text();
        return false;
    }

    emit statisticsUpdated();
    return true;
}

bool Server::checkAndCreateAlarm(const QJsonObject &result)
{
    double confidence = result["confidence"].toDouble();

    // 낮은 신뢰도 알람
    if (confidence < 0.70) {
        QSqlQuery query(m_database);
        query.prepare(
            "INSERT INTO alarm_log (alarm_type, severity, message, camera_id, confidence_value) "
            "VALUES ('LOW_CONFIDENCE', 'WARNING', ?, ?, ?)"
            );

        QString message = QString("낮은 신뢰도 감지: %1%")
                              .arg(confidence * 100, 0, 'f', 2);

        query.addBindValue(message);
        query.addBindValue(result["camera_id"].toInt());
        query.addBindValue(confidence);

        if (!query.exec()) {
            qWarning() << "알람 생성 실패:" << query.lastError().text();
            return false;
        }

        qWarning() << "⚠️  알람 생성:" << message;
    }

    // 불량률 체크
    QSqlQuery statsQuery(m_database);
    statsQuery.exec("SELECT defect_rate FROM daily_statistics WHERE date = CURDATE()");

    if (statsQuery.next()) {
        double defectRate = statsQuery.value(0).toDouble();

        if (defectRate > 5.0) {  // 5% 이상
            QSqlQuery alarmQuery(m_database);
            alarmQuery.prepare(
                "INSERT INTO alarm_log (alarm_type, severity, message) "
                "VALUES ('HIGH_DEFECT_RATE', 'ERROR', ?)"
                );

            QString message = QString("높은 불량률 감지: %1%")
                                  .arg(defectRate, 0, 'f', 2);

            alarmQuery.addBindValue(message);
            alarmQuery.exec();

            qCritical() << "❌ 알람 생성:" << message;
        }
    }

    return true;
}

bool Server::updateBatchInfo(const QJsonObject &result)
{
    if (m_currentBatchNumber.isEmpty()) {
        return true;  // 배치 없음
    }

    QSqlQuery query(m_database);
    query.prepare(
        "UPDATE batch_info SET "
        "inspected_quantity = inspected_quantity + 1, "
        "defect_quantity = defect_quantity + ?, "
        "defect_rate = (defect_quantity * 100.0 / inspected_quantity), "
        "avg_confidence = (SELECT AVG(confidence) FROM inspection_results WHERE batch_number = ?) "
        "WHERE batch_number = ?"
        );

    int isDefect = result["is_defect"].toBool() ? 1 : 0;

    query.addBindValue(isDefect);
    query.addBindValue(m_currentBatchNumber);
    query.addBindValue(m_currentBatchNumber);

    if (!query.exec()) {
        qWarning() << "배치 업데이트 실패:" << query.lastError().text();
        return false;
    }

    // 배치 정보 조회 후 emit
    QJsonObject batchInfo = getCurrentBatch();
    emit batchUpdated(batchInfo);

    return true;
}

/*void Server::saveToCSV(const QJsonObject &result)
{
    QFile csvFile("inspection_history.csv");

    bool isNewFile = !csvFile.exists();

    if (csvFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&csvFile);

        // 헤더 (새 파일일 때만)
        if (isNewFile) {
            out << "timestamp,classification,is_defect,confidence,result,camera_id,processing_time_ms,batch_number\n";
        }

        // 데이터 추가
        out << result["timestamp"].toString() << ","
            << result["classification"].toString() << ","
            << (result["is_defect"].toBool() ? "1" : "0") << ","
            << result["confidence"].toDouble() << ","
            << result["result"].toString() << ","
            << result["camera_id"].toInt() << ","
            << result["processing_time_ms"].toInt() << ","
            << result["batch_number"].toString() << "\n";

        csvFile.close();
    }
}*/

void Server::sendResultToMFC(const QJsonObject &result)
{
    if (!m_mfcClient || m_mfcClient->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "⚠️  MFC 클라이언트가 연결되어 있지 않습니다";
        return;
    }

    // JSON을 바이트 배열로 변환
    QJsonDocument doc(result);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 크기 전송
    quint32 size = jsonData.size();
    QByteArray sizeData(4, 0);
    qToLittleEndian(size, reinterpret_cast<uchar*>(sizeData.data()));

    // 데이터 전송
    m_mfcClient->write(sizeData);
    m_mfcClient->write(jsonData);
    m_mfcClient->flush();

    qInfo() << "📤 MFC로 결과 전송 완료";
}

QJsonObject Server::getDailyStatistics(const QDate &date)
{
    QJsonObject stats;

    if (!m_database.isOpen()) {
        return stats;
    }

    QSqlQuery query(m_database);
    query.prepare(
        "SELECT total_inspections, defect_count, normal_count, "
        "defect_rate, avg_confidence, min_confidence, max_confidence "
        "FROM daily_statistics WHERE date = ?"
        );
    query.addBindValue(date);

    if (query.exec() && query.next()) {
        stats["date"] = date.toString(Qt::ISODate);
        stats["total_inspections"] = query.value(0).toInt();
        stats["defect_count"] = query.value(1).toInt();
        stats["normal_count"] = query.value(2).toInt();
        stats["defect_rate"] = query.value(3).toDouble();
        stats["avg_confidence"] = query.value(4).toDouble();
        stats["min_confidence"] = query.value(5).toDouble();
        stats["max_confidence"] = query.value(6).toDouble();
    }

    return stats;
}

QJsonObject Server::getRealtimeStatistics()
{
    QJsonObject stats;

    if (!m_database.isOpen()) {
        return stats;
    }

    QSqlQuery query(m_database);
    query.exec(
        "SELECT COUNT(*) as total, "
        "SUM(CASE WHEN is_defect = TRUE THEN 1 ELSE 0 END) as defects, "
        "ROUND(AVG(confidence * 100), 2) as avg_confidence, "
        "ROUND(SUM(CASE WHEN is_defect = TRUE THEN 1 ELSE 0 END) * 100.0 / COUNT(*), 2) as defect_rate "
        "FROM inspection_results "
        "WHERE timestamp >= DATE_SUB(NOW(), INTERVAL 1 HOUR)"
        );

    if (query.next()) {
        stats["total"] = query.value(0).toInt();
        stats["defects"] = query.value(1).toInt();
        stats["avg_confidence"] = query.value(2).toDouble();
        stats["defect_rate"] = query.value(3).toDouble();
        stats["period"] = "Last 1 hour";
    }

    return stats;
}

QString Server::startBatch(const QString &operatorName, const QString &shift, int targetQuantity)
{
    if (!m_database.isOpen()) {
        return QString();
    }

    // 배치 번호 생성 (날짜+시간)
    QString batchNumber = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO batch_info ("
        "batch_number, start_time, status, target_quantity, operator_name, shift"
        ") VALUES (?, NOW(), 'RUNNING', ?, ?, ?)"
        );

    query.addBindValue(batchNumber);
    query.addBindValue(targetQuantity);
    query.addBindValue(operatorName);
    query.addBindValue(shift);

    if (!query.exec()) {
        qWarning() << "배치 생성 실패:" << query.lastError().text();
        return QString();
    }

    m_currentBatchNumber = batchNumber;

    qInfo() << "✅ 배치 시작:" << batchNumber;
    qInfo() << "   작업자:" << operatorName;
    qInfo() << "   근무조:" << shift;
    qInfo() << "   목표:" << targetQuantity << "개";

    return batchNumber;
}

bool Server::endBatch(const QString &batchNumber)
{
    if (!m_database.isOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(
        "UPDATE batch_info SET "
        "end_time = NOW(), "
        "status = 'COMPLETED' "
        "WHERE batch_number = ?"
        );

    query.addBindValue(batchNumber);

    if (!query.exec()) {
        qWarning() << "배치 종료 실패:" << query.lastError().text();
        return false;
    }

    if (m_currentBatchNumber == batchNumber) {
        m_currentBatchNumber.clear();
    }

    qInfo() << "✅ 배치 종료:" << batchNumber;

    return true;
}

QJsonObject Server::getCurrentBatch()
{
    QJsonObject batch;

    if (m_currentBatchNumber.isEmpty() || !m_database.isOpen()) {
        return batch;
    }

    QSqlQuery query(m_database);
    query.prepare(
        "SELECT batch_number, operator_name, shift, "
        "target_quantity, inspected_quantity, defect_quantity, "
        "defect_rate, avg_confidence, status, start_time "
        "FROM batch_info WHERE batch_number = ?"
        );
    query.addBindValue(m_currentBatchNumber);

    if (query.exec() && query.next()) {
        batch["batch_number"] = query.value(0).toString();
        batch["operator_name"] = query.value(1).toString();
        batch["shift"] = query.value(2).toString();
        batch["target_quantity"] = query.value(3).toInt();
        batch["inspected_quantity"] = query.value(4).toInt();
        batch["defect_quantity"] = query.value(5).toInt();
        batch["defect_rate"] = query.value(6).toDouble();
        batch["avg_confidence"] = query.value(7).toDouble();
        batch["status"] = query.value(8).toString();
        batch["start_time"] = query.value(9).toDateTime().toString(Qt::ISODate);
    }

    return batch;
}

void Server::onStatisticsTimer()
{
    // 1분마다 통계 업데이트
    calculateRealtimeStatistics();
    checkAlarms();
}

void Server::calculateRealtimeStatistics()
{
    QJsonObject stats = getRealtimeStatistics();

    if (!stats.isEmpty()) {
        qInfo() << "📊 실시간 통계 (최근 1시간):";
        qInfo() << "   총 검사:" << stats["total"].toInt();
        qInfo() << "   불량:" << stats["defects"].toInt();
        qInfo() << "   불량률:" << stats["defect_rate"].toDouble() << "%";
        qInfo() << "   평균 신뢰도:" << stats["avg_confidence"].toDouble() << "%";

        emit statisticsUpdated();
    }
}

void Server::checkAlarms()
{
    if (!m_database.isOpen()) {
        return;
    }

    // 미해결 알람 체크
    QSqlQuery query(m_database);
    query.exec(
        "SELECT COUNT(*) FROM alarm_log "
        "WHERE is_resolved = FALSE AND severity IN ('ERROR', 'CRITICAL')"
        );

    if (query.next()) {
        int unresolved = query.value(0).toInt();
        if (unresolved > 0) {
            qWarning() << "⚠️  미해결 알람:" << unresolved << "건";
        }
    }
}

void Server::sendStatisticsToMFC()
{
    QJsonObject stats = getDailyStatistics(QDate::currentDate());

    if (!stats.isEmpty()) {
        sendResultToMFC(stats);
    }
}

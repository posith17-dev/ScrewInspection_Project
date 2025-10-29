#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include "aiclient.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(quint16 mfcPort = 5555, QObject *parent = nullptr);
    ~Server();

    bool startServer();
    void stopServer();

    // 데이터베이스 연결
    bool connectDatabase();

    // 통계 조회
    QJsonObject getDailyStatistics(const QDate &date = QDate::currentDate());
    QJsonObject getRealtimeStatistics();
    QJsonObject getCameraPerformance();

    // 배치 관리
    QString startBatch(const QString &operatorName, const QString &shift, int targetQuantity);
    bool endBatch(const QString &batchNumber);
    QJsonObject getCurrentBatch();

    // Excel 내보내기
    bool exportToExcel(const QString &filePath, const QDate &startDate, const QDate &endDate);

signals:
    void serverStarted();
    void serverStopped();
    void clientConnected(const QString &address);
    void clientDisconnected();
    void errorOccurred(const QString &error);
    void inspectionCompleted(const QJsonObject &result);
    void statisticsUpdated();
    void batchUpdated(const QJsonObject &batch);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onAIResultReceived(const QJsonObject &result);
    void onStatisticsTimer();
    void checkAlarms();

private:
    // 데이터베이스 관련
    bool saveInspectionResult(const QJsonObject &result);
    bool updateDailyStatistics(const QJsonObject &result);
    bool checkAndCreateAlarm(const QJsonObject &result);
    bool updateBatchInfo(const QJsonObject &result);

    // 통계 계산
    void calculateRealtimeStatistics();

    // MFC로 결과 전송
    void sendResultToMFC(const QJsonObject &result);
    void sendStatisticsToMFC();

    // 멤버 변수
    QTcpServer *m_tcpServer;
    QTcpSocket *m_mfcClient;
    AIClient *m_aiClient;

    quint16 m_mfcPort;
    QByteArray m_receiveBuffer;
    quint32 m_expectedImageSize;
    bool m_receivingImage;

    // 데이터베이스
    QSqlDatabase m_database;
    QString m_currentBatchNumber;

    // 통계 타이머
    QTimer *m_statisticsTimer;

    // 현재 검사 정보
    int m_currentCameraId;
    QDateTime m_processingStartTime;
};

#endif // SERVER_H

#ifndef AICLIENT_H
#define AICLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

class AIClient : public QObject
{
    Q_OBJECT

public:
    explicit AIClient(const QString &host = "127.0.0.1", 
                     quint16 port = 6666, 
                     QObject *parent = nullptr);
    ~AIClient();

    bool sendImageToAI(const QByteArray &imageData);
    bool isConnected() const;

signals:
    void resultReceived(const QJsonObject &result);
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    bool connectToAI();

    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
    
    QByteArray m_receiveBuffer;
    quint32 m_expectedResultSize;
    bool m_receivingResult;
};

#endif // AICLIENT_H

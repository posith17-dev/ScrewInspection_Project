#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// AI ���� ���� ����ü
struct AIResult {
    bool success;
    std::string classification;
    bool is_defect;
    float confidence;
    std::string result;

    AIResult() : success(false), is_defect(false), confidence(0.0f) {}
};

class CTcpClient
{
public:
    CTcpClient();
    ~CTcpClient();

    // ���� ����
    bool Connect(const char* ipAddress, int port);
    void Disconnect();
    bool IsConnected() const { return m_socket != INVALID_SOCKET; }

    // �̹��� ���� �� ��� ����
    bool SendImageAndReceiveResult(const unsigned char* pImageData,
        int width,
        int height,
        AIResult& result);

    // ���� �޽���
    std::string GetLastError() const { return m_lastError; }

private:
    SOCKET m_socket;
    bool m_wsaInitialized;
    std::string m_lastError;

    bool InitializeWinsock();
    void CleanupWinsock();

    // BMP ���ڵ�
    bool EncodeToBMP(const unsigned char* pImageData,
        int width,
        int height,
        std::vector<unsigned char>& bmpData);

    bool SendData(const unsigned char* data, int size);
    bool ReceiveData(std::vector<unsigned char>& buffer);
    bool ParseJsonResult(const std::string& jsonStr, AIResult& result);
};
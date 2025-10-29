#include "stdafx.h"
#include "TcpClient.h"
#include <sstream>
#include <fstream>

// =======================================================
// 🧱 CTcpClient 생성자/소멸자
// =======================================================
CTcpClient::CTcpClient()
    : m_socket(INVALID_SOCKET)
    , m_wsaInitialized(false)
{
    InitializeWinsock();   // WinSock 초기화
}

CTcpClient::~CTcpClient()
{
    Disconnect();          // 연결 종료
    CleanupWinsock();      // WinSock 정리
}

// =======================================================
// 🧰 WinSock 초기화 / 정리
// =======================================================
bool CTcpClient::InitializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  // WinSock 2.2 사용
    if (result != 0) {
        m_lastError = "WSAStartup failed";
        return false;
    }
    m_wsaInitialized = true;
    return true;
}

void CTcpClient::CleanupWinsock()
{
    if (m_wsaInitialized) {
        WSACleanup();     // WinSock 해제
        m_wsaInitialized = false;
    }
}

// =======================================================
// 🔌 서버 연결 / 해제
// =======================================================
bool CTcpClient::Connect(const char* ipAddress, int port)
{
    Disconnect();  // 기존 연결 정리

    // TCP 소켓 생성
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        m_lastError = "Socket creation failed";
        return false;
    }

    // 서버 주소 정보 설정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr);

    // 서버 연결 시도
    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        m_lastError = "Connection failed";
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    return true;
}

void CTcpClient::Disconnect()
{
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);  // 소켓 닫기
        m_socket = INVALID_SOCKET;
    }
}

// =======================================================
// 🧠 AI 서버로 이미지 전송 + 결과 수신 (수정됨!)
// =======================================================
bool CTcpClient::SendImageAndReceiveResult(
    const unsigned char* pImageData,
    int width,
    int height,
    AIResult& result)
{
    if (!IsConnected()) {
        m_lastError = "Not connected";
        return false;
    }

    // ⭐⭐⭐ pImageData는 이미 완전한 BMP 파일 데이터! ⭐⭐⭐
    // OnBnClickedSendToAiBtn에서 SaveImage로 저장한 BMP 파일을 읽어서 전달하므로
    // 이미 BMP 헤더(54바이트) + 픽셀 데이터가 포함되어 있음!

    // BMP 파일 크기 계산
    int rowSize = ((width * 3 + 3) / 4) * 4;  // 4바이트 정렬
    int imageDataSize = rowSize * height;
    unsigned int bmpFileSize = 54 + imageDataSize;  // BMP 헤더(54) + 픽셀 데이터

    // ✅ 1) 이미지 크기(4바이트) 전송
    unsigned char sizeBytes[4] = {
        (bmpFileSize) & 0xFF,
        (bmpFileSize >> 8) & 0xFF,
        (bmpFileSize >> 16) & 0xFF,
        (bmpFileSize >> 24) & 0xFF
    };

    if (!SendData(sizeBytes, 4)) {
        return false;
    }

    // ✅ 2) BMP 파일 데이터 그대로 전송
    if (!SendData(pImageData, bmpFileSize)) {
        return false;
    }

    // ✅ 3) 결과(JSON) 수신
    std::vector<unsigned char> resultBuffer;
    if (!ReceiveData(resultBuffer)) {
        return false;
    }

    // ✅ 4) 문자열 변환 후 파싱
    std::string jsonStr(resultBuffer.begin(), resultBuffer.end());
    return ParseJsonResult(jsonStr, result);
}

// =======================================================
// 🖼 BMP 포맷으로 인코딩 (24bit RGB)
// ⚠️ 이 함수는 이제 사용되지 않음 (참고용으로 유지)
// =======================================================
bool CTcpClient::EncodeToBMP(const unsigned char* pImageData,
    int width,
    int height,
    std::vector<unsigned char>& bmpData)
{
    // 한 행(row)의 크기 (4바이트 패딩)
    int rowSize = ((width * 3 + 3) / 4) * 4;
    int imageSize = rowSize * height;
    int fileSize = 54 + imageSize; // 54 = BMP 헤더 크기

    bmpData.resize(fileSize);
    unsigned char* data = bmpData.data();

    // --- BMP 파일 헤더 (14 bytes)
    data[0] = 'B'; data[1] = 'M';
    *(unsigned int*)(data + 2) = fileSize;
    *(unsigned int*)(data + 10) = 54;

    // --- DIB 헤더 (40 bytes)
    *(unsigned int*)(data + 14) = 40;
    *(int*)(data + 18) = width;
    *(int*)(data + 22) = -height; // 상하 반전 방지
    *(unsigned short*)(data + 26) = 1;
    *(unsigned short*)(data + 28) = 24;
    *(unsigned int*)(data + 34) = imageSize;

    // --- 픽셀 데이터 복사
    unsigned char* dest = data + 54;
    for (int y = 0; y < height; y++) {
        const unsigned char* src = pImageData + y * width * 3;
        memcpy(dest, src, width * 3);
        dest += rowSize; // 패딩 포함
    }

    return true;
}

// =======================================================
// 📤 TCP 송신
// =======================================================
bool CTcpClient::SendData(const unsigned char* data, int size)
{
    int totalSent = 0;
    while (totalSent < size) {
        int sent = send(m_socket, (const char*)(data + totalSent), size - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            m_lastError = "Send failed";
            return false;
        }
        totalSent += sent;
    }
    return true;
}

// =======================================================
// 📥 TCP 수신
// =======================================================
bool CTcpClient::ReceiveData(std::vector<unsigned char>& buffer)
{
    // (1) 수신할 데이터 크기 (4바이트)
    unsigned char sizeBytes[4];
    int totalReceived = 0;

    while (totalReceived < 4) {
        int received = recv(m_socket, (char*)(sizeBytes + totalReceived), 4 - totalReceived, 0);
        if (received <= 0) {
            m_lastError = "Receive size failed";
            return false;
        }
        totalReceived += received;
    }

    unsigned int resultSize = sizeBytes[0] | (sizeBytes[1] << 8) |
        (sizeBytes[2] << 16) | (sizeBytes[3] << 24);

    // (2) 실제 데이터 수신
    buffer.resize(resultSize);
    totalReceived = 0;

    while (totalReceived < (int)resultSize) {
        int received = recv(m_socket, (char*)(buffer.data() + totalReceived),
            resultSize - totalReceived, 0);
        if (received <= 0) {
            m_lastError = "Receive data failed";
            return false;
        }
        totalReceived += received;
    }

    return true;
}

// =======================================================
// 🧩 JSON 파싱 (문자열 기반 수동 파싱)
// =======================================================
bool CTcpClient::ParseJsonResult(const std::string& jsonStr, AIResult& result)
{
    try {
        // success 여부
        result.success = (jsonStr.find("\"success\":true") != std::string::npos);

        // classification
        size_t classPos = jsonStr.find("\"classification\":");
        if (classPos != std::string::npos) {
            classPos = jsonStr.find("\"", classPos + 17);
            if (classPos != std::string::npos) {
                classPos++;
                size_t endPos = jsonStr.find("\"", classPos);
                result.classification = jsonStr.substr(classPos, endPos - classPos);
            }
        }

        // is_defect
        result.is_defect = (jsonStr.find("\"is_defect\":true") != std::string::npos);

        // confidence (숫자 파싱)
        size_t confPos = jsonStr.find("\"confidence\":");
        if (confPos != std::string::npos) {
            confPos += 13;
            size_t endPos = jsonStr.find_first_of(",}", confPos);
            std::string confStr = jsonStr.substr(confPos, endPos - confPos);

            result.confidence = (float)atof(confStr.c_str());
        }
        else {
            result.confidence = 0.0f;
        }

        // result 메시지
        size_t resultPos = jsonStr.find("\"result\":");
        if (resultPos != std::string::npos) {
            resultPos = jsonStr.find("\"", resultPos + 9);
            if (resultPos != std::string::npos) {
                resultPos++;
                size_t endPos = jsonStr.find("\"", resultPos);
                result.result = jsonStr.substr(resultPos, endPos - resultPos);
            }
        }

        return true;
    }
    catch (...) {
        m_lastError = "JSON parsing failed";
        return false;
    }
}
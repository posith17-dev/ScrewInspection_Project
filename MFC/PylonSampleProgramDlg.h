// PylonSampleProgramDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "CameraManager.h"
#include "TcpClient.h"

// CPylonSampleProgramDlg 대화 상자
class CPylonSampleProgramDlg : public CDialog
{
	// ============================================================================
	// 생성자
	// ============================================================================
public:
	CPylonSampleProgramDlg(CWnd* pParent = NULL);	// 표준 생성자

	// ============================================================================
	// 대화 상자 데이터
	// ============================================================================
	enum { IDD = IDD_PYLONSAMPLEPROGRAM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원

	// ============================================================================
	// 구현
	// ============================================================================
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// ============================================================================
	// 메시지 핸들러
	// ============================================================================
public:
	// ----------------------------------------
	// 카메라 제어
	// ----------------------------------------
	afx_msg void OnBnClickedFindCamBtn();
	afx_msg void OnBnClickedOpenCameraBtn();
	afx_msg void OnBnClickedConnectCameraBtn();
	afx_msg void OnBnClickedCloseCamBtn();
	afx_msg void OnBnClickedGrabSingleBtn();
	afx_msg void OnBnClickedSoftTrigBtn();
	afx_msg void OnBnClickedSaveImgBtn();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedExitBtn();

	// ----------------------------------------
	// 라이브 뷰
	// ----------------------------------------
	afx_msg void OnBnClickedCam0Live();
	afx_msg void OnBnClickedCam1Live();
	afx_msg void OnBnClickedCam2Live();
	afx_msg void OnBnClickedCam3Live();
	afx_msg void OnBnClickedTwoCameraLiveBtn();

	// ----------------------------------------
	// 카메라 리스트
	// ----------------------------------------
	afx_msg void OnNMClickCameraList(NMHDR* pNMHDR, LRESULT* pResult);

	// ----------------------------------------
	// 서버 연결 및 AI 검사
	// ----------------------------------------
	afx_msg void OnBnClickedConnectServerBtn();
	afx_msg void OnBnClickedSendToAiBtn();
	afx_msg void OnStnClickedAiResultStatic();

	// ----------------------------------------
	// 자동 검사
	// ----------------------------------------
	afx_msg void OnBnClickedAutoInspectionStart();
	afx_msg void OnBnClickedAutoInspectionStop();

	// ----------------------------------------
	// 배치 관리
	// ----------------------------------------
	afx_msg void OnBnClickedBatchStart();
	afx_msg void OnBnClickedBatchEnd();

	// ----------------------------------------
	// 통계
	// ----------------------------------------
	afx_msg void OnBnClickedShowStatistics();

	// ----------------------------------------
	// 설정
	// ----------------------------------------
	afx_msg void OnBnClickedSettings();

	// ============================================================================
	// 멤버 변수
	// ============================================================================
public:
	// ========================================
	// 카메라 관련
	// ========================================
	CListCtrl m_ctrlCamList;
	CCameraManager m_CameraManager;

	// 카메라 정보
	char m_szCamName[CAM_NUM][100];
	char m_szSerialNum[CAM_NUM][100];
	char m_szInterface[CAM_NUM][100];
	int m_iCamNumber;
	int m_iCamPosition[CAM_NUM];
	CString m_strCamSerial[CAM_NUM];

	// 선택된 카메라
	int m_iCameraIndex;
	int m_iListIndex;
	int m_error;

	// ========================================
	// 이미지 버퍼
	// ========================================
	unsigned char** pImageresizeOrgBuffer[CAM_NUM];
	unsigned char** pImageColorDestBuffer[CAM_NUM];
	BITMAPINFO* bitmapinfo[CAM_NUM];

	// ========================================
	// 디스플레이 관련
	// ========================================
	HWND hWnd[CAM_NUM];
	HDC hdc[CAM_NUM];
	CRect rectStaticClient[CAM_NUM];

	// ========================================
	// 라이브 스레드 관련
	// ========================================
	bool bStopThread[CAM_NUM];
	bool bLiveFlag[CAM_NUM];
	int m_nCamIndexBuf[CAM_NUM];

	// ========================================
	// 성능 측정 (FPS)
	// ========================================
	LARGE_INTEGER start[CAM_NUM];
	LARGE_INTEGER end[CAM_NUM];
	LARGE_INTEGER freq[CAM_NUM];
	int nFrameCount[CAM_NUM];
	double time[CAM_NUM];

	// ========================================
	// TCP 클라이언트 (서버 연결)
	// ========================================
	CTcpClient m_tcpClient;
	CString m_strServerIP;
	int m_nServerPort;
	bool m_bConnectedToServer;

	// ========================================
	// 자동 검사 모드
	// ========================================
	bool m_bAutoInspectionMode;          // 자동 검사 모드 ON/OFF
	int m_nAutoInspectionInterval;       // 검사 주기 (밀리초)
	int m_nAutoInspectionCount;          // 목표 검사 횟수
	int m_nAutoInspectionCurrent;        // 현재 진행 횟수
	CWinThread* m_pAutoInspectionThread; // 자동 검사 스레드

	// ========================================
	// 배치 관리
	// ========================================
	CString m_strCurrentBatch;           // 현재 배치 번호
	CString m_strOperatorName;           // 작업자 이름
	CString m_strShift;                  // 근무조 (주간/야간)
	int m_nBatchTarget;                  // 배치 목표 수량
	int m_nBatchCurrent;                 // 배치 현재 진행 수량

	// ========================================
	// 통계 구조체
	// ========================================
	struct DailyStatistics
	{
		int totalCount;           // 총 검사 수
		int defectCount;          // 불량 수
		int normalCount;          // 정상 수
		double avgConfidence;     // 평균 신뢰도
		double defectRate;        // 불량률

		DailyStatistics()
		{
			totalCount = 0;
			defectCount = 0;
			normalCount = 0;
			avgConfidence = 0.0;
			defectRate = 0.0;
		}
	};

	DailyStatistics m_todayStats;        // 금일 통계

	// ============================================================================
	// 멤버 함수
	// ============================================================================
public:
	// ========================================
	// 카메라 관련
	// ========================================
	void GetSerialNumerFromFile(void);
	void AllocImageBuf(void);
	void InitBitmap(int nCamIndex);

	// ========================================
	// 디스플레이 함수
	// ========================================
	void DisplayCam0(void* pImageBuf);
	void DisplayCam1(void* pImageBuf);
	void DisplayCam2(void* pImageBuf);
	void DisplayCam3(void* pImageBuf);

	// ========================================
	// AI 결과 표시
	// ========================================
	void DisplayAIResult(const AIResult& result);

	// ========================================
	// 자동 검사
	// ========================================
	static UINT AutoInspectionThread(LPVOID pParam);

	// ========================================
	// 배치 관리
	// ========================================
	void UpdateBatchInfo();

	// ========================================
	// 통계
	// ========================================
	void UpdateStatistics(const AIResult& result);
	void ResetStatistics();

	// ========================================
	// 설정
	// ========================================
	void LoadSettings();
	void SaveSettings();
};

// ============================================================================
// 전역 함수
// ============================================================================
UINT LiveGrabThreadCam0(LPVOID pParam);
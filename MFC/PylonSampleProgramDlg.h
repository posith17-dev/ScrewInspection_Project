// PylonSampleProgramDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "CameraManager.h"
#include "TcpClient.h"

// CPylonSampleProgramDlg ��ȭ ����
class CPylonSampleProgramDlg : public CDialog
{
	// ============================================================================
	// ������
	// ============================================================================
public:
	CPylonSampleProgramDlg(CWnd* pParent = NULL);	// ǥ�� ������

	// ============================================================================
	// ��ȭ ���� ������
	// ============================================================================
	enum { IDD = IDD_PYLONSAMPLEPROGRAM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ����

	// ============================================================================
	// ����
	// ============================================================================
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// ============================================================================
	// �޽��� �ڵ鷯
	// ============================================================================
public:
	// ----------------------------------------
	// ī�޶� ����
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
	// ���̺� ��
	// ----------------------------------------
	afx_msg void OnBnClickedCam0Live();
	afx_msg void OnBnClickedCam1Live();
	afx_msg void OnBnClickedCam2Live();
	afx_msg void OnBnClickedCam3Live();
	afx_msg void OnBnClickedTwoCameraLiveBtn();

	// ----------------------------------------
	// ī�޶� ����Ʈ
	// ----------------------------------------
	afx_msg void OnNMClickCameraList(NMHDR* pNMHDR, LRESULT* pResult);

	// ----------------------------------------
	// ���� ���� �� AI �˻�
	// ----------------------------------------
	afx_msg void OnBnClickedConnectServerBtn();
	afx_msg void OnBnClickedSendToAiBtn();
	afx_msg void OnStnClickedAiResultStatic();

	// ----------------------------------------
	// �ڵ� �˻�
	// ----------------------------------------
	afx_msg void OnBnClickedAutoInspectionStart();
	afx_msg void OnBnClickedAutoInspectionStop();

	// ----------------------------------------
	// ��ġ ����
	// ----------------------------------------
	afx_msg void OnBnClickedBatchStart();
	afx_msg void OnBnClickedBatchEnd();

	// ----------------------------------------
	// ���
	// ----------------------------------------
	afx_msg void OnBnClickedShowStatistics();

	// ----------------------------------------
	// ����
	// ----------------------------------------
	afx_msg void OnBnClickedSettings();

	// ============================================================================
	// ��� ����
	// ============================================================================
public:
	// ========================================
	// ī�޶� ����
	// ========================================
	CListCtrl m_ctrlCamList;
	CCameraManager m_CameraManager;

	// ī�޶� ����
	char m_szCamName[CAM_NUM][100];
	char m_szSerialNum[CAM_NUM][100];
	char m_szInterface[CAM_NUM][100];
	int m_iCamNumber;
	int m_iCamPosition[CAM_NUM];
	CString m_strCamSerial[CAM_NUM];

	// ���õ� ī�޶�
	int m_iCameraIndex;
	int m_iListIndex;
	int m_error;

	// ========================================
	// �̹��� ����
	// ========================================
	unsigned char** pImageresizeOrgBuffer[CAM_NUM];
	unsigned char** pImageColorDestBuffer[CAM_NUM];
	BITMAPINFO* bitmapinfo[CAM_NUM];

	// ========================================
	// ���÷��� ����
	// ========================================
	HWND hWnd[CAM_NUM];
	HDC hdc[CAM_NUM];
	CRect rectStaticClient[CAM_NUM];

	// ========================================
	// ���̺� ������ ����
	// ========================================
	bool bStopThread[CAM_NUM];
	bool bLiveFlag[CAM_NUM];
	int m_nCamIndexBuf[CAM_NUM];

	// ========================================
	// ���� ���� (FPS)
	// ========================================
	LARGE_INTEGER start[CAM_NUM];
	LARGE_INTEGER end[CAM_NUM];
	LARGE_INTEGER freq[CAM_NUM];
	int nFrameCount[CAM_NUM];
	double time[CAM_NUM];

	// ========================================
	// TCP Ŭ���̾�Ʈ (���� ����)
	// ========================================
	CTcpClient m_tcpClient;
	CString m_strServerIP;
	int m_nServerPort;
	bool m_bConnectedToServer;

	// ========================================
	// �ڵ� �˻� ���
	// ========================================
	bool m_bAutoInspectionMode;          // �ڵ� �˻� ��� ON/OFF
	int m_nAutoInspectionInterval;       // �˻� �ֱ� (�и���)
	int m_nAutoInspectionCount;          // ��ǥ �˻� Ƚ��
	int m_nAutoInspectionCurrent;        // ���� ���� Ƚ��
	CWinThread* m_pAutoInspectionThread; // �ڵ� �˻� ������

	// ========================================
	// ��ġ ����
	// ========================================
	CString m_strCurrentBatch;           // ���� ��ġ ��ȣ
	CString m_strOperatorName;           // �۾��� �̸�
	CString m_strShift;                  // �ٹ��� (�ְ�/�߰�)
	int m_nBatchTarget;                  // ��ġ ��ǥ ����
	int m_nBatchCurrent;                 // ��ġ ���� ���� ����

	// ========================================
	// ��� ����ü
	// ========================================
	struct DailyStatistics
	{
		int totalCount;           // �� �˻� ��
		int defectCount;          // �ҷ� ��
		int normalCount;          // ���� ��
		double avgConfidence;     // ��� �ŷڵ�
		double defectRate;        // �ҷ���

		DailyStatistics()
		{
			totalCount = 0;
			defectCount = 0;
			normalCount = 0;
			avgConfidence = 0.0;
			defectRate = 0.0;
		}
	};

	DailyStatistics m_todayStats;        // ���� ���

	// ============================================================================
	// ��� �Լ�
	// ============================================================================
public:
	// ========================================
	// ī�޶� ����
	// ========================================
	void GetSerialNumerFromFile(void);
	void AllocImageBuf(void);
	void InitBitmap(int nCamIndex);

	// ========================================
	// ���÷��� �Լ�
	// ========================================
	void DisplayCam0(void* pImageBuf);
	void DisplayCam1(void* pImageBuf);
	void DisplayCam2(void* pImageBuf);
	void DisplayCam3(void* pImageBuf);

	// ========================================
	// AI ��� ǥ��
	// ========================================
	void DisplayAIResult(const AIResult& result);

	// ========================================
	// �ڵ� �˻�
	// ========================================
	static UINT AutoInspectionThread(LPVOID pParam);

	// ========================================
	// ��ġ ����
	// ========================================
	void UpdateBatchInfo();

	// ========================================
	// ���
	// ========================================
	void UpdateStatistics(const AIResult& result);
	void ResetStatistics();

	// ========================================
	// ����
	// ========================================
	void LoadSettings();
	void SaveSettings();
};

// ============================================================================
// ���� �Լ�
// ============================================================================
UINT LiveGrabThreadCam0(LPVOID pParam);
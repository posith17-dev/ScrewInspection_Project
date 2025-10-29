// PylonSampleProgramDlg.cpp : 구현 파일 집에서 깃 연습
// 1. AllocImageBuf에서 CA2W 사용 시 즉시 CString 변환
// 2. LiveGrabThreadCam0의 메모리 복사 경계 검사 강화
// 3. Mono12/16 처리 시 reSizeWidth 사용
// 4. NULL 체크 강화

#include "stdafx.h"
#include "PylonSampleProgram.h"
#include "PylonSampleProgramDlg.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ============================================================================
// 전역 변수
// ============================================================================
CPylonSampleProgramDlg* pMainDlg;

//==== std::string을 CString으로 변환하는 헬퍼 함수 (cpp 파일 상단에 추가)======

CString StdStringToCString(const std::string& str)
{
	// 빈 문자열 체크
	if (str.empty())
	{
		TRACE(_T("StdStringToCString: empty string\n"));
		return CString();
	}

	// NULL 포인터 체크
	const char* pStr = str.c_str();
	if (pStr == NULL)
	{
		TRACE(_T("StdStringToCString: NULL pointer!\n"));
		return CString();
	}

	// 문자열 길이 체크
	size_t len = str.length();
	if (len == 0 || len > 10000)  // 비정상적으로 긴 문자열
	{
		TRACE(_T("StdStringToCString: invalid length %zu\n"), len);
		return CString();
	}

	TRACE(_T("StdStringToCString: converting '%S' (len=%zu)\n"), pStr, len);

	CString result;

	// 안전하게 변환
	int nLen = MultiByteToWideChar(CP_ACP, 0, pStr, -1, NULL, 0);
	if (nLen > 0 && nLen < 10000)
	{
		wchar_t* pBuffer = result.GetBuffer(nLen);
		if (pBuffer != NULL)
		{
			int converted = MultiByteToWideChar(CP_ACP, 0, pStr, -1, pBuffer, nLen);
			result.ReleaseBuffer();

			if (converted > 0)
			{
				TRACE(_T("StdStringToCString: success! result='%s'\n"), (LPCTSTR)result);
			}
			else
			{
				TRACE(_T("StdStringToCString: conversion failed!\n"));
				result = _T("(변환 실패)");
			}
		}
		else
		{
			TRACE(_T("StdStringToCString: GetBuffer failed!\n"));
			result = _T("(버퍼 할당 실패)");
		}
	}
	else
	{
		TRACE(_T("StdStringToCString: invalid nLen=%d\n"), nLen);
		result = _T("(잘못된 길이)");
	}

	return result;
}
//=============================================================================

// ============================================================================
// 라이브 그랩 스레드
// ============================================================================

UINT LiveGrabThreadCam0(LPVOID pParam)
{
	int nindex = 0;
	int nCamIndex = *(int*)pParam;

	// 성능 측정 초기화
	QueryPerformanceCounter(&(pMainDlg->start[nCamIndex]));
	pMainDlg->nFrameCount[nCamIndex] = 0;

	// ========================================
	// 메인 루프
	// ========================================
	while (pMainDlg->bStopThread[nCamIndex] == true)
	{
		// ========================================
		// 카메라 연결 끊김 확인
		// ========================================
		if (pMainDlg->m_CameraManager.m_bRemoveCamera[nCamIndex] == true)
		{
			if (pMainDlg->m_strCamSerial[nCamIndex] == pMainDlg->m_ctrlCamList.GetItemText(0, 2))
			{
				pMainDlg->m_CameraManager.m_bRemoveCamera[nCamIndex] = false;
				pMainDlg->m_ctrlCamList.SetItemText(0, 3, _T("LostConnection"));
			}
			else if (pMainDlg->m_strCamSerial[nCamIndex] == pMainDlg->m_ctrlCamList.GetItemText(1, 2))
			{
				pMainDlg->m_CameraManager.m_bRemoveCamera[nCamIndex] = false;
				pMainDlg->m_ctrlCamList.SetItemText(1, 3, _T("LostConnection"));
			}
			Sleep(10);
			continue;
		}

		// ========================================
		// 이미지 캡처 완료 확인
		// ========================================
		if (!pMainDlg->m_CameraManager.CheckCaptureEnd(nCamIndex))
		{
			Sleep(1);
			continue;
		}

		// ========================================
		// NULL 포인터 체크
		// ========================================
		if (pMainDlg->pImageresizeOrgBuffer[nCamIndex] == NULL)
		{
			TRACE(_T("Error: pImageresizeOrgBuffer[%d] is NULL\n"), nCamIndex);
			pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
			Sleep(10);
			continue;
		}

		if (pMainDlg->pImageresizeOrgBuffer[nCamIndex][nindex] == NULL)
		{
			TRACE(_T("Error: pImageresizeOrgBuffer[%d][%d] is NULL\n"), nCamIndex, nindex);
			pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
			Sleep(10);
			continue;
		}

		// 프레임 카운터 증가
		pMainDlg->nFrameCount[nCamIndex]++;
		QueryPerformanceCounter(&(pMainDlg->end[nCamIndex]));

		// ========================================
		// 이미지 포맷별 처리
		// ========================================

		// Mono8 이미지
		if (pMainDlg->m_CameraManager.m_strCM_ImageForamt[nCamIndex] == "Mono8")
		{
			// NULL 체크
			if (pMainDlg->m_CameraManager.pImage8Buffer[nCamIndex] == NULL)
			{
				TRACE(_T("Error: pImage8Buffer[%d] is NULL\n"), nCamIndex);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			int height = pMainDlg->m_CameraManager.m_iCM_Height[nCamIndex];
			int width = pMainDlg->m_CameraManager.m_iCM_Width[nCamIndex];
			int reSizeWidth = pMainDlg->m_CameraManager.m_iCM_reSizeWidth[nCamIndex];

			// 크기 검증
			if (height <= 0 || width <= 0 || reSizeWidth <= 0 ||
				height > 10000 || width > 10000 || reSizeWidth > 10000)
			{
				TRACE(_T("Error: Invalid image size [%d]: %dx%d, reSizeWidth=%d\n"),
					nCamIndex, width, height, reSizeWidth);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			// width가 reSizeWidth보다 큰지 확인
			if (width > reSizeWidth)
			{
				TRACE(_T("Error: width(%d) > reSizeWidth(%d)\n"), width, reSizeWidth);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			// 메모리 복사 (안전하게)
			bool copySuccess = true;
			for (int y = 0; y < height; y++)
			{
				// 주소 계산 및 범위 체크
				size_t srcOffset = (size_t)y * width;
				size_t dstOffset = (size_t)y * reSizeWidth;

				unsigned char* pSrc = pMainDlg->m_CameraManager.pImage8Buffer[nCamIndex] + srcOffset;
				unsigned char* pDst = pMainDlg->pImageresizeOrgBuffer[nCamIndex][nindex] + dstOffset;

				// 주소 유효성 검사
				if (pSrc == NULL || pDst == NULL)
				{
					TRACE(_T("Error: NULL pointer at line %d\n"), y);
					copySuccess = false;
					break;
				}

				// 복사
				memcpy(pDst, pSrc, width);
			}

			pMainDlg->m_CameraManager.ReadEnd(nCamIndex);

			if (!copySuccess)
			{
				Sleep(10);
				continue;
			}

			// 디스플레이
			switch (nCamIndex)
			{
			case 0:
				if (pMainDlg->pImageresizeOrgBuffer[0][nindex])
					pMainDlg->DisplayCam0(pMainDlg->pImageresizeOrgBuffer[0][nindex]);
				break;
			case 1:
				if (pMainDlg->pImageresizeOrgBuffer[1][nindex])
					pMainDlg->DisplayCam1(pMainDlg->pImageresizeOrgBuffer[1][nindex]);
				break;
			case 2:
				if (pMainDlg->pImageresizeOrgBuffer[2][nindex])
					pMainDlg->DisplayCam2(pMainDlg->pImageresizeOrgBuffer[2][nindex]);
				break;
			case 3:
				if (pMainDlg->pImageresizeOrgBuffer[3][nindex])
					pMainDlg->DisplayCam3(pMainDlg->pImageresizeOrgBuffer[3][nindex]);
				break;
			}
		}
		// Mono12 / Mono16 이미지
		else if (pMainDlg->m_CameraManager.m_strCM_ImageForamt[nCamIndex] == "Mono12" ||
			pMainDlg->m_CameraManager.m_strCM_ImageForamt[nCamIndex] == "Mono16")
		{
			if (pMainDlg->m_CameraManager.pImage12Buffer[nCamIndex] == NULL)
			{
				TRACE(_T("Error: pImage12Buffer[%d] is NULL\n"), nCamIndex);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			int height = pMainDlg->m_CameraManager.m_iCM_Height[nCamIndex];
			int width = pMainDlg->m_CameraManager.m_iCM_Width[nCamIndex];
			int reSizeWidth = pMainDlg->m_CameraManager.m_iCM_reSizeWidth[nCamIndex];

			if (height <= 0 || width <= 0 || reSizeWidth <= 0 ||
				height > 10000 || width > 10000 || reSizeWidth > 10000)
			{
				TRACE(_T("Error: Invalid image size [%d]: %dx%d\n"), nCamIndex, width, height);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			bool copySuccess = true;
			for (int y = 0; y < height && copySuccess; y++)
			{
				for (int x = 0; x < width; x++)
				{
					int srcIdx = y * width + x;
					int dstIdx = y * reSizeWidth + x;  // reSizeWidth 사용

					// 범위 체크
					if (srcIdx >= width * height || dstIdx >= reSizeWidth * height)
					{
						TRACE(_T("Error: Index out of bounds at (%d, %d)\n"), x, y);
						copySuccess = false;
						break;
					}

					unsigned short* pSrc = pMainDlg->m_CameraManager.pImage12Buffer[nCamIndex] + srcIdx;
					unsigned char* pDst = pMainDlg->pImageresizeOrgBuffer[nCamIndex][nindex] + dstIdx;

					if (pSrc == NULL || pDst == NULL)
					{
						copySuccess = false;
						break;
					}

					*pDst = (unsigned char)((*pSrc) / 16);
				}
			}

			pMainDlg->m_CameraManager.ReadEnd(nCamIndex);

			if (!copySuccess)
			{
				Sleep(10);
				continue;
			}

			switch (nCamIndex)
			{
			case 0:
				if (pMainDlg->pImageresizeOrgBuffer[0][nindex])
					pMainDlg->DisplayCam0(pMainDlg->pImageresizeOrgBuffer[0][nindex]);
				break;
			case 1:
				if (pMainDlg->pImageresizeOrgBuffer[1][nindex])
					pMainDlg->DisplayCam1(pMainDlg->pImageresizeOrgBuffer[1][nindex]);
				break;
			case 2:
				if (pMainDlg->pImageresizeOrgBuffer[2][nindex])
					pMainDlg->DisplayCam2(pMainDlg->pImageresizeOrgBuffer[2][nindex]);
				break;
			case 3:
				if (pMainDlg->pImageresizeOrgBuffer[3][nindex])
					pMainDlg->DisplayCam3(pMainDlg->pImageresizeOrgBuffer[3][nindex]);
				break;
			}
		}
		// 컬러 이미지
		else
		{
			if (pMainDlg->m_CameraManager.pImage24Buffer[nCamIndex] == NULL)
			{
				TRACE(_T("Error: pImage24Buffer[%d] is NULL\n"), nCamIndex);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			if (pMainDlg->pImageColorDestBuffer[nCamIndex] == NULL ||
				pMainDlg->pImageColorDestBuffer[nCamIndex][nindex] == NULL)
			{
				TRACE(_T("Error: pImageColorDestBuffer[%d][%d] is NULL\n"), nCamIndex, nindex);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			int width = pMainDlg->m_CameraManager.m_iCM_Width[nCamIndex];
			int height = pMainDlg->m_CameraManager.m_iCM_Height[nCamIndex];

			if (height <= 0 || width <= 0 || height > 10000 || width > 10000)
			{
				TRACE(_T("Error: Invalid image size [%d]: %dx%d\n"), nCamIndex, width, height);
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			size_t copySize = (size_t)width * height * 3;

			unsigned char* pSrc = pMainDlg->m_CameraManager.pImage24Buffer[nCamIndex];
			unsigned char* pDst = pMainDlg->pImageColorDestBuffer[nCamIndex][nindex];

			if (pSrc != NULL && pDst != NULL)
			{
				memcpy(pDst, pSrc, copySize);
			}
			else
			{
				TRACE(_T("Error: Invalid color buffer pointers\n"));
				pMainDlg->m_CameraManager.ReadEnd(nCamIndex);
				Sleep(10);
				continue;
			}

			pMainDlg->m_CameraManager.ReadEnd(nCamIndex);

			switch (nCamIndex)
			{
			case 0:
				if (pMainDlg->pImageColorDestBuffer[0][nindex])
					pMainDlg->DisplayCam0(pMainDlg->pImageColorDestBuffer[0][nindex]);
				break;
			case 1:
				if (pMainDlg->pImageColorDestBuffer[1][nindex])
					pMainDlg->DisplayCam1(pMainDlg->pImageColorDestBuffer[1][nindex]);
				break;
			case 2:
				if (pMainDlg->pImageColorDestBuffer[2][nindex])
					pMainDlg->DisplayCam2(pMainDlg->pImageColorDestBuffer[2][nindex]);
				break;
			case 3:
				if (pMainDlg->pImageColorDestBuffer[3][nindex])
					pMainDlg->DisplayCam3(pMainDlg->pImageColorDestBuffer[3][nindex]);
				break;
			}
		}

		// ========================================
		// 버퍼 인덱스 순환
		// ========================================
		nindex++;
		if (nindex >= BUF_NUM)
		{
			nindex = 0;
		}

		// ========================================
		// FPS 계산 및 표시 (1초마다)
		// ========================================
		double currentTime = pMainDlg->end[nCamIndex].QuadPart / (pMainDlg->freq[nCamIndex].QuadPart / 1000.0);
		double startTime = pMainDlg->start[nCamIndex].QuadPart / (pMainDlg->freq[nCamIndex].QuadPart / 1000.0);

		if (currentTime > startTime + 1000)
		{
			TCHAR temp[64] = { 0 };
			_stprintf_s(temp, _T("%d fps"), pMainDlg->nFrameCount[nCamIndex]);

			switch (nCamIndex)
			{
			case 0: pMainDlg->SetDlgItemText(IDC_CAMERA0_STATS, temp); break;
			case 1: pMainDlg->SetDlgItemText(IDC_CAMERA1_STATS, temp); break;
			case 2: pMainDlg->SetDlgItemText(IDC_CAMERA2_STATS, temp); break;
			case 3: pMainDlg->SetDlgItemText(IDC_CAMERA3_STATS, temp); break;
			}

			pMainDlg->nFrameCount[nCamIndex] = 0;
			QueryPerformanceCounter(&(pMainDlg->start[nCamIndex]));
		}

		// ========================================
		// 프레임 정보 표시
		// ========================================
		TCHAR Info[256] = { 0 };
		_stprintf_s(Info, _T("Grabbed Frame = %d , SkippedFrame = %d"),
			pMainDlg->m_CameraManager.m_iGrabbedFrame[nCamIndex],
			pMainDlg->m_CameraManager.m_iSkippiedFrame[nCamIndex]);

		switch (nCamIndex)
		{
		case 0: pMainDlg->SetDlgItemText(IDC_CAM0_INFO, Info); break;
		case 1: pMainDlg->SetDlgItemText(IDC_CAM1_INFO, Info); break;
		case 2: pMainDlg->SetDlgItemText(IDC_CAM2_INFO, Info); break;
		case 3: pMainDlg->SetDlgItemText(IDC_CAM3_INFO, Info); break;
		}
	}

	return 0;
}


// ============================================================================
// CAboutDlg - 정보 대화상자
// ============================================================================
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ============================================================================
// CPylonSampleProgramDlg 대화 상자
// ============================================================================

// ----------------------------------------------------------------------------
// 생성자
// ----------------------------------------------------------------------------
CPylonSampleProgramDlg::CPylonSampleProgramDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPylonSampleProgramDlg::IDD, pParent)
	, m_strServerIP(_T("127.0.0.1"))
	, m_nServerPort(5555)
	, m_bConnectedToServer(false)
	, m_bAutoInspectionMode(false)
	, m_nAutoInspectionInterval(2000)
	, m_nAutoInspectionCount(100)
	, m_nAutoInspectionCurrent(0)
	, m_pAutoInspectionThread(nullptr)
	, m_nBatchTarget(0)
	, m_nBatchCurrent(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// 카메라 관련 초기화
	for (int i = 0; i < CAM_NUM; i++)
	{
		pImageresizeOrgBuffer[i] = NULL;
		pImageColorDestBuffer[i] = NULL;
		bitmapinfo[i] = NULL;
		bStopThread[i] = false;
		bLiveFlag[i] = false;
		nFrameCount[i] = 0;
		time[i] = 0;
		QueryPerformanceFrequency(&freq[i]);
		m_nCamIndexBuf[i] = i;
		m_CameraManager.m_iCM_Width[i] = 1;
		m_CameraManager.m_iCM_Height[i] = 1;
	}

	m_iCameraIndex = -1;

	// 통계 초기화
	ResetStatistics();
}

// ----------------------------------------------------------------------------
// DoDataExchange
// ----------------------------------------------------------------------------
void CPylonSampleProgramDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAMERA_LIST, m_ctrlCamList);
}

// ----------------------------------------------------------------------------
// 메시지 맵
// ----------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CPylonSampleProgramDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	// 카메라 제어
	ON_BN_CLICKED(IDC_FIND_CAM_BTN, &CPylonSampleProgramDlg::OnBnClickedFindCamBtn)
	ON_BN_CLICKED(IDC_OPEN_CAMERA_BTN, &CPylonSampleProgramDlg::OnBnClickedOpenCameraBtn)
	ON_BN_CLICKED(IDC_CONNECT_CAMERA_BTN, &CPylonSampleProgramDlg::OnBnClickedConnectCameraBtn)
	ON_BN_CLICKED(IDC_CLOSE_CAM_BTN, &CPylonSampleProgramDlg::OnBnClickedCloseCamBtn)
	ON_BN_CLICKED(IDC_GRAB_SINGLE_BTN, &CPylonSampleProgramDlg::OnBnClickedGrabSingleBtn)
	ON_BN_CLICKED(IDC_SOFT_TRIG_BTN, &CPylonSampleProgramDlg::OnBnClickedSoftTrigBtn)
	ON_BN_CLICKED(IDC_SAVE_IMG_BTN, &CPylonSampleProgramDlg::OnBnClickedSaveImgBtn)
	ON_BN_CLICKED(IDC_BUTTON5, &CPylonSampleProgramDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_EXIT_BTN, &CPylonSampleProgramDlg::OnBnClickedExitBtn)

	// 라이브 뷰
	ON_BN_CLICKED(IDC_CAM0_LIVE, &CPylonSampleProgramDlg::OnBnClickedCam0Live)
	ON_BN_CLICKED(IDC_CAM1_LIVE, &CPylonSampleProgramDlg::OnBnClickedCam1Live)
	ON_BN_CLICKED(IDC_CAM2_LIVE, &CPylonSampleProgramDlg::OnBnClickedCam2Live)
	ON_BN_CLICKED(IDC_CAM3_LIVE, &CPylonSampleProgramDlg::OnBnClickedCam3Live)
	ON_BN_CLICKED(IDC_TWO_CAMERA_LIVE_BTN, &CPylonSampleProgramDlg::OnBnClickedTwoCameraLiveBtn)

	// 카메라 리스트
	ON_NOTIFY(NM_CLICK, IDC_CAMERA_LIST, &CPylonSampleProgramDlg::OnNMClickCameraList)

	// 서버 & AI
	ON_BN_CLICKED(IDC_CONNECT_SERVER_BTN, &CPylonSampleProgramDlg::OnBnClickedConnectServerBtn)
	ON_BN_CLICKED(IDC_SEND_TO_AI_BTN, &CPylonSampleProgramDlg::OnBnClickedSendToAiBtn)
	ON_STN_CLICKED(IDC_AI_RESULT_STATIC, &CPylonSampleProgramDlg::OnStnClickedAiResultStatic)

	// 자동 검사
	ON_BN_CLICKED(IDC_AUTO_INSPECTION_BTN, &CPylonSampleProgramDlg::OnBnClickedAutoInspectionStart)
	ON_COMMAND(IDC_AUTO_INSPECTION_STOP, &CPylonSampleProgramDlg::OnBnClickedAutoInspectionStop)

	// 배치 관리
	ON_BN_CLICKED(IDC_BATCH_START_BTN, &CPylonSampleProgramDlg::OnBnClickedBatchStart)
	ON_BN_CLICKED(IDC_BATCH_END_BTN, &CPylonSampleProgramDlg::OnBnClickedBatchEnd)

	// 통계
	ON_BN_CLICKED(IDC_SHOW_STATISTICS_BTN, &CPylonSampleProgramDlg::OnBnClickedShowStatistics)

	// 설정
	ON_BN_CLICKED(IDC_SETTINGS_BTN, &CPylonSampleProgramDlg::OnBnClickedSettings)
END_MESSAGE_MAP()

// ============================================================================
// 초기화
// ============================================================================
BOOL CPylonSampleProgramDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ========================================
	// 시스템 메뉴 설정
	// ========================================
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// ========================================
	// 카메라 리스트 컨트롤 설정
	// ========================================
	m_ctrlCamList.InsertColumn(0, _T("모델명"), LVCFMT_CENTER, 130, -1);
	m_ctrlCamList.InsertColumn(1, _T("Position"), LVCFMT_CENTER, 80, -1);
	m_ctrlCamList.InsertColumn(2, _T("SerialNum"), LVCFMT_CENTER, 90, -1);
	m_ctrlCamList.InsertColumn(3, _T("Stats"), LVCFMT_CENTER, 150, -1);

	m_ctrlCamList.SetExtendedStyle(
		LVS_EX_FULLROWSELECT |
		LVS_EX_GRIDLINES |
		LVS_EX_ONECLICKACTIVATE |
		LVS_EX_HEADERDRAGDROP
	);

	pMainDlg = this;

	// ========================================
	// 카메라 관련 초기화
	// ========================================
	for (int i = 0; i < CAM_NUM; i++)
	{
		pImageresizeOrgBuffer[i] = NULL;
		pImageColorDestBuffer[i] = NULL;
		bitmapinfo[i] = NULL;
		bStopThread[i] = false;
		bLiveFlag[i] = false;
		QueryPerformanceFrequency(&freq[i]);
		nFrameCount[i] = 0;
		time[i] = 0;
		m_nCamIndexBuf[i] = i;
		m_CameraManager.m_iCM_Width[i] = 1;
		m_CameraManager.m_iCM_Height[i] = 1;
	}

	m_iCameraIndex = -1;

	// ========================================
	// 설정 로드
	// ========================================
	LoadSettings();

	// ========================================
	// TCP 서버 연결 설정
	// ========================================
	SetDlgItemText(IDC_SERVER_IP_EDIT, m_strServerIP);
	SetDlgItemInt(IDC_SERVER_PORT_EDIT, m_nServerPort);

	// ========================================
	// 배치 정보 초기화
	// ========================================
	CWnd* pBatchInfo = GetDlgItem(IDC_BATCH_INFO_STATIC);
	if (pBatchInfo)
	{
		pBatchInfo->SetWindowText(_T("배치 없음"));
	}

	CWnd* pProgress = GetDlgItem(IDC_PROGRESS_STATIC);
	if (pProgress)
	{
		pProgress->SetWindowText(_T("대기 중"));
	}

	// ========================================
	// 자동 검사 버튼 초기화
	// ========================================
	CWnd* pAutoBtn = GetDlgItem(IDC_AUTO_INSPECTION_BTN);
	if (pAutoBtn)
	{
		pAutoBtn->SetWindowText(_T("자동 검사 시작"));
	}

	return TRUE;
}

void CPylonSampleProgramDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

void CPylonSampleProgramDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CPylonSampleProgramDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ============================================================================
// 카메라 제어 함수들
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedFindCamBtn()
{
	GetSerialNumerFromFile();
	m_error = m_CameraManager.FindCamera(m_szCamName, m_szSerialNum, m_szInterface, &m_iCamNumber);

	if (m_error == 0)
	{
		m_ctrlCamList.DeleteAllItems();
		int nCount = 0;
		CString strSerialNum;

		for (int i = 0; i < m_iCamNumber; i++)
		{
			nCount++;
			CString strcamname = (CString)m_szCamName[i];
			strSerialNum = (CString)m_szSerialNum[i];

			for (int camIdx = 0; camIdx < CAM_NUM; camIdx++)
			{
				if (strSerialNum == m_strCamSerial[camIdx])
				{
					m_iCamPosition[camIdx] = nCount;

					m_ctrlCamList.InsertItem(i, strcamname);

					CString strPosition;
					strPosition.Format(_T("Inspect%d"), camIdx);
					m_ctrlCamList.SetItemText(i, 1, strPosition);
					m_ctrlCamList.SetItemText(i, 2, strSerialNum);
					m_ctrlCamList.SetItemText(i, 3, _T("Find_Success"));
					break;
				}
			}
		}
	}
	else if (m_error == -1)
	{
		AfxMessageBox(_T("연결된 카메라가 없습니다."));
	}
	else if (m_error == -2)
	{
		AfxMessageBox(_T("Pylon Function Error"));
	}
}

void CPylonSampleProgramDlg::GetSerialNumerFromFile(void)
{
	TCHAR buff[100];
	CString strTemp;
	DWORD length;

	for (int i = 0; i < CAM_NUM; i++)
	{
		strTemp.Format(_T("CAM%d"), i + 1);

		memset(buff, 0x00, sizeof(buff));
		length = GetPrivateProfileString(
			strTemp,
			_T("Serial"),
			_T(""),
			buff,
			sizeof(buff),
			_T(".\\CameraInfo.txt")
		);
		m_strCamSerial[i] = (CString)buff;
	}
}

void CPylonSampleProgramDlg::OnNMClickCameraList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_iListIndex = pNMListView->iItem;
	SetDlgItemText(IDC_SELECT_CAMERA, m_ctrlCamList.GetItemText(m_iListIndex, 1));

	CString strPosition = m_ctrlCamList.GetItemText(pNMListView->iItem, 1);

	if (strPosition == _T("Inspect0"))
	{
		m_iCameraIndex = 0;
	}
	else if (strPosition == _T("Inspect1"))
	{
		m_iCameraIndex = 1;
	}
	else if (strPosition == _T("Inspect2"))
	{
		m_iCameraIndex = 2;
	}
	else if (strPosition == _T("Inspect3"))
	{
		m_iCameraIndex = 3;
	}

	*pResult = 0;
}

void CPylonSampleProgramDlg::OnBnClickedOpenCameraBtn()
{
	if (m_iCameraIndex != -1)
	{
		int error = m_CameraManager.Open_Camera(m_iCameraIndex, m_iCamPosition[m_iCameraIndex]);

		if (error == 0)
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Open_Success"));
		}
		else if (error == -1)
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Already_Open"));
		}
		else
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Open_Fail"));
		}
	}
	else
	{
		AfxMessageBox(_T("리스트에서 카메라를 먼저 선택하세요!!"));
	}
}

void CPylonSampleProgramDlg::OnBnClickedCloseCamBtn()
{
	if (m_CameraManager.m_bCamOpenFlag[m_iCameraIndex] == true)
	{
		if (m_CameraManager.Close_Camera(m_iCameraIndex) == 0)
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Close_Success"));

			if (bitmapinfo[m_iCameraIndex])
			{
				delete bitmapinfo[m_iCameraIndex];
				bitmapinfo[m_iCameraIndex] = NULL;
			}

			if (pImageColorDestBuffer[m_iCameraIndex])
			{
				for (int i = 0; i < BUF_NUM; i++)
				{
					free(pImageColorDestBuffer[m_iCameraIndex][i]);
				}
				free(pImageColorDestBuffer[m_iCameraIndex]);
				pImageColorDestBuffer[m_iCameraIndex] = NULL;
			}

			if (pImageresizeOrgBuffer[m_iCameraIndex])
			{
				for (int i = 0; i < BUF_NUM; i++)
				{
					free(pImageresizeOrgBuffer[m_iCameraIndex][i]);
				}
				free(pImageresizeOrgBuffer[m_iCameraIndex]);
				pImageresizeOrgBuffer[m_iCameraIndex] = NULL;
			}
		}
		else
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Close_Fail"));
		}
	}
}

// ============================================================================
// OnBnClickedConnectCameraBtn - MultiByteToWideChar 버전
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedConnectCameraBtn()
{
	TRACE(_T("\n========================================\n"));
	TRACE(_T("OnBnClickedConnectCameraBtn START\n"));
	TRACE(_T("========================================\n"));

	if (m_iCameraIndex < 0)
	{
		TRACE(_T("ERROR: Camera index < 0\n"));
		AfxMessageBox(_T("Please select a camera first!"));
		return;
	}

	TRACE(_T("Camera Index: %d\n"), m_iCameraIndex);
	TRACE(_T("Camera Open Flag: %d\n"), m_CameraManager.m_bCamOpenFlag[m_iCameraIndex]);

	if (m_CameraManager.m_bCamOpenFlag[m_iCameraIndex] == true)
	{
		TRACE(_T("Calling Connect_Camera...\n"));
		int result = m_CameraManager.Connect_Camera(m_iCameraIndex, 0, 0, 1984, 1264, _T("Mono8"));
		TRACE(_T("Connect_Camera result: %d\n"), result);

		if (result == 0)
		{
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Connect_Success"));

			// 버퍼 할당 전 상태 확인
			TRACE(_T("\n>>> BEFORE AllocImageBuf <<<\n"));
			TRACE(_T("Width: %d\n"), m_CameraManager.m_iCM_Width[m_iCameraIndex]);
			TRACE(_T("Height: %d\n"), m_CameraManager.m_iCM_Height[m_iCameraIndex]);
			TRACE(_T("reSizeWidth: %d\n"), m_CameraManager.m_iCM_reSizeWidth[m_iCameraIndex]);
			TRACE(_T("Format: %s\n"), m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex]);

			// 버퍼 할당
			TRACE(_T("Calling AllocImageBuf...\n"));
			AllocImageBuf();
			TRACE(_T("AllocImageBuf returned\n"));

			// 버퍼 할당 후 확인
			TRACE(_T("\n>>> AFTER AllocImageBuf <<<\n"));
			TRACE(_T("pImageresizeOrgBuffer[%d]: %p\n"),
				m_iCameraIndex, pImageresizeOrgBuffer[m_iCameraIndex]);

			// 버퍼 할당 확인
			if (pImageresizeOrgBuffer[m_iCameraIndex] == NULL)
			{
				TRACE(_T("ERROR: Buffer allocation failed!\n"));
				AfxMessageBox(_T("Image buffer allocation failed!"));
				m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Buffer_Alloc_Failed"));
				return;
			}

			// 개별 버퍼 확인
			bool allBuffersValid = true;
			for (int i = 0; i < BUF_NUM; i++)
			{
				TRACE(_T("Buffer[%d][%d]: %p\n"),
					m_iCameraIndex, i, pImageresizeOrgBuffer[m_iCameraIndex][i]);

				if (pImageresizeOrgBuffer[m_iCameraIndex][i] == NULL)
				{
					TRACE(_T("ERROR: Buffer[%d][%d] is NULL\n"), m_iCameraIndex, i);
					allBuffersValid = false;
					break;
				}
			}

			if (!allBuffersValid)
			{
				TRACE(_T("ERROR: Some buffers are NULL!\n"));
				AfxMessageBox(_T("Some buffers allocation failed!"));
				m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Buffer_Alloc_Failed"));
				return;
			}

			TRACE(_T("All buffers are valid!\n"));

			// Bitmap 초기화
			TRACE(_T("Calling InitBitmap...\n"));
			InitBitmap(m_iCameraIndex);
			TRACE(_T("InitBitmap returned\n"));

			// 성공 메시지
			CString strFormat = m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex];

			CString msg;
			msg.Format(_T("Camera %d connected!\n\nBuffer size: %dx%d\nFormat: %s"),
				m_iCameraIndex,
				m_CameraManager.m_iCM_Width[m_iCameraIndex],
				m_CameraManager.m_iCM_Height[m_iCameraIndex],
				strFormat);

			TRACE(_T("%s\n"), msg);
			TRACE(_T("Connect SUCCESS!\n"));
		}
		else
		{
			TRACE(_T("Connect FAILED! (result=%d)\n"), result);
			m_ctrlCamList.SetItemText(m_iListIndex, 3, _T("Connect_Fail"));
		}
	}
	else
	{
		TRACE(_T("ERROR: Camera not opened!\n"));
		AfxMessageBox(_T("Please Open Camera first!"));
	}

	TRACE(_T("========================================\n"));
	TRACE(_T("OnBnClickedConnectCameraBtn END\n"));
	TRACE(_T("========================================\n\n"));
}


//============================================================================
void CPylonSampleProgramDlg::OnBnClickedGrabSingleBtn()
{
	if (m_CameraManager.m_bCamConnectFlag[m_iCameraIndex] == true)
	{
		bLiveFlag[m_iCameraIndex] = false;

		if (m_CameraManager.SingleGrab(m_iCameraIndex) == 0)
		{
			while (bLiveFlag[m_iCameraIndex] == false)
			{
				if (m_CameraManager.CheckCaptureEnd(m_iCameraIndex))
				{
					if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono8")
					{
						for (int y = 0; y < m_CameraManager.m_iCM_Height[m_iCameraIndex]; y++)
						{
							memcpy(
								&pImageresizeOrgBuffer[m_iCameraIndex][0][y * m_CameraManager.m_iCM_reSizeWidth[m_iCameraIndex]],
								&m_CameraManager.pImage8Buffer[m_iCameraIndex][y * m_CameraManager.m_iCM_Width[m_iCameraIndex]],
								m_CameraManager.m_iCM_Width[m_iCameraIndex]
							);
						}

						m_CameraManager.ReadEnd(m_iCameraIndex);

						switch (m_iCameraIndex)
						{
						case 0: DisplayCam0(pImageresizeOrgBuffer[0][0]); break;
						case 1: DisplayCam1(pImageresizeOrgBuffer[1][0]); break;
						case 2: DisplayCam2(pImageresizeOrgBuffer[2][0]); break;
						case 3: DisplayCam3(pImageresizeOrgBuffer[3][0]); break;
						}
					}
					else if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono12")
					{
						int height = m_CameraManager.m_iCM_Height[m_iCameraIndex];
						int width = m_CameraManager.m_iCM_Width[m_iCameraIndex];

						for (int y = 0; y < height; y++)
						{
							for (int x = 0; x < width; x++)
							{
								pImageresizeOrgBuffer[m_iCameraIndex][0][y * width + x] =
									(m_CameraManager.pImage12Buffer[m_iCameraIndex][y * m_CameraManager.m_iCM_Width[m_iCameraIndex] + x] / 16);
							}
						}

						m_CameraManager.ReadEnd(m_iCameraIndex);

						switch (m_iCameraIndex)
						{
						case 0: DisplayCam0(pImageresizeOrgBuffer[0][0]); break;
						case 1: DisplayCam1(pImageresizeOrgBuffer[1][0]); break;
						case 2: DisplayCam2(pImageresizeOrgBuffer[2][0]); break;
						case 3: DisplayCam3(pImageresizeOrgBuffer[3][0]); break;
						}
					}
					else
					{
						memcpy(
							pImageColorDestBuffer[m_iCameraIndex][0],
							m_CameraManager.pImage24Buffer[m_iCameraIndex],
							m_CameraManager.m_iCM_reSizeWidth[m_iCameraIndex] * m_CameraManager.m_iCM_Height[m_iCameraIndex] * 3
						);

						m_CameraManager.ReadEnd(m_iCameraIndex);

						switch (m_iCameraIndex)
						{
						case 0: DisplayCam0(pImageColorDestBuffer[0][0]); break;
						case 1: DisplayCam1(pImageColorDestBuffer[1][0]); break;
						case 2: DisplayCam2(pImageColorDestBuffer[2][0]); break;
						case 3: DisplayCam3(pImageColorDestBuffer[3][0]); break;
						}
					}

					bLiveFlag[m_iCameraIndex] = true;
				}
			}
		}
	}
}

// ============================================================================
// AllocImageBuf - MultiByteToWideChar 버전
// ============================================================================

void CPylonSampleProgramDlg::AllocImageBuf(void)
{
	UpdateData();

	TRACE(_T("=== AllocImageBuf START ===\n"));
	TRACE(_T("Camera Index: %d\n"), m_iCameraIndex);

	// ========================================
	// std::string → CString 변환 (MultiByteToWideChar)
	// ========================================
	// m_strCM_ImageForamt가 이미 CString이므로 그냥 대입!
	CString strFormat = m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex];

	TRACE(_T("Format: %s\n"), (LPCTSTR)strFormat);

	TRACE(_T("Size: %dx%d\n"),
		m_CameraManager.m_iCM_Width[m_iCameraIndex],
		m_CameraManager.m_iCM_Height[m_iCameraIndex]);

	// ========================================
	// 기존 버퍼 해제
	// ========================================
	if (pImageresizeOrgBuffer[m_iCameraIndex] != NULL)
	{
		TRACE(_T("Freeing existing Mono buffer\n"));
		for (int i = 0; i < BUF_NUM; i++)
		{
			if (pImageresizeOrgBuffer[m_iCameraIndex][i] != NULL)
			{
				free(pImageresizeOrgBuffer[m_iCameraIndex][i]);
				pImageresizeOrgBuffer[m_iCameraIndex][i] = NULL;
			}
		}
		free(pImageresizeOrgBuffer[m_iCameraIndex]);
		pImageresizeOrgBuffer[m_iCameraIndex] = NULL;
	}

	if (pImageColorDestBuffer[m_iCameraIndex] != NULL)
	{
		TRACE(_T("Freeing existing Color buffer\n"));
		for (int i = 0; i < BUF_NUM; i++)
		{
			if (pImageColorDestBuffer[m_iCameraIndex][i] != NULL)
			{
				free(pImageColorDestBuffer[m_iCameraIndex][i]);
				pImageColorDestBuffer[m_iCameraIndex][i] = NULL;
			}
		}
		free(pImageColorDestBuffer[m_iCameraIndex]);
		pImageColorDestBuffer[m_iCameraIndex] = NULL;
	}

	// ========================================
	// 크기 검증
	// ========================================
	int width = m_CameraManager.m_iCM_Width[m_iCameraIndex];
	int height = m_CameraManager.m_iCM_Height[m_iCameraIndex];
	int reSizeWidth = m_CameraManager.m_iCM_reSizeWidth[m_iCameraIndex];

	if (width <= 0 || height <= 0 || reSizeWidth <= 0)
	{
		CString err;
		err.Format(_T("Invalid image size!\nWidth: %d, Height: %d, reSizeWidth: %d"),
			width, height, reSizeWidth);
		AfxMessageBox(err);
		TRACE(_T("ERROR: Invalid size\n"));
		return;
	}

	if (width > 10000 || height > 10000 || reSizeWidth > 10000)
	{
		AfxMessageBox(_T("Image size too large!"));
		TRACE(_T("ERROR: Size too large\n"));
		return;
	}

	// width가 reSizeWidth보다 작거나 같아야 함
	if (width > reSizeWidth)
	{
		CString err;
		err.Format(_T("Width(%d) cannot be larger than reSizeWidth(%d)!"), width, reSizeWidth);
		AfxMessageBox(err);
		return;
	}

	// ========================================
	// 새 버퍼 할당
	// ========================================

	if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono8")
	{
		TRACE(_T("Allocating Mono8 buffers...\n"));

		pImageresizeOrgBuffer[m_iCameraIndex] = (unsigned char**)calloc(BUF_NUM, sizeof(unsigned char*));

		if (pImageresizeOrgBuffer[m_iCameraIndex] == NULL)
		{
			AfxMessageBox(_T("Memory allocation failed!"));
			TRACE(_T("ERROR: Failed to allocate pointer array\n"));
			return;
		}

		size_t bufSize = (size_t)reSizeWidth * (size_t)height;
		TRACE(_T("Buffer size: %zu bytes\n"), bufSize);

		for (int i = 0; i < BUF_NUM; i++)
		{
			pImageresizeOrgBuffer[m_iCameraIndex][i] = (unsigned char*)calloc(bufSize, sizeof(unsigned char));

			if (pImageresizeOrgBuffer[m_iCameraIndex][i] == NULL)
			{
				CString err;
				err.Format(_T("Memory allocation failed! (Buffer %d/%d)"), i + 1, BUF_NUM);
				AfxMessageBox(err);
				TRACE(_T("ERROR: Failed to allocate buffer %d\n"), i);

				// 이미 할당된 버퍼들 해제
				for (int j = 0; j < i; j++)
				{
					free(pImageresizeOrgBuffer[m_iCameraIndex][j]);
					pImageresizeOrgBuffer[m_iCameraIndex][j] = NULL;
				}
				free(pImageresizeOrgBuffer[m_iCameraIndex]);
				pImageresizeOrgBuffer[m_iCameraIndex] = NULL;
				return;
			}

			TRACE(_T("Buffer[%d] allocated at: 0x%p\n"), i, pImageresizeOrgBuffer[m_iCameraIndex][i]);
		}

		TRACE(_T("Mono8 buffers allocated successfully\n"));
	}
	else if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono12" ||
		m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono16")
	{
		TRACE(_T("Allocating Mono12/16 buffers...\n"));

		pImageresizeOrgBuffer[m_iCameraIndex] = (unsigned char**)calloc(BUF_NUM, sizeof(unsigned char*));

		if (pImageresizeOrgBuffer[m_iCameraIndex] == NULL)
		{
			AfxMessageBox(_T("Memory allocation failed!"));
			return;
		}

		size_t bufSize = (size_t)reSizeWidth * (size_t)height;

		for (int i = 0; i < BUF_NUM; i++)
		{
			pImageresizeOrgBuffer[m_iCameraIndex][i] = (unsigned char*)calloc(bufSize, sizeof(unsigned char));

			if (pImageresizeOrgBuffer[m_iCameraIndex][i] == NULL)
			{
				AfxMessageBox(_T("Memory allocation failed!"));

				// 이미 할당된 버퍼들 해제
				for (int j = 0; j < i; j++)
				{
					free(pImageresizeOrgBuffer[m_iCameraIndex][j]);
					pImageresizeOrgBuffer[m_iCameraIndex][j] = NULL;
				}
				free(pImageresizeOrgBuffer[m_iCameraIndex]);
				pImageresizeOrgBuffer[m_iCameraIndex] = NULL;
				return;
			}
		}

		TRACE(_T("Mono12/16 buffers allocated successfully\n"));
	}
	else if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "YUV422Packed")
	{
		TRACE(_T("Allocating YUV422 buffers...\n"));

		pImageColorDestBuffer[m_iCameraIndex] = (unsigned char**)calloc(BUF_NUM, sizeof(unsigned char*));

		if (pImageColorDestBuffer[m_iCameraIndex] == NULL)
		{
			AfxMessageBox(_T("Memory allocation failed!"));
			return;
		}

		size_t bufSize = (size_t)reSizeWidth * (size_t)height * 3;

		for (int i = 0; i < BUF_NUM; i++)
		{
			pImageColorDestBuffer[m_iCameraIndex][i] = (unsigned char*)calloc(bufSize, sizeof(unsigned char));

			if (pImageColorDestBuffer[m_iCameraIndex][i] == NULL)
			{
				AfxMessageBox(_T("Memory allocation failed!"));

				// 이미 할당된 버퍼들 해제
				for (int j = 0; j < i; j++)
				{
					free(pImageColorDestBuffer[m_iCameraIndex][j]);
					pImageColorDestBuffer[m_iCameraIndex][j] = NULL;
				}
				free(pImageColorDestBuffer[m_iCameraIndex]);
				pImageColorDestBuffer[m_iCameraIndex] = NULL;
				return;
			}
		}

		TRACE(_T("YUV422 buffers allocated successfully\n"));
	}
	else  // Bayer Color
	{
		TRACE(_T("Allocating Bayer Color buffers...\n"));

		pImageresizeOrgBuffer[m_iCameraIndex] = (unsigned char**)calloc(BUF_NUM, sizeof(unsigned char*));
		pImageColorDestBuffer[m_iCameraIndex] = (unsigned char**)calloc(BUF_NUM, sizeof(unsigned char*));

		if (pImageresizeOrgBuffer[m_iCameraIndex] == NULL ||
			pImageColorDestBuffer[m_iCameraIndex] == NULL)
		{
			AfxMessageBox(_T("Memory allocation failed!"));
			return;
		}

		size_t bufSize = (size_t)reSizeWidth * (size_t)height;

		for (int i = 0; i < BUF_NUM; i++)
		{
			pImageresizeOrgBuffer[m_iCameraIndex][i] = (unsigned char*)calloc(bufSize, sizeof(unsigned char));
			pImageColorDestBuffer[m_iCameraIndex][i] = (unsigned char*)calloc(bufSize * 3, sizeof(unsigned char));

			if (pImageresizeOrgBuffer[m_iCameraIndex][i] == NULL ||
				pImageColorDestBuffer[m_iCameraIndex][i] == NULL)
			{
				AfxMessageBox(_T("Memory allocation failed!"));

				// 이미 할당된 버퍼들 해제
				if (pImageresizeOrgBuffer[m_iCameraIndex][i])
				{
					free(pImageresizeOrgBuffer[m_iCameraIndex][i]);
				}
				if (pImageColorDestBuffer[m_iCameraIndex][i])
				{
					free(pImageColorDestBuffer[m_iCameraIndex][i]);
				}
				for (int j = 0; j < i; j++)
				{
					free(pImageresizeOrgBuffer[m_iCameraIndex][j]);
					free(pImageColorDestBuffer[m_iCameraIndex][j]);
				}
				free(pImageresizeOrgBuffer[m_iCameraIndex]);
				free(pImageColorDestBuffer[m_iCameraIndex]);
				pImageresizeOrgBuffer[m_iCameraIndex] = NULL;
				pImageColorDestBuffer[m_iCameraIndex] = NULL;
				return;
			}
		}

		TRACE(_T("Bayer Color buffers allocated successfully\n"));
	}

	// ========================================
	// 디스플레이 핸들 설정
	// ========================================
	switch (m_iCameraIndex)
	{
	case 0:
		hWnd[0] = GetDlgItem(IDC_CAM0_DISPLAY)->GetSafeHwnd();
		GetDlgItem(IDC_CAM0_DISPLAY)->GetClientRect(&rectStaticClient[0]);
		hdc[0] = ::GetDC(hWnd[0]);
		TRACE(_T("Display handle set for Camera 0\n"));
		break;
	case 1:
		hWnd[1] = GetDlgItem(IDC_CAM1_DISPLAY)->GetSafeHwnd();
		GetDlgItem(IDC_CAM1_DISPLAY)->GetClientRect(&rectStaticClient[1]);
		hdc[1] = ::GetDC(hWnd[1]);
		TRACE(_T("Display handle set for Camera 1\n"));
		break;
	case 2:
		hWnd[2] = GetDlgItem(IDC_CAM2_DISPLAY)->GetSafeHwnd();
		GetDlgItem(IDC_CAM2_DISPLAY)->GetClientRect(&rectStaticClient[2]);
		hdc[2] = ::GetDC(hWnd[2]);
		TRACE(_T("Display handle set for Camera 2\n"));
		break;
	case 3:
		hWnd[3] = GetDlgItem(IDC_CAM3_DISPLAY)->GetSafeHwnd();
		GetDlgItem(IDC_CAM3_DISPLAY)->GetClientRect(&rectStaticClient[3]);
		hdc[3] = ::GetDC(hWnd[3]);
		TRACE(_T("Display handle set for Camera 3\n"));
		break;
	}

	TRACE(_T("=== AllocImageBuf END (SUCCESS) ===\n"));
}

//==========================================================

void CPylonSampleProgramDlg::InitBitmap(int nCamIndex)
{
	if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono8" ||
		m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono16")
	{
		if (bitmapinfo[nCamIndex])
		{
			delete bitmapinfo[nCamIndex];
			bitmapinfo[nCamIndex] = NULL;
		}

		bitmapinfo[nCamIndex] = (BITMAPINFO*)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)]);

		bitmapinfo[nCamIndex]->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo[nCamIndex]->bmiHeader.biWidth = (int)m_CameraManager.m_iCM_reSizeWidth[nCamIndex];
		bitmapinfo[nCamIndex]->bmiHeader.biHeight = -(int)m_CameraManager.m_iCM_Height[nCamIndex];
		bitmapinfo[nCamIndex]->bmiHeader.biPlanes = 1;
		bitmapinfo[nCamIndex]->bmiHeader.biCompression = BI_RGB;
		bitmapinfo[nCamIndex]->bmiHeader.biBitCount = 8;
		bitmapinfo[nCamIndex]->bmiHeader.biSizeImage = (int)(m_CameraManager.m_iCM_reSizeWidth[nCamIndex] * m_CameraManager.m_iCM_Height[nCamIndex]);
		bitmapinfo[nCamIndex]->bmiHeader.biXPelsPerMeter = 0;
		bitmapinfo[nCamIndex]->bmiHeader.biYPelsPerMeter = 0;
		bitmapinfo[nCamIndex]->bmiHeader.biClrUsed = 256;
		bitmapinfo[nCamIndex]->bmiHeader.biClrImportant = 0;

		for (int j = 0; j < 256; j++)
		{
			bitmapinfo[nCamIndex]->bmiColors[j].rgbRed = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbGreen = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbBlue = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbReserved = 0;
		}
	}
	else
	{
		if (bitmapinfo[nCamIndex])
		{
			delete bitmapinfo[nCamIndex];
			bitmapinfo[nCamIndex] = NULL;
		}

		bitmapinfo[nCamIndex] = (BITMAPINFO*)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)]);

		bitmapinfo[nCamIndex]->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo[nCamIndex]->bmiHeader.biWidth = (int)m_CameraManager.m_iCM_reSizeWidth[nCamIndex];
		bitmapinfo[nCamIndex]->bmiHeader.biHeight = -(int)m_CameraManager.m_iCM_Height[nCamIndex];
		bitmapinfo[nCamIndex]->bmiHeader.biPlanes = 1;
		bitmapinfo[nCamIndex]->bmiHeader.biCompression = BI_RGB;
		bitmapinfo[nCamIndex]->bmiHeader.biBitCount = 24;
		bitmapinfo[nCamIndex]->bmiHeader.biSizeImage = (int)m_CameraManager.m_iCM_reSizeWidth[nCamIndex] * (int)m_CameraManager.m_iCM_Height[nCamIndex] * 3;
		bitmapinfo[nCamIndex]->bmiHeader.biXPelsPerMeter = 0;
		bitmapinfo[nCamIndex]->bmiHeader.biYPelsPerMeter = 0;
		bitmapinfo[nCamIndex]->bmiHeader.biClrUsed = 256;
		bitmapinfo[nCamIndex]->bmiHeader.biClrImportant = 0;

		for (int j = 0; j < 256; j++)
		{
			bitmapinfo[nCamIndex]->bmiColors[j].rgbRed = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbGreen = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbBlue = (unsigned char)j;
			bitmapinfo[nCamIndex]->bmiColors[j].rgbReserved = 0;
		}
	}
}

void CPylonSampleProgramDlg::DisplayCam0(void* pImageBuf)
{
	SetStretchBltMode(hdc[0], COLORONCOLOR);
	StretchDIBits(
		hdc[0], 0, 0,
		rectStaticClient[0].Width(), rectStaticClient[0].Height(),
		0, 0,
		(int)m_CameraManager.m_iCM_reSizeWidth[0], (int)m_CameraManager.m_iCM_Height[0],
		pImageBuf, bitmapinfo[0], DIB_RGB_COLORS, SRCCOPY
	);
}

void CPylonSampleProgramDlg::DisplayCam1(void* pImageBuf)
{
	SetStretchBltMode(hdc[1], COLORONCOLOR);
	StretchDIBits(
		hdc[1], 0, 0,
		rectStaticClient[1].Width(), rectStaticClient[1].Height(),
		0, 0,
		(int)m_CameraManager.m_iCM_reSizeWidth[1], (int)m_CameraManager.m_iCM_Height[1],
		pImageBuf, bitmapinfo[1], DIB_RGB_COLORS, SRCCOPY
	);
}

void CPylonSampleProgramDlg::DisplayCam2(void* pImageBuf)
{
	SetStretchBltMode(hdc[2], COLORONCOLOR);
	StretchDIBits(
		hdc[2], 0, 0,
		rectStaticClient[2].Width(), rectStaticClient[2].Height(),
		0, 0,
		(int)m_CameraManager.m_iCM_reSizeWidth[2], (int)m_CameraManager.m_iCM_Height[2],
		pImageBuf, bitmapinfo[2], DIB_RGB_COLORS, SRCCOPY
	);
}

void CPylonSampleProgramDlg::DisplayCam3(void* pImageBuf)
{
	SetStretchBltMode(hdc[3], COLORONCOLOR);
	StretchDIBits(
		hdc[3], 0, 0,
		rectStaticClient[3].Width(), rectStaticClient[3].Height(),
		0, 0,
		(int)m_CameraManager.m_iCM_reSizeWidth[3], (int)m_CameraManager.m_iCM_Height[3],
		pImageBuf, bitmapinfo[3], DIB_RGB_COLORS, SRCCOPY
	);
}

// ============================================================================
// 라이브 뷰 함수들
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedCam0Live()
{
	if (m_CameraManager.m_bCamConnectFlag[0] == true)
	{
		bStopThread[0] = (bStopThread[0] + 1) & 0x01;

		if (bStopThread[0])
		{
			bLiveFlag[0] = true;
			m_CameraManager.GrabLive(0, 0);
			SetDlgItemText(IDC_CAM0_LIVE, _T("Live_Cam0_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[0]);
		}
		else
		{
			bLiveFlag[0] = false;
			SetDlgItemText(IDC_CAM0_LIVE, _T("Live_Cam0_Start"));
			m_CameraManager.LiveStop(0, 0);
		}
	}
	else
	{
		AfxMessageBox(_T("Camera0 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM0_LIVE);
		pButton->SetCheck(0);
	}
}

void CPylonSampleProgramDlg::OnBnClickedCam1Live()
{
	if (m_CameraManager.m_bCamConnectFlag[1] == true)
	{
		bStopThread[1] = (bStopThread[1] + 1) & 0x01;

		if (bStopThread[1])
		{
			bLiveFlag[1] = true;
			m_CameraManager.GrabLive(1, 0);
			SetDlgItemText(IDC_CAM1_LIVE, _T("Live_Cam1_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[1]);
		}
		else
		{
			bLiveFlag[1] = false;
			SetDlgItemText(IDC_CAM1_LIVE, _T("Live_Cam1_Start"));
			m_CameraManager.LiveStop(1, 0);
		}
	}
	else
	{
		AfxMessageBox(_T("Camera1 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM1_LIVE);
		pButton->SetCheck(0);
	}
}

void CPylonSampleProgramDlg::OnBnClickedCam2Live()
{
	if (m_CameraManager.m_bCamConnectFlag[2] == true)
	{
		bStopThread[2] = (bStopThread[2] + 1) & 0x01;

		if (bStopThread[2])
		{
			bLiveFlag[2] = true;
			m_CameraManager.GrabLive(2, 0);
			SetDlgItemText(IDC_CAM2_LIVE, _T("Live_Cam2_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[2]);
		}
		else
		{
			bLiveFlag[2] = false;
			SetDlgItemText(IDC_CAM2_LIVE, _T("Live_Cam2_Start"));
			m_CameraManager.LiveStop(2, 0);
		}
	}
	else
	{
		AfxMessageBox(_T("Camera2 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM2_LIVE);
		pButton->SetCheck(0);
	}
}

void CPylonSampleProgramDlg::OnBnClickedCam3Live()
{
	if (m_CameraManager.m_bCamConnectFlag[3] == true)
	{
		bStopThread[3] = (bStopThread[3] + 1) & 0x01;

		if (bStopThread[3])
		{
			bLiveFlag[3] = true;
			m_CameraManager.GrabLive(3, 0);
			SetDlgItemText(IDC_CAM3_LIVE, _T("Live_Cam3_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[3]);
		}
		else
		{
			bLiveFlag[3] = false;
			SetDlgItemText(IDC_CAM3_LIVE, _T("Live_Cam3_Start"));
			m_CameraManager.LiveStop(3, 0);
		}
	}
	else
	{
		AfxMessageBox(_T("Camera3 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM3_LIVE);
		pButton->SetCheck(0);
	}
}

void CPylonSampleProgramDlg::OnBnClickedTwoCameraLiveBtn()
{
	if (m_CameraManager.m_bCamConnectFlag[0] == true)
	{
		bStopThread[0] = (bStopThread[0] + 1) & 0x01;

		if (bStopThread[0])
		{
			bLiveFlag[0] = true;
			m_CameraManager.GrabLive(0, 1);
			SetDlgItemText(IDC_CAM0_LIVE, _T("Live_Cam0_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[0]);
		}
		else
		{
			bLiveFlag[0] = false;
			SetDlgItemText(IDC_CAM0_LIVE, _T("Live_Cam0_Start"));
			m_CameraManager.LiveStop(0, 1);
		}
	}
	else
	{
		AfxMessageBox(_T("Camera0 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM0_LIVE);
		pButton->SetCheck(0);
	}

	if (m_CameraManager.m_bCamConnectFlag[1] == true)
	{
		bStopThread[1] = (bStopThread[1] + 1) & 0x01;

		if (bStopThread[1])
		{
			bLiveFlag[1] = true;
			SetDlgItemText(IDC_CAM1_LIVE, _T("Live_Cam1_Stop"));
			AfxBeginThread(LiveGrabThreadCam0, &m_nCamIndexBuf[1]);
		}
		else
		{
			bLiveFlag[1] = false;
			SetDlgItemText(IDC_CAM1_LIVE, _T("Live_Cam1_Start"));
		}
	}
	else
	{
		AfxMessageBox(_T("Camera1 Connect를 하세요!!"));
		CButton* pButton = (CButton*)GetDlgItem(IDC_CAM1_LIVE);
		pButton->SetCheck(0);
	}
}

void CPylonSampleProgramDlg::OnBnClickedSoftTrigBtn()
{
	m_CameraManager.SetCommand(m_iCameraIndex, "TriggerSoftware");
}

void CPylonSampleProgramDlg::OnBnClickedButton5()
{
	static bool bTrig = false;

	if (bTrig == false)
	{
		SetDlgItemText(IDC_BUTTON5, _T("Trigger_해제"));
		bTrig = true;

		m_CameraManager.SetEnumeration(m_iCameraIndex, "FrameStart", "TriggerSelector");
		m_CameraManager.SetEnumeration(m_iCameraIndex, "On", "TriggerMode");
		m_CameraManager.SetEnumeration(m_iCameraIndex, "Software", "TriggerSource");
	}
	else
	{
		SetDlgItemText(IDC_BUTTON5, _T("Trigger_설정"));
		bTrig = false;

		m_CameraManager.SetEnumeration(m_iCameraIndex, "Off", "TriggerMode");
	}
}

void CPylonSampleProgramDlg::OnBnClickedSaveImgBtn()
{
	CString t = _T("test1.bmp");

	if (m_CameraManager.m_strCM_ImageForamt[m_iCameraIndex] == "Mono8")
	{
		m_CameraManager.SaveImage(
			0,
			m_CameraManager.pImage8Buffer[m_iCameraIndex],
			CT2A(t),
			0,
			m_CameraManager.m_iCM_Width[m_iCameraIndex],
			m_CameraManager.m_iCM_Height[m_iCameraIndex],
			1
		);
	}
	else
	{
		m_CameraManager.SaveImage(
			0,
			m_CameraManager.pImage24Buffer[m_iCameraIndex],
			CT2A(t),
			1,
			m_CameraManager.m_iCM_Width[m_iCameraIndex],
			m_CameraManager.m_iCM_Height[m_iCameraIndex],
			3
		);
	}
}

// ============================================================================
// 서버 연결 및 AI 검사
// ============================================================================

// ============================================================================
// OnBnClickedConnectServerBtn - 수정
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedConnectServerBtn()
{
	GetDlgItemText(IDC_SERVER_IP_EDIT, m_strServerIP);
	m_nServerPort = GetDlgItemInt(IDC_SERVER_PORT_EDIT);

	CString strIP_Ansi(m_strServerIP);
	CT2A pszConvertedAnsi(strIP_Ansi);
	std::string strIP(pszConvertedAnsi);

	if (m_tcpClient.Connect(strIP.c_str(), m_nServerPort))
	{
		m_bConnectedToServer = true;
		AfxMessageBox(_T("서버 연결 성공!"));
		GetDlgItem(IDC_CONNECT_SERVER_BTN)->SetWindowText(_T("연결됨"));
		GetDlgItem(IDC_SEND_TO_AI_BTN)->EnableWindow(TRUE);
	}
	else
	{
		m_bConnectedToServer = false;

		// ⭐ 수정: std::string → CString 변환
		CString strError = StdStringToCString(m_tcpClient.GetLastError());
		AfxMessageBox(_T("서버 연결 실패: ") + strError);
	}
}

// ============================================================================
// OnBnClickedSendToAiBtn - Live 중지 추가
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedSendToAiBtn()
{
	TRACE(_T("\n========================================\n"));
	TRACE(_T("OnBnClickedSendToAiBtn START\n"));
	TRACE(_T("========================================\n"));

	if (!m_bConnectedToServer)
	{
		TRACE(_T("ERROR: Not connected to server\n"));
		AfxMessageBox(_T("서버에 연결되지 않았습니다!"));
		return;
	}

	int camIndex = m_iCameraIndex >= 0 ? m_iCameraIndex : 0;
	TRACE(_T("Camera Index: %d\n"), camIndex);

	// ⭐ Live가 실행 중이면 경고
	if (bLiveFlag[camIndex] == true)
	{
		TRACE(_T("WARNING: Live is running! Stopping Live...\n"));

		AfxMessageBox(_T("라이브 뷰가 실행 중입니다!\nAI 검사를 위해 잠시 중지합니다."));

		// Live 중지
		bStopThread[camIndex] = false;
		bLiveFlag[camIndex] = false;
		m_CameraManager.LiveStop(camIndex, 0);

		Sleep(500);  // 스레드 종료 대기

		TRACE(_T("Live stopped\n"));
	}

	CString tempFilePath = _T("temp_ai_test.bmp");

	TRACE(_T("Checking image buffers...\n"));
	TRACE(_T("pImage24Buffer[%d]: %p\n"), camIndex, m_CameraManager.pImage24Buffer[camIndex]);
	TRACE(_T("pImage8Buffer[%d]: %p\n"), camIndex, m_CameraManager.pImage8Buffer[camIndex]);

	if (m_CameraManager.pImage24Buffer[camIndex] != NULL ||
		m_CameraManager.pImage8Buffer[camIndex] != NULL)
	{
		int width = m_CameraManager.m_iCM_Width[camIndex];
		int height = m_CameraManager.m_iCM_Height[camIndex];

		TRACE(_T("Image size: %dx%d\n"), width, height);

		if (width <= 0 || height <= 0)
		{
			TRACE(_T("ERROR: Invalid image size!\n"));
			AfxMessageBox(_T("이미지 크기 정보가 없습니다!"));
			return;
		}

		// ⭐ Single Grab으로 새 이미지 캡처
		TRACE(_T("Capturing new image with SingleGrab...\n"));
		if (m_CameraManager.SingleGrab(camIndex) == 0)
		{
			// 캡처 완료 대기
			int timeout = 0;
			while (!m_CameraManager.CheckCaptureEnd(camIndex))
			{
				Sleep(10);
				timeout++;
				if (timeout > 300) {  // 3초 타임아웃
					TRACE(_T("ERROR: Capture timeout!\n"));
					AfxMessageBox(_T("이미지 캡처 타임아웃!"));
					return;
				}
			}
			TRACE(_T("Image captured successfully\n"));
		}

		char filePathA[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, tempFilePath, -1, filePathA, MAX_PATH, NULL, NULL);
		TRACE(_T("Temp file path: %S\n"), filePathA);

		int result = -1;

		TRACE(_T("Image format: %s\n"), m_CameraManager.m_strCM_ImageForamt[camIndex]);

		if (m_CameraManager.m_strCM_ImageForamt[camIndex] == _T("Mono8"))
		{
			TRACE(_T("Saving Mono8 image...\n"));
			result = m_CameraManager.SaveImage(
				0,
				m_CameraManager.pImage8Buffer[camIndex],
				filePathA,
				0,
				width,
				height,
				1
			);
		}
		else
		{
			TRACE(_T("Saving Color image...\n"));
			result = m_CameraManager.SaveImage(
				0,
				m_CameraManager.pImage24Buffer[camIndex],
				filePathA,
				1,
				width,
				height,
				3
			);
		}

		TRACE(_T("SaveImage result: %d\n"), result);

		if (result != 0)
		{
			TRACE(_T("ERROR: Image save failed!\n"));
			AfxMessageBox(_T("이미지 저장 실패!"));
			return;
		}

		TRACE(_T("Opening temp file...\n"));
		FILE* fp = NULL;
		_wfopen_s(&fp, tempFilePath, L"rb");
		if (fp == NULL)
		{
			TRACE(_T("ERROR: Cannot open temp file!\n"));
			AfxMessageBox(_T("임시 파일 열기 실패!"));
			return;
		}

		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		TRACE(_T("File size: %ld bytes\n"), fileSize);

		if (fileSize <= 0 || fileSize > 100 * 1024 * 1024)
		{
			TRACE(_T("ERROR: Invalid file size: %ld\n"), fileSize);
			fclose(fp);
			AfxMessageBox(_T("파일 크기가 비정상적입니다!"));
			return;
		}

		TRACE(_T("Allocating %ld bytes...\n"), fileSize);
		unsigned char* fileData = NULL;

		try
		{
			fileData = new unsigned char[fileSize];
		}
		catch (std::bad_alloc&)
		{
			TRACE(_T("ERROR: Memory allocation failed!\n"));
			fclose(fp);
			AfxMessageBox(_T("메모리 할당 실패!"));
			return;
		}

		if (fileData == NULL)
		{
			TRACE(_T("ERROR: fileData is NULL!\n"));
			fclose(fp);
			AfxMessageBox(_T("메모리 할당 실패!"));
			return;
		}

		TRACE(_T("Memory allocated at: %p\n"), fileData);

		TRACE(_T("Reading file...\n"));
		size_t bytesRead = fread(fileData, 1, fileSize, fp);
		fclose(fp);

		TRACE(_T("Bytes read: %zu / %ld\n"), bytesRead, fileSize);

		if (bytesRead != fileSize)
		{
			TRACE(_T("ERROR: File read incomplete!\n"));
			delete[] fileData;
			AfxMessageBox(_T("파일 읽기 실패!"));
			return;
		}

		if (fileSize >= 2 && fileData[0] == 'B' && fileData[1] == 'M')
		{
			TRACE(_T("BMP file signature verified: BM\n"));
		}

		TRACE(_T("Calling SendImageAndReceiveResult...\n"));
		TRACE(_T("  fileData: %p\n"), fileData);
		TRACE(_T("  width: %d\n"), width);
		TRACE(_T("  height: %d\n"), height);

		AIResult aiResult;
		bool success = false;

		try
		{
			TRACE(_T("Entering SendImageAndReceiveResult...\n"));
			success = m_tcpClient.SendImageAndReceiveResult(
				fileData,
				width,
				height,
				aiResult
			);
			TRACE(_T("SendImageAndReceiveResult returned: %d\n"), success);
		}
		catch (...)
		{
			TRACE(_T("EXCEPTION in SendImageAndReceiveResult!\n"));
			success = false;
		}

		TRACE(_T("Deleting fileData...\n"));
		delete[] fileData;
		fileData = NULL;
		TRACE(_T("fileData deleted\n"));

		DeleteFile(tempFilePath);

		if (success)
		{
			TRACE(_T("AI검사 성공! Calling DisplayAIResult...\n"));
			TRACE(_T("result.classification: '%S' (len=%zu)\n"),
				aiResult.classification.c_str(), aiResult.classification.length());
			TRACE(_T("result.result: '%S' (len=%zu)\n"),
				aiResult.result.c_str(), aiResult.result.length());
			TRACE(_T("result.confidence: %.2f\n"), aiResult.confidence);
			TRACE(_T("result.is_defect: %d\n"), aiResult.is_defect);

			DisplayAIResult(aiResult);

			TRACE(_T("DisplayAIResult returned\n"));
		}
		else
		{
			TRACE(_T("AI 검사 실패!\n"));

			std::string stdError = m_tcpClient.GetLastError();
			TRACE(_T("Error string: '%S' (len=%zu)\n"), stdError.c_str(), stdError.length());

			CString strError = StdStringToCString(stdError);

			AfxMessageBox(_T("AI 검사 실패: ") + strError);
		}
	}
	else
	{
		TRACE(_T("ERROR: No image buffer!\n"));
		AfxMessageBox(_T("이미지 버퍼가 없습니다!\n\n1. 카메라 연결\n2. Grab_Single 클릭\n3. 다시 시도"));
	}

	TRACE(_T("========================================\n"));
	TRACE(_T("OnBnClickedSendToAiBtn END\n"));
	TRACE(_T("========================================\n\n"));
}

// ============================================================================
//  안전한 DisplayAIResult 함수 
// ============================================================================

void CPylonSampleProgramDlg::DisplayAIResult(const AIResult& result)
{
	TRACE(_T("\n========================================\n"));
	TRACE(_T("DisplayAIResult START\n"));
	TRACE(_T("========================================\n"));

	TRACE(_T("Calling UpdateStatistics...\n"));
	UpdateStatistics(result);
	TRACE(_T("UpdateStatistics returned\n"));

	// 안전한 변환
	TRACE(_T("Converting classification...\n"));
	TRACE(_T("  classification.length() = %zu\n"), result.classification.length());
	TRACE(_T("  classification.empty() = %d\n"), result.classification.empty());
	TRACE(_T("  classification.c_str() = '%S'\n"), result.classification.c_str());

	CString strClassification = StdStringToCString(result.classification);
	TRACE(_T("  -> CString: '%s'\n"), (LPCTSTR)strClassification);

	TRACE(_T("Converting result...\n"));
	TRACE(_T("  result.length() = %zu\n"), result.result.length());
	TRACE(_T("  result.empty() = %d\n"), result.result.empty());
	TRACE(_T("  result.c_str() = '%S'\n"), result.result.c_str());

	CString strResultText = StdStringToCString(result.result);
	TRACE(_T("  -> CString: '%s'\n"), (LPCTSTR)strResultText);

	TRACE(_T("Conversion completed!\n"));

	// Format 생성
	TRACE(_T("Creating Format string...\n"));

	CString strOutput;
	strOutput.Format(
		_T("========== AI 검사 결과 ==========\n")
		_T("분류: %s\n")
		_T("결과: %s\n")
		_T("신뢰도: %.1f%%\n")
		_T("불량 여부: %s\n\n")
		_T("[ 금일 누적 ]\n")
		_T("총 검사: %d회\n")
		_T("불량: %d회 (%.1f%%)\n")
		_T("================================"),
		strClassification,
		strResultText,
		result.confidence * 100,
		result.is_defect ? _T("불량") : _T("정상"),
		m_todayStats.totalCount,
		m_todayStats.defectCount,
		m_todayStats.defectRate
	);

	TRACE(_T("Format string created successfully\n"));
	TRACE(_T("Output message:\n%s\n"), (LPCTSTR)strOutput);

	// 자동 검사 모드가 아닐 때만 메시지 박스
	if (!m_bAutoInspectionMode)
	{
		TRACE(_T("Showing MessageBox...\n"));
		AfxMessageBox(strOutput);
		TRACE(_T("MessageBox closed\n"));
	}

	TRACE(_T("Calling SetDlgItemText...\n"));
	SetDlgItemText(IDC_AI_RESULT_STATIC, strOutput);
	TRACE(_T("SetDlgItemText completed\n"));

	// 배치 정보 업데이트
	if (!m_strCurrentBatch.IsEmpty())
	{
		TRACE(_T("Calling UpdateBatchInfo...\n"));
		UpdateBatchInfo();
		TRACE(_T("UpdateBatchInfo completed\n"));
	}

	TRACE(_T("========================================\n"));
	TRACE(_T("DisplayAIResult END\n"));
	TRACE(_T("========================================\n\n"));
}

//============================================================================


void CPylonSampleProgramDlg::OnStnClickedAiResultStatic()
{
	// 추가 기능 필요 시 구현
}

// ============================================================================
// 자동 검사 기능
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedAutoInspectionStart()
{
	if (m_bAutoInspectionMode)
	{
		// 이미 실행 중이면 중지
		OnBnClickedAutoInspectionStop();
		return;
	}

	if (m_iCameraIndex < 0 || !m_CameraManager.m_bCamConnectFlag[m_iCameraIndex])
	{
		AfxMessageBox(_T("카메라를 먼저 연결하세요!"));
		return;
	}

	if (!m_bConnectedToServer)
	{
		AfxMessageBox(_T("서버에 먼저 연결하세요!"));
		return;
	}

	CString msg;
	msg.Format(
		_T("자동 검사를 시작하시겠습니까?\n\n")
		_T("검사 횟수: %d회\n")
		_T("검사 간격: %.1f초"),
		m_nAutoInspectionCount,
		m_nAutoInspectionInterval / 1000.0
	);

	if (AfxMessageBox(msg, MB_YESNO) != IDYES)
	{
		return;
	}

	m_bAutoInspectionMode = true;
	m_nAutoInspectionCurrent = 0;

	GetDlgItem(IDC_AUTO_INSPECTION_BTN)->SetWindowText(_T("자동 검사 중지"));

	m_pAutoInspectionThread = AfxBeginThread(AutoInspectionThread, this);
}

void CPylonSampleProgramDlg::OnBnClickedAutoInspectionStop()
{
	if (!m_bAutoInspectionMode)
	{
		return;
	}

	m_bAutoInspectionMode = false;

	if (m_pAutoInspectionThread)
	{
		WaitForSingleObject(m_pAutoInspectionThread->m_hThread, 5000);
		m_pAutoInspectionThread = nullptr;
	}

	GetDlgItem(IDC_AUTO_INSPECTION_BTN)->SetWindowText(_T("자동 검사 시작"));

	CString msg;
	msg.Format(
		_T("자동 검사 완료!\n\n")
		_T("검사 수: %d회\n")
		_T("불량: %d회\n")
		_T("정상: %d회\n")
		_T("불량률: %.2f%%"),
		m_todayStats.totalCount,
		m_todayStats.defectCount,
		m_todayStats.normalCount,
		m_todayStats.defectRate
	);

	AfxMessageBox(msg);
}

UINT CPylonSampleProgramDlg::AutoInspectionThread(LPVOID pParam)
{
	CPylonSampleProgramDlg* pDlg = (CPylonSampleProgramDlg*)pParam;

	while (pDlg->m_bAutoInspectionMode &&
		pDlg->m_nAutoInspectionCurrent < pDlg->m_nAutoInspectionCount)
	{
		if (pDlg->m_CameraManager.SingleGrab(pDlg->m_iCameraIndex) == 0)
		{
			int timeout = 0;
			while (!pDlg->m_CameraManager.CheckCaptureEnd(pDlg->m_iCameraIndex))
			{
				Sleep(10);
				timeout++;
				if (timeout > 500) break;
			}

			if (timeout > 500)
			{
				continue;
			}

			pDlg->OnBnClickedSendToAiBtn();

			pDlg->m_nAutoInspectionCurrent++;
			pDlg->m_nBatchCurrent++;

			CString progress;
			progress.Format(
				_T("자동 검사 중: %d/%d (%.1f%%)"),
				pDlg->m_nAutoInspectionCurrent,
				pDlg->m_nAutoInspectionCount,
				pDlg->m_nAutoInspectionCurrent * 100.0 / pDlg->m_nAutoInspectionCount
			);

			pDlg->SetDlgItemText(IDC_PROGRESS_STATIC, progress);

			if (!pDlg->m_strCurrentBatch.IsEmpty())
			{
				pDlg->UpdateBatchInfo();
			}

			Sleep(pDlg->m_nAutoInspectionInterval);
		}
	}

	if (pDlg->m_bAutoInspectionMode)
	{
		pDlg->PostMessage(WM_COMMAND, IDC_AUTO_INSPECTION_STOP);
	}

	return 0;
}

// ============================================================================
// 배치 관리 기능
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedBatchStart()
{
	CString operatorName = _T("작업자1");
	CString shift = _T("주간");
	int target = 1000;

	m_strOperatorName = operatorName;
	m_strShift = shift;
	m_nBatchTarget = target;
	m_nBatchCurrent = 0;

	CTime now = CTime::GetCurrentTime();
	m_strCurrentBatch = now.Format(_T("%Y%m%d_%H%M%S"));

	CString batchInfo;
	batchInfo.Format(
		_T("배치: %s\n작업자: %s\n근무조: %s\n목표: %d개"),
		m_strCurrentBatch,
		m_strOperatorName,
		m_strShift,
		m_nBatchTarget
	);

	SetDlgItemText(IDC_BATCH_INFO_STATIC, batchInfo);
}

void CPylonSampleProgramDlg::OnBnClickedBatchEnd()
{
	if (m_strCurrentBatch.IsEmpty())
	{
		AfxMessageBox(_T("시작된 배치가 없습니다!"));
		return;
	}

	CString result;
	result.Format(
		_T("========== 배치 완료 ==========\n\n")
		_T("배치 번호: %s\n")
		_T("작업자: %s\n")
		_T("근무조: %s\n\n")
		_T("목표: %d개\n")
		_T("검사: %d개\n")
		_T("달성률: %.1f%%\n\n")
		_T("불량: %d개\n")
		_T("정상: %d개\n")
		_T("불량률: %.2f%%\n\n")
		_T("평균 신뢰도: %.1f%%\n")
		_T("============================"),
		m_strCurrentBatch,
		m_strOperatorName,
		m_strShift,
		m_nBatchTarget,
		m_nBatchCurrent,
		m_nBatchCurrent * 100.0 / m_nBatchTarget,
		m_todayStats.defectCount,
		m_todayStats.normalCount,
		m_todayStats.defectRate,
		m_todayStats.avgConfidence
	);

	AfxMessageBox(result);

	m_strCurrentBatch.Empty();
	m_nBatchCurrent = 0;
	SetDlgItemText(IDC_BATCH_INFO_STATIC, _T("배치 없음"));
}

void CPylonSampleProgramDlg::UpdateBatchInfo()
{
	if (m_strCurrentBatch.IsEmpty())
	{
		return;
	}

	CString batchInfo;
	batchInfo.Format(
		_T("배치: %s\n")
		_T("진행: %d/%d (%.1f%%)\n")
		_T("불량률: %.2f%%"),
		m_strCurrentBatch,
		m_nBatchCurrent,
		m_nBatchTarget,
		m_nBatchTarget > 0 ? m_nBatchCurrent * 100.0 / m_nBatchTarget : 0.0,
		m_todayStats.defectRate
	);

	SetDlgItemText(IDC_BATCH_INFO_STATIC, batchInfo);
}

// ============================================================================
// 통계 기능
// ============================================================================

void CPylonSampleProgramDlg::UpdateStatistics(const AIResult& result)
{
	m_todayStats.totalCount++;

	if (result.is_defect)
	{
		m_todayStats.defectCount++;
	}
	else
	{
		m_todayStats.normalCount++;
	}

	m_todayStats.defectRate = m_todayStats.defectCount * 100.0 / m_todayStats.totalCount;

	double newConfidence = result.confidence * 100.0;
	m_todayStats.avgConfidence =
		(m_todayStats.avgConfidence * (m_todayStats.totalCount - 1) + newConfidence) / m_todayStats.totalCount;
}

void CPylonSampleProgramDlg::OnBnClickedShowStatistics()
{
	CString stats;
	stats.Format(
		_T("========================================\n")
		_T("           금일 검사 통계\n")
		_T("========================================\n\n")
		_T("총 검사: %d회\n")
		_T("불량: %d회\n")
		_T("정상: %d회\n\n")
		_T("불량률: %.2f%%\n")
		_T("평균 신뢰도: %.1f%%\n\n")
		_T("========================================"),
		m_todayStats.totalCount,
		m_todayStats.defectCount,
		m_todayStats.normalCount,
		m_todayStats.defectRate,
		m_todayStats.avgConfidence
	);

	AfxMessageBox(stats);
}

void CPylonSampleProgramDlg::ResetStatistics()
{
	m_todayStats.totalCount = 0;
	m_todayStats.defectCount = 0;
	m_todayStats.normalCount = 0;
	m_todayStats.avgConfidence = 0.0;
	m_todayStats.defectRate = 0.0;
}

// ============================================================================
// 설정 저장/로드
// ============================================================================

void CPylonSampleProgramDlg::LoadSettings()
{
	m_nAutoInspectionInterval = GetPrivateProfileInt(
		_T("AUTO"), _T("Interval"), 2000, _T(".\\Settings.ini")
	);

	m_nAutoInspectionCount = GetPrivateProfileInt(
		_T("AUTO"), _T("Count"), 100, _T(".\\Settings.ini")
	);

	TCHAR buffer[256];
	GetPrivateProfileString(
		_T("SERVER"), _T("IP"), _T("127.0.0.1"),
		buffer, 256, _T(".\\Settings.ini")
	);
	m_strServerIP = buffer;

	m_nServerPort = GetPrivateProfileInt(
		_T("SERVER"), _T("Port"), 5555, _T(".\\Settings.ini")
	);
}

void CPylonSampleProgramDlg::SaveSettings()
{
	CString value;

	value.Format(_T("%d"), m_nAutoInspectionInterval);
	WritePrivateProfileString(
		_T("AUTO"), _T("Interval"), value, _T(".\\Settings.ini")
	);

	value.Format(_T("%d"), m_nAutoInspectionCount);
	WritePrivateProfileString(
		_T("AUTO"), _T("Count"), value, _T(".\\Settings.ini")
	);

	WritePrivateProfileString(
		_T("SERVER"), _T("IP"), m_strServerIP, _T(".\\Settings.ini")
	);

	value.Format(_T("%d"), m_nServerPort);
	WritePrivateProfileString(
		_T("SERVER"), _T("Port"), value, _T(".\\Settings.ini")
	);
}

void CPylonSampleProgramDlg::OnBnClickedSettings()
{
	CString msg;
	msg.Format(
		_T("자동 검사 설정\n\n")
		_T("현재 간격: %.1f초\n")
		_T("현재 횟수: %d회\n\n")
		_T("수정하시겠습니까?"),
		m_nAutoInspectionInterval / 1000.0,
		m_nAutoInspectionCount
	);

	if (AfxMessageBox(msg, MB_YESNO) == IDYES)
	{
		// TODO: 설정 다이얼로그 구현
	}
}

// ============================================================================
// 종료
// ============================================================================

void CPylonSampleProgramDlg::OnBnClickedExitBtn()
{
	// 자동 검사 중지
	if (m_bAutoInspectionMode)
	{
		OnBnClickedAutoInspectionStop();
	}

	// 설정 저장
	SaveSettings();

	// 카메라 리소스 해제
	for (int k = 0; k < CAM_NUM; k++)
	{
		if (bitmapinfo[k])
		{
			delete bitmapinfo[k];
			bitmapinfo[k] = NULL;
		}

		if (pImageColorDestBuffer[k])
		{
			for (int i = 0; i < BUF_NUM; i++)
			{
				if (pImageColorDestBuffer[k][i])
				{
					free(pImageColorDestBuffer[k][i]);
				}
			}
			free(pImageColorDestBuffer[k]);
			pImageColorDestBuffer[k] = NULL;
		}

		if (pImageresizeOrgBuffer[k])
		{
			for (int i = 0; i < BUF_NUM; i++)
			{
				if (pImageresizeOrgBuffer[k][i])
				{
					free(pImageresizeOrgBuffer[k][i]);
				}
			}
			free(pImageresizeOrgBuffer[k]);
			pImageresizeOrgBuffer[k] = NULL;
		}

		if (m_CameraManager.m_bCamOpenFlag[k] == true)
		{
			m_CameraManager.Close_Camera(k);
		}

		if (hdc[k])
		{
			::ReleaseDC(hWnd[k], hdc[k]);
			hdc[k] = NULL;
		}
	}

	// TCP 연결 종료
	if (m_bConnectedToServer)
	{
		m_tcpClient.Disconnect();
		m_bConnectedToServer = false;
	}

	CDialog::OnOK();
}


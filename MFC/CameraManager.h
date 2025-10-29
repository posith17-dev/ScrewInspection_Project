#pragma once

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#include <pylon/PylonGUI.h>
#include <pylon/gige/BaslerGigECamera.h>
#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/BaslerGigEDeviceInfo.h>
//#include <pylon/gige/PylonGigEDevice.h>




using namespace Pylon;
using namespace GenApi;



#define CAM_NUM   4
#define BUF_NUM   3



//class CCameraEndofExposureEventHandler : public CCameraEventHandler;

class CCameraManager : public CImageEventHandler ,              // For receiving grabbed images.
                       public CConfigurationEventHandler     // For getting notified about device removal.

{


public:
	CCameraManager(void);
	~CCameraManager(void);

	// Camera ���� ��� ���� ����
    CTlFactory *m_tlFactory;                       // Get the transport layer factory.
	CInstantCameraArray m_pCamera;                 // Multi Camera
	CInstantCameraArray m_pCopyCamera;                 // Multi Camera
	CPylonImage Image[CAM_NUM];
    CImageFormatConverter converter[CAM_NUM];
    CIntegerPtr m_pHeartbeatTimeout[CAM_NUM];
    IGigETransportLayer * m_pIP;

	INodeMap *m_pCameraNodeMap[CAM_NUM];                           
	DeviceInfoList_t devices;
	
   // CBaslerGigEDeviceInfo di;

    //   IPylonGigEDevice *mm;


	CEnumerationPtr			ptrEnumeration[CAM_NUM];
	CIntegerPtr				ptrInteger[CAM_NUM];
	CBooleanPtr             ptrBoolean[CAM_NUM];
	CFloatPtr				ptrFloat[CAM_NUM];
	CCommandPtr             ptrCommand[CAM_NUM];
    



/*************************************************************************************************/
/*							log ����															 */
/*************************************************************************************************/
	 bool  bLogUse;
	 bool  bMessageBoxUse;
	 bool  bTraceUse;
     bool  bStopLiveThread;
	 bool  bStopFlag;
	 FILE *log;
	 char filename[256];
	 time_t t;
	 int Hour,Min,Sec;
/*************************************************************************************************/
/*							Image buffer														 */
/*************************************************************************************************/
	 unsigned char*   pImage8Buffer[CAM_NUM];              // 8bit image receive buffer
	 unsigned short*  pImage12Buffer[CAM_NUM];      	   // 8bit �̻� image receive buffer
	 unsigned char*   pImage24Buffer[CAM_NUM];             // 24bit image receive buffer
     unsigned char *pImageBigBuffer[CAM_NUM];      // Big image

	 
     CString   m_strCM_ImageForamt[CAM_NUM];               // Image format	

     int       m_iCM_OffsetX[CAM_NUM];
	 int       m_iCM_OffsetY[CAM_NUM];
     int       m_iCM_Width[CAM_NUM];
	 int       m_iCM_Height[CAM_NUM];
	 int       m_iCM_reSizeWidth[CAM_NUM];

	 bool      m_bCaptureEnd[CAM_NUM];                        // capture finish flag
	 bool      m_bRemoveCamera[CAM_NUM];                      // ī�޶� ���� ���� ����
	 

     bool      m_bCamOpenFlag[CAM_NUM];                       // ī�޶� open flag
	 bool      m_bCamConnectFlag[CAM_NUM];                    // ī�޶� Connect flag
 
	 long      m_iSkippiedFrame[CAM_NUM];
	 long      m_iGrabbedFrame[CAM_NUM];
    
	 int        m_imgNjm;
	 
	// ��� �Լ� ����
    
	int FindCamera(char szCamName[CAM_NUM][100],char szCamSerialNumber[CAM_NUM][100],char szInterfacName[CAM_NUM][100],int *nCamNumber); // �Ϸ�(finish)
	int Open_Camera(int nCamIndex, int nPosition);
	int Close_Camera(int nCamIndex);
    int Connect_Camera(int nCamIndex,int nOffsetX,int nOffsetY,int nWidth,int nHeight,CString strImgFormat);
    int SingleGrab(int nCamIndex);
	int GrabLive(int nCamIndex,int nMode);

	int LiveStop(int nCamIndex,int nMode);
	
	int GetEnumeration(int nCamIndex, char *szValue, char *szNodeName);
	int SetEnumeration(int nCamIndex, char *szValue, char *szNodeName);
	int GetInteger(int nCamIndex, int *nValue, char *szNodeName);
	int SetInteger(int nCamIndex, int nValue, char *szNodeName);
    int GetIntegerMax(int nCamIndex, int *nValue, char *szNodeName);
	int GetIntegerMin(int nCamIndex, int *nValue, char *szNodeName);
	int GetBoolean(int nCamIndex, bool *bValue, char *szNodeName);
	int SetBoolean(int nCamIndex, bool bValue, char *szNodeName);
	int GetFloat(int nCamIndex, float *fValue, char *szNodeName);
	int SetFloat(int nCamIndex, float fValue, char *szNodeName);
	int SetCommand(int nCamIndex, char *szNodeName);	
	
	void ReadEnd(int nCamIndex);
	void WriteLog(int nCamIdex, CString strTemp1, CString strTemp2);

	bool CheckCaptureEnd(int nCamIndex);
	int SaveImage(int nFileFormat, unsigned char* pImage, char *filename,int nPixelType,int width, int height,int nColorband);

	static UINT LiveThread(void *lParam);

    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult);
	virtual void OnImagesSkipped( CInstantCamera& camera, size_t countOfSkippedImages);
    virtual void OnCameraDeviceRemoved( CInstantCamera& camera);

};

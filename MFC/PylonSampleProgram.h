// PylonSampleProgram.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CPylonSampleProgramApp:
// �� Ŭ������ ������ ���ؼ��� PylonSampleProgram.cpp�� �����Ͻʽÿ�.
//

class CPylonSampleProgramApp : public CWinApp
{
public:
	CPylonSampleProgramApp();

// �������Դϴ�.
	public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CPylonSampleProgramApp theApp;
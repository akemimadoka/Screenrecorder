
// screenrecorder.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CscreenrecorderApp: 
// �йش����ʵ�֣������ screenrecorder.cpp
//

class CscreenrecorderApp : public CWinApp
{
public:
	CscreenrecorderApp();

// ��д
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CscreenrecorderApp theApp;
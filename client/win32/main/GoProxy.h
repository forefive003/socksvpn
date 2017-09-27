
// FullProxy.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CGoProxyApp:
// See FullProxy.cpp for the implementation of this class
//

class CGoProxyApp : public CWinApp
{
public:
	CGoProxyApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	HANDLE m_hMutex;

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CGoProxyApp theApp;
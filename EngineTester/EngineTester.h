// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// EngineTester.h : main header file for the EngineTester application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


void UnicodeToAnsi(const wchar_t *szUnicode, int len, char *szAnsi);
void AnsiToUnicode(const char *szAnsi, wchar_t *szUnicode, int len);


// CEngineTesterApp:
// See EngineTester.cpp for the implementation of this class
//

class CEngineTesterApp : public CWinAppEx
{
public:
	CEngineTesterApp();
	~CEngineTesterApp();
	void Init_OCR();

public:
	TCHAR m_ExePath[256];
	BYTE *m_pDicData[5];
	BYTE *m_pLockSymDic;
	int m_nDicCount;
// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
protected:
	ULONG_PTR m_GdiplusToken;
	Gdiplus::Status m_gdiplusstatus;
};

extern CEngineTesterApp theApp;

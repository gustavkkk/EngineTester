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

// EngineTester.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "EngineTester.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "EngineTesterDoc.h"
#include "EngineTesterView.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



void UnicodeToAnsi(const wchar_t *szUnicode, int len, char *szAnsi)
{
	WideCharToMultiByte(CP_ACP, 0, szUnicode, -1, szAnsi, len, NULL, NULL);
}

void AnsiToUnicode(const char *szAnsi, wchar_t *szUnicode, int len)
{
	MultiByteToWideChar(CP_ACP, 0, szAnsi, -1, szUnicode, len);
}


// CEngineTesterApp

BEGIN_MESSAGE_MAP(CEngineTesterApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CEngineTesterApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CEngineTesterApp construction

CEngineTesterApp::CEngineTesterApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("EngineTester.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

CEngineTesterApp::~CEngineTesterApp()
{
	int i;
	for( i = 0; i < m_nDicCount; i++)
	{
		if(m_pDicData[i])
			free(m_pDicData[i]);
	}

	if( m_pLockSymDic )
		free(m_pLockSymDic);
}

// The one and only CEngineTesterApp object

CEngineTesterApp theApp;


// CEngineTesterApp initialization

void CEngineTesterApp::Init_OCR()
{
	TCHAR DicPath[256];
	TCHAR temp[256];
	int i, fh, nSize;
	
	m_nDicCount = 5;
	
	for( i = 0; i < m_nDicCount; i++)
	{
		m_pDicData[i] = NULL;
		wcscpy(temp, m_ExePath);
		wcscat( temp, L"\\DIC\\sz_ocr_" );
		wsprintf( DicPath, L"%s%d", temp,i);
		wcscat(DicPath, L".dic");

		fh = _wopen(DicPath, _O_RDONLY | _O_BINARY, _S_IREAD);
		if(fh != -1)
		{
			nSize = _filelength(fh);
			m_pDicData[i] = (BYTE*)malloc(nSize);
			_read(fh, m_pDicData[i], nSize);
			_close(fh);
		}
	}

	m_pLockSymDic = NULL;
	
	wcscpy(DicPath, m_ExePath);
	wcscat(DicPath, L"\\DIC\\sz_sym.dic" );
			
	fh = _wopen(DicPath , _O_RDONLY | _O_BINARY, _S_IREAD);
	if( fh != -1 )
	{
		nSize = _filelength(fh);
		m_pLockSymDic = (BYTE*)malloc(nSize);
		_read(fh, m_pLockSymDic, nSize);
		_close(fh);
	}

}

BOOL CEngineTesterApp::InitInstance()
{
	GetModuleFileName(AfxGetInstanceHandle(), m_ExePath, MAX_PATH);
	m_ExePath[_tcsrchr(m_ExePath, '\\')-m_ExePath] = 0;
	Init_OCR();

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization

	//kjy-todo
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	m_gdiplusstatus = GdiplusStartup(&m_GdiplusToken,&gdiplusStartupInput,NULL);
	//
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(16);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_EngineTesterTYPE,
		RUNTIME_CLASS(CEngineTesterDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CEngineTesterView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CEngineTesterApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	if(m_gdiplusstatus == 0)
		GdiplusShutdown(m_GdiplusToken);
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CEngineTesterApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CEngineTesterApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CEngineTesterApp customization load/save methods

void CEngineTesterApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CEngineTesterApp::LoadCustomState()
{
}

void CEngineTesterApp::SaveCustomState()
{
}

// CEngineTesterApp message handlers




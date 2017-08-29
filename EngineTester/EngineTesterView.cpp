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

// EngineTesterView.cpp : implementation of the CEngineTesterView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "EngineTester.h"
#endif

#include "EngineTesterDoc.h"
#include "EngineTesterView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEngineTesterView

IMPLEMENT_DYNCREATE(CEngineTesterView, CScrollView)

BEGIN_MESSAGE_MAP(CEngineTesterView, CScrollView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CEngineTesterView construction/destruction

CEngineTesterView::CEngineTesterView()
{
	// TODO: add construction code here

}

CEngineTesterView::~CEngineTesterView()
{
}

BOOL CEngineTesterView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

// CEngineTesterView drawing

void CEngineTesterView::OnDraw(CDC* pDC)
{
	CEngineTesterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
	CSize sizeTotal;

	sizeTotal.cx = pDoc->GetWidth();
	sizeTotal.cy = pDoc->GetHeight();
	SetScrollSizes(MM_TEXT, sizeTotal);

	BITMAPINFO	m_BmpInfo;

	memset(&m_BmpInfo.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	m_BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_BmpInfo.bmiHeader.biPlanes = 1;
	m_BmpInfo.bmiHeader.biBitCount = 24;
	m_BmpInfo.bmiHeader.biHeight = -pDoc->GetHeight();
	m_BmpInfo.bmiHeader.biWidth = pDoc->GetWidth();

	SetDIBitsToDevice(pDC->GetSafeHdc(),
		0, 0, pDoc->GetWidth(), pDoc->GetHeight(),
		0, 0, 0, pDoc->GetHeight(),
		pDoc->GetBits(), &m_BmpInfo, DIB_RGB_COLORS);
}

void CEngineTesterView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize sizeTotal;
	// TODO: calculate the total size of this view
	CEngineTesterDoc* pDoc = GetDocument();

	sizeTotal.cx = pDoc->GetWidth();
	sizeTotal.cy = pDoc->GetHeight();
	SetScrollSizes(MM_TEXT, sizeTotal);
}

void CEngineTesterView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CEngineTesterView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CEngineTesterView diagnostics

#ifdef _DEBUG
void CEngineTesterView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CEngineTesterView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CEngineTesterDoc* CEngineTesterView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEngineTesterDoc)));
	return (CEngineTesterDoc*)m_pDocument;
}
#endif //_DEBUG


// CEngineTesterView message handlers


void CEngineTesterView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// TODO: Add your specialized code here and/or call the base class
	if (bActivate)
	{
		CEngineTesterDoc	*pDoc = GetDocument();

		((CMainFrame *)AfxGetMainWnd())->SetPaneString(pDoc->GetOCRResult());

	}

	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

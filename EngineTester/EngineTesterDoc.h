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

// EngineTesterDoc.h : interface of the CEngineTesterDoc class
//


#pragma once

//#include <opencv\cv.h>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include "OCREngine.h"
#include "LockSymbolLearning.h"
#include "CharLearningDlg.h"

using namespace cv;
using namespace std;

#define LEARNING_FUNCTION

class CEngineTesterDoc : public CDocument
{
protected: // create from serialization only
	CEngineTesterDoc();
	DECLARE_DYNCREATE(CEngineTesterDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CEngineTesterDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	int GetWidth();
	int GetHeight();
	LPBYTE GetBits();
	CString GetOCRResult()	{ return m_strResult; }

protected:
	CString m_strImagePath;
	Mat m_image;
	Mat m_photo;
	CRect m_rcROI;
	BOOL m_bOCR;
	CString m_strResult;
	OCR_RESULT m_OcrResult;
	int m_nSelView;
	void Reset();


// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	afx_msg void OnImageRotateCW();
	afx_msg void OnUpdateImageRotateCW(CCmdUI *pCmdUI);
	afx_msg void OnImageRotateCCW();
	afx_msg void OnUpdateImageRotateCCW(CCmdUI *pCmdUI);
	afx_msg void OnImageRotate180();
	afx_msg void OnUpdateImageRotate180(CCmdUI *pCmdUI);
	afx_msg void OnImageCrop();
	afx_msg void OnUpdateImageCrop(CCmdUI *pCmdUI);
	afx_msg void OnImageStringArea();
	afx_msg void OnUpdateImageStringArea(CCmdUI *pCmdUI);
	afx_msg void OnImageInclCorr();
	afx_msg void OnUpdateImageInclCorr(CCmdUI *pCmdUI);
	afx_msg void OnImageSegment();
	afx_msg void OnUpdateImageSegment(CCmdUI *pCmdUI);
	afx_msg void OnImageOCR();
#ifdef LEARNING_FUNCTION
	afx_msg void OnImageLearning();
	afx_msg void OnImageCharlearning();
#endif
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	afx_msg void OnImageBinarization();
	afx_msg void OnImageAlpr();
	afx_msg void OnImageFacerecognition();
	afx_msg void OnImageObjectrecognition();
	afx_msg void OnImageLoad();
};

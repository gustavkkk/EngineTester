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

// EngineTesterDoc.cpp : implementation of the CEngineTesterDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "EngineTester.h"
#endif

#include "EngineTesterDoc.h"
#include "MainFrm.h"

#include <propkey.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CEngineTesterDoc

IMPLEMENT_DYNCREATE(CEngineTesterDoc, CDocument)

BEGIN_MESSAGE_MAP(CEngineTesterDoc, CDocument)
	ON_COMMAND(ID_IMAGE_ROTATE_CW, &CEngineTesterDoc::OnImageRotateCW)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_ROTATE_CW, &CEngineTesterDoc::OnUpdateImageRotateCW)
	ON_COMMAND(ID_IMAGE_ROTATE_CCW, &CEngineTesterDoc::OnImageRotateCCW)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_ROTATE_CCW, &CEngineTesterDoc::OnUpdateImageRotateCCW)
	ON_COMMAND(ID_IMAGE_ROTATE180, &CEngineTesterDoc::OnImageRotate180)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_ROTATE180, &CEngineTesterDoc::OnUpdateImageRotate180)
	ON_COMMAND(ID_IMAGE_CROP, &CEngineTesterDoc::OnImageCrop)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_CROP, &CEngineTesterDoc::OnUpdateImageCrop)
	ON_COMMAND(ID_IMAGE_STRING_AREA, &CEngineTesterDoc::OnImageStringArea)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_STRING_AREA, &CEngineTesterDoc::OnUpdateImageStringArea)
	ON_COMMAND(ID_IMAGE_INCL_CORR, &CEngineTesterDoc::OnImageInclCorr)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_INCL_CORR, &CEngineTesterDoc::OnUpdateImageInclCorr)
	ON_COMMAND(ID_IMAGE_SEGMENT, &CEngineTesterDoc::OnImageSegment)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_SEGMENT, &CEngineTesterDoc::OnUpdateImageSegment)
	ON_COMMAND(ID_IMAGE_OCR, &CEngineTesterDoc::OnImageOCR)
#ifdef LEARNING_FUNCTION
	ON_COMMAND(ID_IMAGE_LEARNING, &CEngineTesterDoc::OnImageLearning)
	ON_COMMAND(ID_IMAGE_CHARLEARNING, &CEngineTesterDoc::OnImageCharlearning)
#endif
	ON_COMMAND(ID_IMAGE_BINARIZATION, &CEngineTesterDoc::OnImageBinarization)
	ON_COMMAND(ID_IMAGE_ALPR, &CEngineTesterDoc::OnImageAlpr)
	ON_COMMAND(ID_IMAGE_FACERECOGNITION, &CEngineTesterDoc::OnImageFacerecognition)
	ON_COMMAND(ID_IMAGE_OBJECTRECOGNITION, &CEngineTesterDoc::OnImageObjectrecognition)
	ON_COMMAND(ID_IMAGE_LOAD, &CEngineTesterDoc::OnImageLoad)
END_MESSAGE_MAP()


// CEngineTesterDoc construction/destruction

CEngineTesterDoc::CEngineTesterDoc()
	: m_strResult(_T(""))
	, m_bOCR(FALSE)
	, m_nSelView(0)
{
	// TODO: add one-time construction code here
}

CEngineTesterDoc::~CEngineTesterDoc()
{
}

BOOL CEngineTesterDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return FALSE;//TRUE;
}

BOOL CEngineTesterDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	// TODO:  Add your specialized creation code here
	char	szFileName[MAX_PATH];

	UnicodeToAnsi(lpszPathName, MAX_PATH, szFileName);
	m_image = imread(szFileName);
	if (m_image.empty())
	{
		return FALSE;
	}

	if ((m_image.cols % 4) || (m_image.rows % 4))
	{
		cv::resize(m_image, m_image, cv::Size(m_image.cols/4*4, m_image.rows/4*4));
	}

	m_strImagePath = lpszPathName;

	return TRUE;
}




// CEngineTesterDoc serialization

void CEngineTesterDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CEngineTesterDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CEngineTesterDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CEngineTesterDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CEngineTesterDoc diagnostics

#ifdef _DEBUG
void CEngineTesterDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CEngineTesterDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

int CEngineTesterDoc::GetWidth()
{
	switch (m_nSelView)
	{
	case 1:
		break;

	case 2:
		return m_OcrResult.imgIncCorr.cols;

	case 3:
		return m_OcrResult.seg_res.imgSegments.cols;
	}

	return m_image.cols;
}

int CEngineTesterDoc::GetHeight()
{
	switch (m_nSelView)
	{
	case 1:
		break;

	case 2:
		return m_OcrResult.imgIncCorr.rows;

	case 3:
		return m_OcrResult.seg_res.imgSegments.rows;
	}

	return m_image.rows;
}

LPBYTE CEngineTesterDoc::GetBits()
{
	switch (m_nSelView)
	{
	case 1:
		break;

	case 2:
		return m_OcrResult.imgIncCorr.data;

	case 3:
		return m_OcrResult.seg_res.imgSegments.data;
	}

	return m_image.data;
}

void CEngineTesterDoc::Reset()
{
	m_rcROI.SetRectEmpty();
	m_bOCR = FALSE;
	m_strResult.Empty();
	((CMainFrame *)AfxGetMainWnd())->SetPaneString(m_strResult);
}


// CEngineTesterDoc commands


void CEngineTesterDoc::OnImageRotateCW()
{
	// TODO: Add your command handler code here
	Mat	imgRotate(m_image.cols, m_image.rows, CV_8UC3);
	int	offset = 0;

	for (int y = 0; y < m_image.rows; y++)
	{
		for (int x = 0; x < m_image.cols; x++)
		{
			imgRotate.data[x*imgRotate.step+(m_image.rows-1-y)*3] = m_image.data[offset];
			imgRotate.data[x*imgRotate.step+(m_image.rows-1-y)*3+1] = m_image.data[offset+1];
			imgRotate.data[x*imgRotate.step+(m_image.rows-1-y)*3+2] = m_image.data[offset+2];

			offset += 3;
		}
	}

	m_image = imgRotate;

	UpdateAllViews(NULL);
	Reset();
}


void CEngineTesterDoc::OnUpdateImageRotateCW(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(!m_nSelView);
}


void CEngineTesterDoc::OnImageRotateCCW()
{
	// TODO: Add your command handler code here
	Mat	imgRotate(m_image.cols, m_image.rows, CV_8UC3);
	int	offset = 0;

	for (int y = 0; y < m_image.rows; y++)
	{
		for (int x = 0; x < m_image.cols; x++)
		{
			imgRotate.data[(m_image.cols-1-x)*imgRotate.step+y*3] = m_image.data[offset];
			imgRotate.data[(m_image.cols-1-x)*imgRotate.step+y*3+1] = m_image.data[offset+1];
			imgRotate.data[(m_image.cols-1-x)*imgRotate.step+y*3+2] = m_image.data[offset+2];

			offset += 3;
		}
	}

	m_image = imgRotate;

	UpdateAllViews(NULL);
	Reset();
}


void CEngineTesterDoc::OnUpdateImageRotateCCW(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(!m_nSelView);
}


void CEngineTesterDoc::OnImageRotate180()
{
	// TODO: Add your command handler code here
	flip(m_image, m_image, -1);

	UpdateAllViews(NULL);
	Reset();
}


void CEngineTesterDoc::OnUpdateImageRotate180(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(!m_nSelView);
}


void CEngineTesterDoc::OnImageCrop()
{
	// TODO: Add your command handler code here
	m_bOCR = FALSE;
}

void CEngineTesterDoc::OnUpdateImageCrop(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(!m_rcROI.IsRectEmpty()&&!m_nSelView);
}


void CEngineTesterDoc::OnImageStringArea()
{
	// TODO: Add your command handler code here
	m_nSelView = 1;
}


void CEngineTesterDoc::OnUpdateImageStringArea(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_bOCR);
	pCmdUI->SetCheck(m_nSelView==1);
}


void CEngineTesterDoc::OnImageInclCorr()
{
	// TODO: Add your command handler code here
	m_nSelView = m_nSelView == 2 ? 0 : 2;
	UpdateAllViews(NULL);
}


void CEngineTesterDoc::OnUpdateImageInclCorr(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_bOCR);
	pCmdUI->SetCheck(m_nSelView==2);
}


void CEngineTesterDoc::OnImageSegment()
{
	// TODO: Add your command handler code here
	m_nSelView = m_nSelView == 3 ? 0 : 3;
	UpdateAllViews(NULL);
}


void CEngineTesterDoc::OnUpdateImageSegment(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_bOCR);
	pCmdUI->SetCheck(m_nSelView==3);
}


void CEngineTesterDoc::OnImageOCR()
{
	// TODO: Add your command handler code here
	TCHAR		*szResult = NULL;
	BYTE **ppDic = theApp.m_pDicData;
	BYTE *pLockSymDic = theApp.m_pLockSymDic;

	OCR(m_image, &m_OcrResult);

	int		fh;
	char	szLine[512] = "No,1st cand.,2nd cand.,3rd cand.,4th cand.,5th cand.,6th cand.,7th cand.,8th cand.,9th cand.,10th cand.\n";

	fh = _wopen(m_strImagePath+L".csv", _O_CREAT|_O_WRONLY, _S_IWRITE);
	_write(fh, szLine, lstrlenA(szLine));

	for (int i = 0; i < (int)m_OcrResult.confidence.size(); i++)
	{

		wsprintfA(szLine, "%d", i+1);

		for (int j = 0; j < (int)m_OcrResult.confidence[i].codes.size(); j++)
		{
			char	szConf[32];

			sprintf(szConf, ",%c(%2.3f%%)",
				m_OcrResult.confidence[i].codes[j],
				m_OcrResult.confidence[i].degrees[j]*100);

			strcat(szLine, szConf);
		}

		strcat(szLine, "\n");
		_write(fh, szLine, lstrlenA(szLine));
	}

	_close(fh);

	if (m_OcrResult.string.size())
	{
		try
		{
			szResult = new TCHAR[m_OcrResult.string.size()+1];
			AnsiToUnicode(m_OcrResult.string.data(), szResult, m_OcrResult.string.size());
			szResult[m_OcrResult.string.size()] = 0;

			m_strResult = CString(szResult);
			delete []szResult;

			((CMainFrame *)AfxGetMainWnd())->SetPaneString(m_strResult);
			UpdateAllViews(NULL);
		}
		catch (...)
		{
		}
	}
	else
	{
		//AfxMessageBox(m_szResult, MB_OK|MB_ICONINFORMATION);
	}

	m_bOCR = TRUE;
}


#ifdef LEARNING_FUNCTION
void CEngineTesterDoc::OnImageLearning()
{
	// TODO: Add your command handler code here

	vector<Mat> m_imagelist;
	m_imagelist = GetLockList(m_image);
	

	LockSymbolLearning locksymbol;
	locksymbol.setImageList(m_imagelist);
	locksymbol.DoModal();
}


void CEngineTesterDoc::OnImageCharlearning()
{
	// TODO: Add your command handler code here
	vector<Mat> m_imagelist;
	//OCR(m_image, &m_OcrResult);
	Mat image_Segments=m_OcrResult.seg_res.imgNR;
	vector<cv::Rect> segments=m_OcrResult.seg_res.segments;
	vector<char> string=m_OcrResult.string;
	for(unsigned int i=0;i<segments.size();++i)
	{
		Mat m;

		Mat(image_Segments,segments[i]).copyTo(m);
		cv::cvtColor(m, m, CV_BGR2GRAY);
		m_imagelist.push_back(m);
	}
	
	CharLearningDlg charlearning;
	charlearning.setImageList(m_imagelist,string);
	charlearning.DoModal();
}
#endif


BOOL CEngineTesterDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// TODO: Add your specialized code here and/or call the base class

	return FALSE;//CDocument::OnSaveDocument(lpszPathName);
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void CEngineTesterDoc::OnImageBinarization()
{
	// TODO: Add your command handler code here
	AfxMessageBox(_T("I am here!")); 
}

void CEngineTesterDoc::OnImageAlpr()
{
	// TODO: Add your command handler code here
	//AfxMessageBox(_T("I am here!")); 
	if(IsInvolved(m_image))
	{
		//AfxMessageBox(_T("Succeed!")); 
	}
	else
	{
		//AfxMessageBox(_T("Failed!")); 
	}	
}


void CEngineTesterDoc::OnImageFacerecognition()
{
	// TODO: Add your command handler code here
	//AfxMessageBox(_T("I am here!")); 
	containFaces(m_image);
}


void CEngineTesterDoc::OnImageObjectrecognition()
{
	// TODO: Add your command handler code here
	//AfxMessageBox(_T("I am here!")); 
	containCars(m_image);
}


void CEngineTesterDoc::OnImageLoad()
{
	// TODO: Add your command handler code here
	TCHAR		szFilter[] = _T("All supported files (*.bmp;*.jpg;*.tif;*.png;...)|*.bmp;*.dib;*.jpg;*.jpeg;*.jpe;*.jfif;*.tif;*.tiff;*.png|")
							_T("Bitmap files (*.bmp)|*.dib|")
							_T("JPEG files (*.jpg;*.jpeg;*.jpe;*.jfif)|*.jpg;*.jpeg;*.jpe;*.jfif|")
							_T("TIFF files (*.tif;*.tiff)|*.tif;*.tiff|")
							_T("PNG files (*.png)|*.png|")
							_T("All files (*.*)|*.*||");
	CFileDialog	fileDlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT|OFN_EXPLORER, szFilter);
	
	CString fileName;
	const int c_cMaxFiles = 200;
	const int c_cbBuffSize = (c_cMaxFiles * (MAX_PATH + 1)) + 1;

	//fileDlg.GetOFN().lpstrInitialDir = theApp.m_szOpenDir;
	fileDlg.GetOFN().lpstrFile = fileName.GetBuffer(c_cbBuffSize);
	fileDlg.GetOFN().nMaxFile = c_cbBuffSize;

	if (fileDlg.DoModal() == IDOK)
	{
		POSITION	pos;
		CString		strDisplayFileName = _T("");
		int nSeek = 0;

		pos = fileDlg.GetStartPosition();
		while (pos != NULL)
		{
			CString	strFilePath;
			char szFile[MAX_PATH];

			strFilePath = fileDlg.GetNextPathName(pos);
			
			//lstrcpy(theApp.m_szOpenDir, strFilePath.Left(strFilePath.ReverseFind('\\')));
			//theApp.SaveSettings();

			strDisplayFileName += "\"";
			strDisplayFileName += strFilePath.Right(strFilePath.GetLength()-strFilePath.ReverseFind('\\')-1);
			strDisplayFileName += "\" ";
			
			UnicodeToAnsi(strFilePath, sizeof(szFile), szFile);
			////////////////////////////////////////////////////////////////////////////////////
			m_photo = imread(szFile);
			if (m_photo.empty())
			{
				return;
			}

			if ((m_photo.cols % 4) || (m_photo.rows % 4))
			{
				cv::resize(m_photo, m_photo, cv::Size(m_photo.cols/4*4, m_photo.rows/4*4));
			}

			GetFeatures(m_photo);
		}
	}

	fileName.ReleaseBuffer();

	//GetTopLevelParent()->FlashWindowEx(FLASHW_CAPTION, 0, 0);
}

// LockSymbolLearning.cpp : implementation file
//

#include "stdafx.h"
#include "EngineTester.h"
#include "LockSymbolLearning.h"
#include "afxdialogex.h"
#include <WinGDI.h>
#include "OCREngine.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

extern BYTE *pLockSymDic;

// LockSymbolLearning dialog

IMPLEMENT_DYNAMIC(LockSymbolLearning, CDialogEx)

LockSymbolLearning::LockSymbolLearning(CWnd* pParent /*=NULL*/)
	: CDialogEx(LockSymbolLearning::IDD, pParent)
	, m_pThumbView(NULL)
{
}

LockSymbolLearning::~LockSymbolLearning()
{
}

void LockSymbolLearning::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_LIST_IMAGE, m_imageListCtrl);
}


BEGIN_MESSAGE_MAP(LockSymbolLearning, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &LockSymbolLearning::OnBnClickedButtonCancel)
	ON_BN_CLICKED(IDC_BUTTON_LEARNING, &LockSymbolLearning::OnBnClickedButtonLearning)
	ON_WM_CREATE()
END_MESSAGE_MAP()


// LockSymbolLearning message handlers


void LockSymbolLearning::OnBnClickedButtonCancel()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}


void LockSymbolLearning::OnBnClickedButtonLearning()
{
	// TODO: Add your control notification handler code here
	
	SEL_THUMB_LIST selList;
	m_pThumbView->GetSelectedIndex(selList);
	if (selList.size()==0)
	{
		AfxMessageBox(_T("Warining : Please select Lock Symbol")); 
	}
	else
	{
		for (SEL_THUMB_LIST::iterator iter = selList.begin(); iter != selList.end(); iter++)
		{
			Mat locksym = m_ImageList[*iter];
			
			//LearningLockSymbol(locksym.data, locksym.rows, locksym.cols);
			CString codeString("O");
			int nKind=3;
			LearningChar(locksym.data, locksym.rows, locksym.cols, codeString.GetAt(0), nKind);
		}

		CDialog::OnCancel();
	}
}


void LockSymbolLearning::setImageList(vector<Mat> image_list)
{
	m_ImageList=image_list;
	
	
}


int LockSymbolLearning::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	m_pThumbView = new CThumbView(RGB(255, 255, 255));
	m_pThumbView->CreateEx(WS_EX_STATICEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE, CRect(10, 10, 380, 250), this, 60000);


	return 0;
}

Mat ocr_gray2BGR(const Mat in)
{
	if(!in.data)
		return Mat();
	Mat out = Mat::zeros(in.rows,in.cols,CV_8UC3);
	for(int j = 0; j < in.rows; j++)
	{
		for(int i = 0; i < in.cols; i++)
		{
			if(*in.ptr(j,i) > 128)
			{
				*out.ptr(j,i) = 255;
				*(out.ptr(j,i) + 1) = 255;
				*(out.ptr(j,i) + 2) = 255;
			}
			else{
				*out.ptr(j,i) = 0;
				*(out.ptr(j,i) + 1) = 0;
				*(out.ptr(j,i) + 2) = 0;
			}
		}
	}
	return out;
}
BOOL LockSymbolLearning::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	int size=m_ImageList.size();
	for(int i=0;i<size;++i)
	{
		Mat currentImage=m_ImageList[i];
		Mat cvtImage;
		//cvtColor(currentImage,currentImage,CV_GRAY2BGR);
#ifdef OCR_TEST_LOCKSYMBOLLIST
		char name[15];
		sprintf(name,"lock %d",i);
		imshow(name,ocr_gray2BGR(currentImage));
		
#endif
		if ((currentImage.cols % 4) || (currentImage.rows % 4))
		{
			cv::resize(currentImage, currentImage, cv::Size(currentImage.cols/4*4, currentImage.rows/4*4));
		}
		m_pThumbView->AddThumbnail(ocr_gray2BGR(currentImage));
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// CharLearningDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EngineTester.h"
#include "CharLearningDlg.h"
#include "afxdialogex.h"
#include "OCREngine.h"

// CharLearningDlg dialog

IMPLEMENT_DYNAMIC(CharLearningDlg, CDialogEx)

CharLearningDlg::CharLearningDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CharLearningDlg::IDD, pParent)
	, m_pThumbView(NULL)
	, m_codeString(_T(""))
	, m_typeString(_T(""))
{

}

CharLearningDlg::~CharLearningDlg()
{
}

void CharLearningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);


	DDX_Text(pDX, IDC_EDIT1, m_codeString);
	DDX_Text(pDX, IDC_EDIT2, m_typeString);
}


BEGIN_MESSAGE_MAP(CharLearningDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1_Learning, &CharLearningDlg::OnBnClickedButton1Learning)
	ON_BN_CLICKED(IDC_BUTTON2_CANCEL, &CharLearningDlg::OnBnClickedButton2Cancel)
	ON_WM_CREATE()
END_MESSAGE_MAP()


// CharLearningDlg message handlers


BOOL CharLearningDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	
	int size=m_ImageList.size();
	for(int i=0;i<size;++i)
	{
		Mat currentImage;
		char currentString=m_string[i];
		cv::String cstring;
		cstring.push_back(currentString);
		Mat cvtImage;

		if (m_ImageList[i].cols % 4)
		{
			Mat(m_ImageList[i], cv::Rect(0, 0, m_ImageList[i].cols/4*4, m_ImageList[i].rows)).copyTo(currentImage);
		}
		else
		{
			m_ImageList[i].copyTo(currentImage);
		}

		cvtColor(currentImage, currentImage, CV_GRAY2BGR);
		m_pThumbView->AddString(m_string);
		m_pThumbView->AddThumbnail(currentImage);
	}
		
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CharLearningDlg::setImageList(vector<Mat> image_list, vector<char> string)
{
	m_ImageList=image_list;
	m_string=string;
	
}


void CharLearningDlg::OnBnClickedButton1Learning()
{
	// TODO: Add your control notification handler code here
	SEL_THUMB_LIST selList;
	m_pThumbView->GetSelectedIndex(selList);
	UpdateData(true);
	if (selList.size()==0 || m_codeString.IsEmpty() || m_typeString.IsEmpty())
	{
		AfxMessageBox(_T("Warining : Please select Char Symbol or type codeText and typeText")); 
	}
	else
	{
		
		int nKind = _wtoi(m_typeString.GetBuffer());
		if( nKind < 0 || nKind > 4 )
		{
			AfxMessageBox(_T("Enter from 0 to 4!")); 
		}
		else
		{
			m_codeString.MakeUpper();
			for (SEL_THUMB_LIST::iterator iter = selList.begin(); iter != selList.end(); iter++)
			{
			
				Mat CharImg = m_ImageList[*iter];
				//imshow("aaa", CharImag);waitKey(0);
			
				LearningChar(CharImg.data, CharImg.rows, CharImg.cols, m_codeString.GetAt(0), nKind);
			}

			CDialog::OnCancel();
		}
	}
}


void CharLearningDlg::OnBnClickedButton2Cancel()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}


int CharLearningDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	m_pThumbView = new CThumbView(RGB(255, 255, 255));
	m_pThumbView->CreateEx(WS_EX_STATICEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE, CRect(10, 10, 380, 250), this, 60000);

	return 0;
}

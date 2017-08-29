#pragma once

#include "opencv2\opencv.hpp"
#include "afxcmn.h"
#include "ThumbView.h"
#include "afxwin.h"
// CharLearningDlg dialog

using namespace cv;
using namespace std;

class CharLearningDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CharLearningDlg)

public:
	CharLearningDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CharLearningDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_CHAR_LEARNING };

private:
	vector<Mat> m_ImageList;
	vector<char> m_string;
	CThumbView *m_pThumbView;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void setImageList(vector<Mat> image_list, vector<char> string);
	afx_msg void OnBnClickedButton1Learning();
	afx_msg void OnBnClickedButton2Cancel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	
	
	CString m_codeString;
	CString m_typeString;
};

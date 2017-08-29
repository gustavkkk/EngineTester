#pragma once

#include "opencv2\opencv.hpp"
#include "afxcmn.h"
#include "ThumbView.h"
// LockSymbolLearning dialog

using namespace cv;
using namespace std;

class LockSymbolLearning : public CDialogEx
{
	DECLARE_DYNAMIC(LockSymbolLearning)

public:
	LockSymbolLearning(CWnd* pParent = NULL);   // standard constructor
	virtual ~LockSymbolLearning();

// Dialog Data
	enum { IDD = IDD_DIALOG_LEARNING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonCancel();
	afx_msg void OnBnClickedButtonLearning();

protected:
	


private:
	vector<Mat> m_ImageList;
	CThumbView *m_pThumbView;

public:
	void setImageList(vector<Mat> image_list);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();

	
};
	
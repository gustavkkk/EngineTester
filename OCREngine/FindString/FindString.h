#pragma once

#include "opencv/cv.h"
#include "OCR.h"
using namespace cv;

typedef struct OCR_Substitute_String{
	cv::Rect rect;
	Mat thr;
	Mat firstMark;
	Mat lastMark;
	cv::Rect firstLock;
	cv::Rect lastLock;
	bool existFirstLock;
	bool existLastLock;
	OCR_Substitute_String()
	{
		rect = cv::Rect();
		thr = cv::Mat();
		firstMark = cv::Mat();
		lastMark = cv::Mat();
		existFirstLock = false;
		existLastLock = false;
	}
	OCR_Substitute_String(cv::Rect rect_,Mat firstL,Mat lastL)
	{
		rect = rect_;
		firstMark = firstL;
		lastMark = lastL;
		existFirstLock = false;
		existLastLock = false;
	}
	OCR_Substitute_String(cv::Rect rect_,Mat firstL,Mat lastL,cv::Rect fll,cv::Rect lll)
	{
		rect = rect_;
		firstMark = firstL;
		lastMark = lastL;
		firstLock = fll;
		lastLock = lll;
		existFirstLock = false;
		existLastLock = false;
	}
	OCR_Substitute_String(cv::Rect rect_,Mat thr_,Mat firstL,Mat lastL,cv::Rect fll,cv::Rect lll)
	{
		rect = rect_;
		thr = thr_;
		firstMark = firstL;
		lastMark = lastL;
		firstLock = fll;
		lastLock = lll;
		existFirstLock = false;
		existLastLock = false;
	}
}Substitute_String,*LPSubstitute_String;

#define USER_DEFINED_RATIO 2
#define USER_DEFINED__W_MARGIN 40//10//40//30//15//30.0//20//15
#define USER_DEFINED__W_MARGIN_EQUALIZED USER_DEFINED__W_MARGIN//20
#define USER_DEFINED__H_MARGIN 10//8//4//2//6//6//4//2//3//6//8//6
#define USER_DEFINED__H_MARGIN_EQUALIZED  USER_DEFINED__H_MARGIN//12
#define USER_DEFINED__H_MARGIN_ZOOMOUT_UP 25
#define USER_DEFINED__H_MARGIN_ZOOMOUT_DOWN 30//20
#define USER_DEFINED_MIN 10000
#define USER_DEFINED_MAX 0

#define PI 3.141592
#define RATIO PI/180.0

extern BYTE *g_pLockSymDic;
extern BYTE *g_pDicData[DIC_COUNT];

class FindString{

private:
	Mat image,
	image_cv_gray,
	image_hsv,
	image_hsv_gray,
	image_hjh_gray,
	image_eroded,
	image_eroded_gray;

	Mat image_resized,
	image_cv_gray_resized,
	image_hsv_resized,
	image_hsv_gray_resized,
	image_hjh_gray_resized,
	image_eroded_resized,
	image_eroded_gray_resized;

	cv::Size image_size;
	cv::Size image_size_zoomed;

	int zoomout;
	int zoomin;
	bool flag;
	bool flag_;
	bool flag_needEqualization;

	Mat poster_edge;
	Mat drawing;
    std::vector<std::vector<cv::Point> > contours;
	Mat drawing_binary;
	Mat original_binary;
	Mat adaptiveThresholded;
	Mat tess;
	cv::Rect m_stringrect;
	Mat m_cropped_thr;

    std::vector<Substitute_String> subStringList;
    std::vector<Mat> StringList;

	bool isEmpty();
	bool isZoomed();
	void Init();
	void ModifydrawingBinary(int top,int bottom);
	void filtering(int* array_h);
	Mat processNormal();
	////////////////kjy-todo-2015.4.29///////////////
	Mat m_goodfeatures;
    std::vector<cv::Rect> rectlist;
    std::vector<cv::Point2f> pointlist;
	Mat m_img_;
	Mat m_jon;
	bool m_bIscropped;
	//////////////kjy-todo-11.21-for 90 %
	vector<double> m_rData;
	double	m_Rotation;
public:
	////////older////
	static void RefineString(Mat&);
	FindString();
	FindString(const Mat img_);
	~FindString();
	Substitute_String getStringRect();
    void getlocks(cv::Rect rect,Mat& firstL,Mat& lastL,cv::Rect& fll,cv::Rect& lll);
	Mat getBinaryImage(cv::Rect rect);
    void checkLocks_test(const Mat img_,std::vector<Substitute_String>& outputList);
	void insertSubString(Substitute_String substring);
    void findStringList(std::vector<Mat>& outputList);
	//////////old////////////
	void findStringRect(const Mat img_,Mat& original_thr,Mat& output,Mat drawing_);
	void findStringRect_(const Mat img,Mat& output);
    void findStringRect_test(const Mat img_,std::vector<Mat>& outputList);
	//////////new///////////
	void getGoodFeaturesAsPoints(Mat);
	void getGoodFeaturesAsPoints_new(Mat);
	void getRectList();
	void refineRectList();
	void getSSList();
	void getlocks_new(cv::Rect rect,Mat& string,Mat& firstL,Mat& lastL,cv::Rect& fll,cv::Rect& lll);
	cv::Rect GetHighDensityRect();
	Mat findString_kojy(Mat);
	/////last
	void GetWhatYouWantFirst(Mat);
    std::vector<OCR_Substitute_String> retrieveSSL()
	{
		return subStringList;
	};
	static Mat removeLocks(OCR_Substitute_String&);
		///kjy-todo-20150523
	void GetLockList(Mat image,vector<Mat>& locklist);
	void GetLockListEx(Mat image,vector<Mat>& locklist);
	vector<OCR_Substitute_String> GetSubstituteStringList(Mat image)
	{
		Init();
		GetWhatYouWantFirst(image);
		return retrieveSSL();
	}
	//kojy-todo-20151121
	vector<double>	GetRotationData() {return m_rData;}
	//
	void GetOutputList(Mat image, vector<Mat> &outputList);
	void GetOutputListEx(Mat image, vector<Mat>& outputList);
	void RefineRotationData();

	static Mat Rotation(Mat in,double angle = 0.0,double scale = 1.0);
};

BOOL existsLockSymbol(Mat& roi_);
Mat ocr_test_goodfeatures(Mat in);
void Refinebyfeatures(Mat mask,Mat& origin);
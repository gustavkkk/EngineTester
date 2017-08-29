#include "stdafx.h"
#include "FindString.h"
#include "FilterContours.h"
#include "Array.h"
#include "Binarization.h"
#include "RemoveNoise.h"
#include "InclCorr.h"
#include "ExtractBlack.h"
#include "PosterEdge.h"
#include "opencv2/highgui/highgui.hpp"

//#define OCR_DEBUG

//#define OCR_ONCE
//#define OCR_TEST
#define OCR_GOODFEATURES
#define OCR_STRING_COUNT_ESTIMATE 5//10

void calcProjection(Mat thresh_output,int* sum_h,int* sum_v)
{
	if(!thresh_output.data || !sum_h || !sum_v) return;
	for( int i = 0; i< thresh_output.rows; i++ )
	{
		sum_h[i] = 0;
		for(int j = 0; j < thresh_output.cols; j++ )
			if(*thresh_output.ptr(i,j) == 0)
				sum_h[i]++;
	}
	for( int i = 0; i< thresh_output.cols; i++ )
	{
		sum_v[i] = 0;
		for(int j = 0; j < thresh_output.rows; j++ )
			if(*thresh_output.ptr(j,i) == 0)
				sum_v[i]++;
	}
}
void calcProjection_t(Mat thresh_output,int* sum_h,int* sum_v)
{
	if(!thresh_output.data || !sum_h || !sum_v) return;
	for( int i = 0; i< thresh_output.rows; i++ )
	{
		sum_h[i] = 0;
		for(int j = 0; j < thresh_output.cols; j++ )
			if(*thresh_output.ptr(i,j) == 255)
				sum_h[i]++;
	}
	for( int i = 0; i< thresh_output.cols; i++ )
	{
		sum_v[i] = 0;
		for(int j = 0; j < thresh_output.rows; j++ )
			if(*thresh_output.ptr(j,i) == 255)
				sum_v[i]++;
	}
}
void calcHorizontalProjection(Mat thresh_output,int* sum_h,int value)
{
	if(!thresh_output.data || !sum_h || value < 0 || value > 255) return;
	for( int i = 0; i< thresh_output.rows; i++ )
	{
		sum_h[i] = 0;
		for(int j = 0; j < thresh_output.cols; j++ )
			if(*thresh_output.ptr(i,j) == value)
				sum_h[i]++;
	}
	return;
}
void calcVerticalProjection(Mat thresh_output,int* sum_v,int value)
{
	if(!thresh_output.data ||!sum_v || value < 0 || value > 255) return;
	for( int i = 0; i< thresh_output.cols; i++ )
	{
		sum_v[i] = 0;
		for(int j = 0; j < thresh_output.rows; j++ )
			if(*thresh_output.ptr(j,i) == value)
				sum_v[i]++;
	}
	return;
}

void findString(int* array_x,int rows,int* array_y,int cols,int& top,int& bottom,int& left,int& right)
{
	top = getFirstIndex(array_x,rows,1);
	bottom = getLastIndex(array_x,rows,1);
	left = getFirstIndex(array_y,cols,1);
	right = getLastIndex(array_y,cols,1);
	return;
}
#define USER_DEFINED_STRING_HEIGHT 150
#define USER_DEFINED_STRING_WIDTH 130
void findTopBottom(int* array_x,int rows,int& top,int& bottom)
{
	//calcGroupCount(array_x,rows,biggest_x);
	////calcSize(array_x,rows,top,bottom);
	//top = biggest_x.firstindex;
	//bottom = biggest_x.lastindex;
	//if((bottom - top) > USER_DEFINED_STRING_HEIGHT)
	calcSize(array_x,rows,top,bottom);

	if(top < 0)
		top = 0;
	if(bottom >= rows)
		bottom = rows;
	if(top > bottom)
	{
		top = 0;
		bottom = rows - 1;
	}
	return;
}
void findLeftRight(int* array_y,int cols,int& left,int& right)
{
	left = getFirstIndex(array_y,cols,1);
	right = getLastIndex(array_y,cols,1);
	////////////////////////////////////
	if(left > right)
	{
		left = 0;
		right = cols - 1;
	}
	return;
}
#define OCR_STRING_DENSITY 1
void findStringinBinaryImage(int* array_x,int rows,int* array_y,int cols,int& top,int& bottom,int& left,int& right)
{
	top = getFirstIndex(array_x,rows,OCR_STRING_DENSITY);
	bottom = getLastIndex(array_x,rows,OCR_STRING_DENSITY);
	left = getFirstIndex(array_y,cols,OCR_STRING_DENSITY);
	right = getLastIndex(array_y,cols,OCR_STRING_DENSITY);
	////////////////////////////////////////
	if(top > bottom)
	{
		top = 0;
		bottom = rows - 1;
	}
	if(left > right)
	{
		left = 0;
		right = cols - 1;
	}
}
void DrawIntegralProject(Mat thresh_output,Mat& integral_projection_x,Mat& integral_projection_y,int* sum_h,int* sum_v)
{
	if(!thresh_output.data || !sum_h || !sum_v || !integral_projection_x.data || !integral_projection_y.data)
		return;
	for( int i = 0; i< thresh_output.rows; i++ )
		line( integral_projection_x,cv::Point(0,i),cv::Point(sum_h[i],i),Scalar( 255, 0, 0),2,8,0);
	for( int i = 0; i< thresh_output.cols; i++ )
		line( integral_projection_y,cv::Point(i,0),cv::Point(i,sum_v[i]),Scalar( 255, 0, 0),2,8,0);
	return;
}

Mat FindString::Rotation(Mat in,double angle,double scale)
{
	Mat out;
	//Corr1
	int nRadius, nWidth, nHeight,nHalfWidth,nHalfHeight,nWidth_, nHeight_,nHalfWidth_,nHalfHeight_,nHalfWidth_tmp,nHalfHeight_tmp,nMIN;
	nWidth = in.cols;
	nHeight = in.rows;
	nHalfWidth = nWidth >> 1;
	nHalfHeight = nHeight >> 1;
	nRadius = (int)sqrt(double(nHalfWidth * nHalfWidth + nHalfHeight * nHalfHeight));
	double angle_ ;
	angle_ = atan2(double(nHalfHeight),double(nHalfWidth));
	float fAngle1,fAngle2;
	fAngle1 = float(angle * RATIO + angle_);
	fAngle2 = float(angle * RATIO - angle_);
	nMIN = MIN(nHalfWidth,nHalfHeight);
	nHalfWidth_ = (int)abs(nRadius * cos(fAngle1));//
	nHalfWidth_tmp = (int)abs(nRadius * cos(fAngle2));
	nHalfHeight_ = (int)abs(nRadius * sin(fAngle1));//
	nHalfHeight_tmp = (int)abs(nRadius * sin(fAngle2));//
	nHalfWidth_ = MAX(nHalfWidth,MAX(nHalfWidth_,nHalfWidth_tmp));
	nHalfHeight_ = MAX(nHalfHeight,MAX(nHalfHeight_,nHalfHeight_tmp));
	//nHalfWidth_ = nHalfWidth_ / 4 * 4;
	//nHalfHeight_ = nHalfHeight_ / 4 * 4;
	nWidth_ = nHalfWidth_ << 1;
	nHeight_ = nHalfHeight_ << 1;
	Mat in_corr(nHeight_,nWidth_,CV_8UC3);
	int DeltaX,DeltaY;
	DeltaX = abs(nHalfWidth_ - nHalfWidth);
	DeltaY = abs(nHalfHeight_ - nHalfHeight);
	for(int j = 0; j < nHeight_; j++)
		for(int i = 0; i < nWidth_; i++)
			for(int k = 0; k < 3; k++)
			{
				if(j < DeltaY ||
				   j >= (nHeight_ - DeltaY) ||
				   i < DeltaX ||
				   i >= (nWidth_ - DeltaX))
				   *(in_corr.ptr(j,i) + k) = 255;
				else{
					*(in_corr.ptr(j,i) + k) = *(in.ptr(j - DeltaY,i - DeltaX) + k);
				}
			}
	//imshow("test",in_corr);
	cv::Point center = cv::Point(nHalfWidth_, nHalfHeight_);
	Mat rotationMat(2,3,CV_32FC1);
	rotationMat = getRotationMatrix2D(center,angle,scale);
	warpAffine(in_corr,out,rotationMat,out.size());
	//
#if 0
	if ((out.cols % 4) || (out.rows % 4))
	{
		cv::resize(out, out, cv::Size(in_corr.cols/4*4, in_corr.rows/4*4));
	}
#endif 
	//
	return out;
}

void showHistogram(char* title,Mat input)
{
	int histSize = 256;
	float range[] = {0,256};
	const float* histRange = {range};
	Mat hist;
	////////////////////////////////////////////////
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );
	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
	calcHist(&input,1,0,Mat(),hist,1,&histSize,&histRange,true,false);
	normalize(hist, hist, 0, 1, NORM_MINMAX, -1, Mat() );
	cv::Point start_p,end_p;
	int* array_ = new int[histSize];
	///////////////////////////////////////////////
	for( int i = 1; i < histSize; i++ )
	{
		start_p.x = bin_w*(i-1);
		start_p.y = hist_h - cvRound(hist.at<float>(i-1));
		end_p.x = bin_w*(i);
		end_p.y = hist_h - cvRound(hist.at<float>(i));
		line( histImage,
			  start_p,
		      end_p,
			  Scalar(255, 0, 0),
			  2,
			  8,
			  0 );
		/////
		array_[i] = cvRound(hist.at<float>(i));
	}
	delete []array_;
	imshow(title,histImage);

}
bool needEqualizing(Mat input)
{
	int histSize = 256;
	float range[] = {0,256};
	const float* histRange = {range};
	Mat hist;
	////////////////////////////////////////////////
	int hist_w = 512; int hist_h = 400;
	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar(0,0,0) );
	calcHist(&input,1,0,Mat(),hist,1,&histSize,&histRange,true,false);
	normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	int nonZeroCount = 0;
	for(int i = 1; i < histSize; i++)
		if(cvRound(hist.at<float>(i - 1)) != 0)
			nonZeroCount++;
	if(double(nonZeroCount) / double(histSize) < 0.5)
		return true;
	return false;
}
#define OCR_SHOWARRAY_HEIGHT 400
void showArray_h(char* title,int* array_h,int arraySize)
{
	Mat ip_h(arraySize,OCR_SHOWARRAY_HEIGHT,CV_8UC3);
	if(!array_h)
		return;
	for( int i = 0; i< arraySize; i++ )
		line( ip_h,cv::Point(0,i),cv::Point(array_h[i],i),Scalar( 255, 0, 0),2,8,0);
	imshow(title,ip_h);
	waitKey(4000);
}
void showArray_v(char* title,int* array_v,int arraySize)
{
	Mat ip_v(OCR_SHOWARRAY_HEIGHT,arraySize,CV_8UC3);
	if(!array_v)
		return;
	for( int i = 0; i< arraySize; i++ )
		line( ip_v,cv::Point(i,0),cv::Point(i,array_v[i]),Scalar( 255, 0, 0),2,8,0);
	imshow(title,ip_v);
	waitKey(4000);
}
#define OCR_BINARY_ORNOT 5
bool isBinaryImage(Mat gray)
{
	int histSize = 256;
	float range[] = {0,256};
	const float* histRange = {range};
	Mat hist;
	////////////////////////////////////////////////
	int hist_w = 512; int hist_h = 400;
	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
	calcHist(&gray,1,0,Mat(),hist,1,&histSize,&histRange,true,false);
	normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	int cumulated_amount = 0;
	int firstandlast = 0;
	firstandlast = cvRound(hist.at<float>(0)) + cvRound(hist.at<float>(histSize - 1));
	for(int i = 1; i < histSize - 1; i++)
		cumulated_amount += cvRound(hist.at<float>(i));
	if(cumulated_amount * OCR_BINARY_ORNOT < firstandlast)
		return true;

	return false;
}
void HSV2GRAY(Mat HSV,Mat& Gray)
{
	if(!HSV.data) return;
	uchar* pointer;
	Gray = Mat(HSV.rows,HSV.cols,CV_8UC1);
	for(int j = 0; j < HSV.rows; j++)
		for(int i = 0; i < HSV.cols; i++)
		{
			pointer = HSV.ptr(j,i) + 2;
			*Gray.ptr(j,i) = *pointer;
		}
	return;
}
#define OCR_STRING_RECT_TOP  0.4
#define OCR_STRING_RECT_LEFT 0.15
#define OCR_STRING_RECT_WIDTH 0.7
#define OCR_STRING_RECT_HEIGHT 0.2
void findStrinRectinBinaryImage(Mat thr,int& top,int& bottom,int& left,int& right)
{
	if(thr.empty()) return;
	int *sum_h, *sum_v;
	sum_h = new int[thr.rows];
	sum_v = new int[thr.cols];
	calcProjection(thr,sum_h,sum_v);
	findStringinBinaryImage(sum_h,thr.rows,sum_v,thr.cols,top,bottom,left,right);
	delete []sum_h;
	delete []sum_v;
	return;
}
#define USER_DEFINED_DELTA2 15
#define USER_DEFINED_DELTA3 2

FindString::FindString()
{
	Init();
}

FindString::~FindString()
{
	Init();

}

bool FindString::isEmpty()
{
	if(image.empty() ||
	   image_cv_gray.empty() ||
	   image_hsv.empty() ||
	   image_hjh_gray.empty())
        return true;
    return false;
}
bool FindString::isZoomed()
{
	if(zoomout == 1)
        return true;
    return false;
}
void FindString::Init()
{
	zoomout = 1;
	zoomin = 1;
	flag = false;
	flag_ = false;//true if vertical pattern is stronger than horizontal pattern,false if not
	flag_needEqualization = false;
	m_bIscropped = false;
	image.release();
	image_cv_gray.release();
	image_hsv.release();
	image_hsv_gray.release();
	image_hjh_gray.release();
	image_eroded.release();
	image_eroded_gray.release();
	image_resized.release();
	image_cv_gray_resized.release();
	image_hsv_resized.release();
	image_hsv_gray_resized.release();
	image_hjh_gray_resized.release();
	image_eroded_resized.release();
	image_eroded_gray_resized.release();
	m_goodfeatures.release();
	////////////////////////////////////////
	m_rData.clear();
	subStringList.clear();
	StringList.clear();
	rectlist.clear();
	pointlist.clear();
	///////////////////////////////////////
	return;
}
std::vector<cv::Point2f>  ocr_test_goodfeatures_as_points(Mat in)
{
    std::vector<cv::Point2f> corners;
	if(in.empty() ||
	   in.type() > 1)
		return corners;
	 // Compute good features to track
   cv::goodFeaturesToTrack(in,corners,
      1000,   // maximum number of corners to be returned
      0.1,   // quality level
      1);   // minimum allowed distance between points
    return corners;
}
Mat ocr_test_goodfeatures_3division(Mat in)
{
	if(in.empty() ||
	   in.type() > 1)
		return Mat();
	Mat result = Mat::zeros(cv::Size(in.cols,in.rows), CV_8UC1);
    std::vector<cv::Point2f> pointlist;
	Mat leftwing,body,rightwing;
	Mat(in,cv::Rect(0,0,in.cols / 3,in.rows)).copyTo(leftwing);
	Mat(in,cv::Rect(in.cols / 3,0,in.cols / 3,in.rows)).copyTo(body);
	Mat(in,cv::Rect(in.cols * 2 / 3,0,in.cols / 3,in.rows)).copyTo(rightwing);
    std::vector<cv::Point2f> pl_body,pl_right;
	pl_body = ocr_test_goodfeatures_as_points(body);
	pl_right = ocr_test_goodfeatures_as_points(rightwing);
	pointlist = ocr_test_goodfeatures_as_points(leftwing);
    for (std::vector<cv::Point2f>::iterator	iter = pl_body.begin(); iter != pl_body.end(); iter++)
	{
		cv::Point2f point = *iter;
		pointlist.push_back(cv::Point2f(point.x + in.cols /3,point.y));
	}
    for (std::vector<cv::Point2f>::iterator	iter = pl_right.begin(); iter != pl_right.end(); iter++)
	{
		cv::Point2f point = *iter;
		pointlist.push_back(cv::Point2f(point.x + in.cols * 2 / 3,point.y));
	}
    std::vector<cv::Point2f> corners = pointlist;
    std::vector<cv::Point2f> neighbors;
    std::vector<cv::Point2f>::iterator itc;
#if 0
	//step 2----->removing noise-diameter = 15///-kjy-todo-2015.4.28
	neighbors = corners;
   	itc= corners.begin();
	int thr_dis,thr_con;
	//if(m_bIscropped)
	//{
		thr_dis = 10;
		thr_con = 4;
	//}
	//{
	//	thr_dis = 15;
	//	thr_con = 6;
	//}
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		bool isIsolated = true;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y);
			delta_x *= 1;
			delta_y *= 4;
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < thr_dis)//10.0)//8.0)
				count ++;
		}
		if(count < thr_con)//3)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif
#if 1
	//step 1----->removing noise-diameter = 3///-kjy-todo-2015.4.28
    neighbors = corners;
   	itc= corners.begin();
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y); 
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < 25.0)//8.0)
				count ++;
		}
		if(count < 5)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif
	pointlist = corners;
	/////////////
    for (unsigned int i=0;i<corners.size();++i)
    {
        cv::ellipse(result,corners[i],cv::Size(1,1),0,0,360,cv::Scalar(255),2,8,0);
    }
   threshold(result,result,128,255,THRESH_BINARY_INV);
#ifdef OCR_TEST
   imshow("goodfeatures",result);
   waitKey(1000);
#endif
   return result;
}
Mat ocr_test_goodfeatures(Mat in)
{
	if(in.empty() || in.type() != CV_8UC1)
		return Mat::zeros(cv::Size(in.cols,in.rows), CV_8UC1);
	Mat result = Mat::zeros(cv::Size(in.cols,in.rows), CV_8UC1);
	//in.copyTo(result);
   // Compute good features to track
   std::vector<cv::Point2f> corners;
   cv::goodFeaturesToTrack(in,corners,
      3000,   // maximum number of corners to be returned
      0.05,   // quality level
      1);   // minimum allowed distance between points
   ///removing noise///-kjy-todo-2015.4.28
    std::vector<cv::Point2f> neighbors;
    std::vector<cv::Point2f>::iterator itc;
#if 1
	//step 2----->removing noise-diameter = 15///-kjy-todo-2015.4.28
	neighbors = corners;
   	itc= corners.begin();
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y);
			delta_x *= 1;
			delta_y *= 4;
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < 20.0)//8.0)
				count ++;
		}
		if(count < 4)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif

   for (unsigned int i=0;i<corners.size();++i)
    {
        cv::ellipse(result,corners[i],cv::Size(4,0/*.5*/),0,0,360,cv::Scalar(255),2,8,0);
    }
   threshold(result,result,128,255,THRESH_BINARY_INV);
   imshow("goodfeatures",result);
   waitKey(1000);
   return result;
}

#define OCR_FILTER_UPPER 0.1
#define OCR_FILTER_LOWER 0.2
#define OCR_FILTER_RATIO 0.3
void calcSize_t(int* array_,int arraySize,int& firstindex,int& lastindex)
{
	//blur_(array_,arraySize);
	//Gradient_(array_,arraySize);
	int min = calcMin(array_,arraySize);
	min = min < 0 ? 0: min;
	int max = calcMax(array_,arraySize);
	int mean = (int)(min * 0.95 + max * 0.05);//calcMean(array_,arraySize);//(int)(float(max - min) / 1.4142);//
	int maxindex = int(arraySize / 2) ;//getFirstIndex(array_,arraySize,max);//kjy-todo-2015.3.27
	//int oldfirstindex = getFirstIndex(array_,arraySize,mean - 1),oldlastindex = getLastIndex(array_,arraySize,mean - 1);

	for(int i = maxindex; i < arraySize - 1; i++)
		if((array_[i] >= mean && array_[i + 1] <= mean) || i == arraySize - 1)
		{
			lastindex = i;
			break;
		}
	for(int j = maxindex; j > 0; j--)
		if((array_[j] >= mean && array_[j - 1] <= mean) || j == 1)
		{
			firstindex = j;
			break;
		}
	return;
}
#define OCR_REASONABLE_COUNT 100
#define OCR_FILTER_2 0.5
//kjy-todo-2015.3.21--it's for filtering non-string outputs
void DrawIntegralProject_v(Mat thresh_output,Mat& integral_projection_v,int* sum_v)
{
	if(!thresh_output.data || !sum_v)// || !integral_projection_v.data)
		return;
	for( int i = 0; i< thresh_output.cols; i++ )
		line( integral_projection_v,cv::Point(i,0),cv::Point(i,sum_v[i]),Scalar( 255, 0, 0),2,8,0);
	return;
}
void calcVerticalProjection_t(Mat thresh_output,int* sum_v)
{
	if(!thresh_output.data ||!sum_v) return;
	for( int i = 0; i< thresh_output.cols; i++ )
	{
		sum_v[i] = 0;
		for(int j = 0; j < thresh_output.rows; j++ )
			if(*thresh_output.ptr(j,i) == 0)
				sum_v[i]++;
	}
	return;
}
//kjy-todo-2015.3.21
#define OCR_REASONABLE_STRING_COUNT 3
cv::Rect getLockLocation(cv::Rect rect1,cv::Rect subrect)
{
	int x,y,width,height;
	x = rect1.x + subrect.x;
	y = rect1.y + subrect.y;
	width = subrect.width;
	height = subrect.height;
	return cv::Rect(x,y,width,height);
}

#define OCR_REASONABLE_STRING_HEIGHT_LOWER_BOUND 0.03
#define OCR_REASONABLE_STRING_WIDTH_LOWER_BOUND 0.2
#define OCR_REASONABLE_LOCK_HEIGHT_LOWER_BOUND 0.2//0.01
#define OCR_REASONABLE_LOCK_WIDTH_LOWER_BOUND 0.01
#define OCR_REASONABLE_LOCK_HEIGHT_UPPER_BOUND 0.1
#define OCR_REASONABLE_LOCK_WIDTH_UPPER_BOUND 0.1

Mat FindString::getBinaryImage(cv::Rect rect)
{
#if 1
	Mat cropped;//(original_binary,rect);
	Mat tmp;
	if(!flag_needEqualization)
	{
		//Mat cropped_temp(image_hsv_gray,cv::Rect(rect.x * zoomout,rect.y * zoomout,rect.width * zoomout,rect.height * zoomout));
		resize(image_hsv_gray_resized,tmp,cv::Size(image_size_zoomed.width * 2,image_size_zoomed.height * 2));
		Mat cropped_temp(tmp,cv::Rect(rect.x * 2,rect.y * 2,rect.width * 2,rect.height * 2));
		cropped_temp.copyTo(cropped);
		cropped_temp.release();
	}
	else{
		resize(original_binary,tmp,cv::Size(image_size_zoomed.width * 2,image_size_zoomed.height * 2));
		Mat cropped_temp(tmp,cv::Rect(rect.x * 2,rect.y * 2,rect.width * 2,rect.height * 2));
		cropped_temp.copyTo(cropped);
		cropped_temp.release();
	}
	Mat thr = Mat::zeros(cropped.size(),CV_8UC1);
	windowthreshold_new(cropped,thr);
	Mat adpthr_resized;
	resize(adaptiveThresholded,adpthr_resized,cv::Size(image_size_zoomed.width * 2,image_size_zoomed.height * 2));
	Mat croppedinAdaptivethd(adpthr_resized,cv::Rect(rect.x * 2,rect.y * 2,rect.width * 2,rect.height * 2));
	threshold(croppedinAdaptivethd,croppedinAdaptivethd,128,255,THRESH_BINARY);
	min(croppedinAdaptivethd,thr,thr);
#else
	Mat thr;
	m_cropped_thr.copyTo(thr);
#endif
	removeBoundaryNoise(thr);
	////////////////////////////////////
	Mat original_thr;
	thr.copyTo(original_thr);
	removeNoise_revised(thr,(zoomout == 1 ? false:true));
#ifdef OCR_DEBUG
	imshow("thr",thr);
	imshow("original_thr",original_thr);
	waitKey(10000);
#endif
	/////////////InclCorr///////////////////////
	double	alphaRotate, alphaSkew;
	ProcLinearInterpolation(thr);
	ProcTransformAlpha(thr, alphaRotate, alphaSkew);
	ProcTransformCompensation(thr, alphaRotate, alphaSkew);
	ProcTransformCompensation(original_thr, alphaRotate, alphaSkew);
	//////////////findstring///////////////////////////////////////////////////////////
	int left,right,top = 0,bottom = thr.rows;
	findStrinRectinBinaryImage(thr,top,bottom,left,right);
	////////////////////////////////////////////////////////////////////
	int width = right - left,height = thr.rows - 1;//bottom - top;
	width = (left + width + USER_DEFINED_DELTA3) > thr.cols ? width:(width + USER_DEFINED_DELTA3);
	int refined_left = left,refined_right = right;
	int refined_width = width;
	if(!isBinaryImage(image_cv_gray_resized))
	{
		refined_left -= USER_DEFINED_DELTA2;
		refined_right += USER_DEFINED_DELTA2;
		if(refined_left < 0) refined_left = 0;
		if(refined_right > thr.cols) refined_right = thr.cols;
		refined_width = refined_right - refined_left;
    }
	//////////////////////////////////////////////////////////////////////////////////////////////////
	removeGaussianNoisebyWindow(thr,original_thr,1,10);//20);
	int* sum_h;
	sum_h = new int[thr.rows];
	calcHorizontalProjection(thr,sum_h,OCR_PIXEL_MIN);
	calcSize_t(sum_h,thr.rows,top,bottom);
	delete []sum_h;
	//kjy-todo-2015.4.1
	///////////////kjy-todo-20150314//////////////
	//removeGaussianNoisebyWindow(thr,original_thr,10,3);
	height = bottom - top + 40;
	top -= 20;
	if(top < 0) top = 0;
	if((height + top) > thr.rows)
	{
		height = thr.rows - top - 1;
	}
	Mat output(original_thr,cv::Rect(refined_left < 0 ? 0:refined_left,top,(refined_left + refined_width) > thr.cols ? width:refined_width,height));
	////////////////////////////////////////////////////////////////////
	return output;
}
cv::Rect getLockRemovedRect(Substitute_String substring)
{
	int left = substring.rect.x,
		top = substring.rect.y,
		right = left + substring.rect.width,
		bottom = top + substring.rect.height;
	if(substring.existFirstLock)
		left = substring.firstLock.x + substring.firstLock.width + 1;
	if(substring.existFirstLock)
		right = substring.lastLock.x;
	return cv::Rect(left,top,right - left,bottom - top);
}
void FindString::findStringList(std::vector<Mat>& outputList)
{
	Substitute_String substring;
	Mat output;
    for (std::vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
	{
		substring = *iter;
		output = getBinaryImage(getLockRemovedRect(substring));
		outputList.push_back(output);
#ifdef OCR_DEBUG
		imshow("output",output);
		waitKey(1000);
#endif
	}
	return;
}

void FindString::insertSubString(Substitute_String substring)
{
	subStringList.push_back(substring);
	return;
}
void FindString::getGoodFeaturesAsPoints_new(Mat in)
{
	if(in.empty() || in.type() != CV_8UC1)
		return;
	Mat leftwing,body,rightwing;
	Mat(in,cv::Rect(0,0,in.cols / 2,in.rows)).copyTo(leftwing);
	Mat(in,cv::Rect(in.cols / 2,0,in.cols / 3,in.rows)).copyTo(rightwing);
    std::vector<cv::Point2f> pl_body,pl_right;
	getGoodFeaturesAsPoints(rightwing);
	pl_right = pointlist;
	getGoodFeaturesAsPoints(leftwing);
    for (std::vector<cv::Point2f>::iterator	iter = pl_right.begin(); iter != pl_right.end(); iter++)
	{
		cv::Point2f point = *iter;
		pointlist.push_back(cv::Point2f(point.x + in.cols /2,point.y));
	}
    std::vector<cv::Point2f> corners = pointlist;
    std::vector<cv::Point2f> neighbors;
    std::vector<cv::Point2f>::iterator itc;
#if 0
	if(!m_bIscropped)
	{
	itc= corners.begin();
	while (itc!=corners.end()) {
		cv::Point2f point = *itc;
		if((point.y < in.rows * USER_DEFINED_ERASE_REGION_TOPBOTTOM) ||
			(point.y > in.rows * (1 - USER_DEFINED_ERASE_REGION_TOPBOTTOM)) ||
			(point.x < in.cols * USER_DEFINED_ERASE_REGION_LEFTRIGHT) ||
			(point.x > in.cols * (1 - USER_DEFINED_ERASE_REGION_LEFTRIGHT)))
			itc= corners.erase(itc);
		else
			++itc;
		}
	}
	//step 2----->removing noise-diameter = 15///-kjy-todo-2015.4.28
	neighbors = corners;
   	itc= corners.begin();
	int thr_dis,thr_con;
	if(m_bIscropped)
	{
		thr_dis = 10;
		thr_con = 4;
	}
	else
	{
		thr_dis = 15;
		thr_con = 6;
	}
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		bool isIsolated = true;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y);
			delta_x *= 1;
			delta_y *= 4;
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < thr_dis)//10.0)//8.0)
				count ++;
		}
		if(count < thr_con)//3)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
	//step 1----->removing noise-diameter = 3///-kjy-todo-2015.4.28
    neighbors = corners;
   	itc= corners.begin();
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		bool isIsolated = true;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y); 
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < 20.0)//8.0)
				count ++;
		}
		if(count < 5)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif
	pointlist = corners;
   ////////////////////////////////
   return;
}
///////////////////////////////////////////////////////////////new
void FindString::getGoodFeaturesAsPoints(Mat in)
{
	if(in.empty() || in.type() != CV_8UC1)
		return;
	//
	pointlist.clear();
   // Compute good features to track
   std::vector<cv::Point2f> corners;
   cv::goodFeaturesToTrack(in,corners,
      1000,//500,//   // maximum number of corners to be returned
      0.01,   // quality level
      1);   // minimum allowed distance between points
    ////////////////////////////////////////////////////////////////
    std::vector<cv::Point2f> neighbors;
    std::vector<cv::Point2f>::iterator itc;
#if 1
	if(!m_bIscropped)
	{
	itc= corners.begin();
	while (itc!=corners.end()) {
		cv::Point2f point = *itc;
		if((point.y < in.rows * USER_DEFINED_ERASE_REGION_TOPBOTTOM) ||
			(point.y > in.rows * (1 - USER_DEFINED_ERASE_REGION_TOPBOTTOM)) ||
			(point.x < in.cols * USER_DEFINED_ERASE_REGION_LEFTRIGHT) ||
			(point.x > in.cols * (1 - USER_DEFINED_ERASE_REGION_LEFTRIGHT)))
			itc= corners.erase(itc);
		else
			++itc;
		}
	}
	//step 2----->removing noise-diameter = 15///-kjy-todo-2015.4.28
	neighbors = corners;
   	itc= corners.begin();
	int thr_dis,thr_con;
	if(m_bIscropped)
	{
		thr_dis = 10;
		thr_con = 5;//4;
	}
	else
	{
		thr_dis = 16;//15;
		thr_con = 7;//6;
	}
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y);
			delta_x *= 1;
			delta_y *= 2;
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < thr_dis)//10.0)//8.0)
				count ++;
		}
		if(count < thr_con)//3)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif

#if 1
	if(!m_bIscropped)
	{
	itc= corners.begin();
	while (itc!=corners.end()) {
		cv::Point2f point = *itc;
		if((point.y < in.rows * USER_DEFINED_ERASE_REGION_TOPBOTTOM) ||
			(point.y > in.rows * (1 - USER_DEFINED_ERASE_REGION_TOPBOTTOM)) ||
			(point.x < in.cols * USER_DEFINED_ERASE_REGION_LEFTRIGHT) ||
			(point.x > in.cols * (1 - USER_DEFINED_ERASE_REGION_LEFTRIGHT)))
			itc= corners.erase(itc);
		else
			++itc;
		}
	}
	//step 2----->removing noise-diameter = 15///-kjy-todo-2015.4.28
	neighbors = corners;
   	itc= corners.begin();
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y);
			delta_x *= 1;
			delta_y *= 2;
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < thr_dis)//10.0)//8.0)
				count ++;
		}
		if(count < thr_con)//3)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif
#if 0
	//step 1----->removing noise-diameter = 3///-kjy-todo-2015.4.28
    neighbors = corners;
   	itc= corners.begin();
	while (itc!=corners.end()) {
		//Create bounding rect of object
		cv::Point2f  observingpoint = *itc;
		int count = 0;
        for (std::vector<cv::Point2f>::iterator	iter = neighbors.begin(); iter != neighbors.end(); iter++)
		{
			cv::Point2f neighbor = *iter;
			double delta_x = abs(observingpoint.x - neighbor.x); 
			double delta_y = abs(observingpoint.y - neighbor.y); 
			double distance = sqrt(delta_x * delta_x +delta_y * delta_y);
			if(distance < 20.0)//8.0)
				count ++;
		}
		if(count < 5)//5)
			itc= corners.erase(itc);
		else
			++itc;
	}
#endif
   ////////fill the pointlist and m_goodfeatures(Create an Engine)///////////////////
   pointlist = corners;
   ////////////////////////////////
   return;
}
#define OCR_THR_RATIO_COUNTBYSIZE 0.01
cv::Rect FindString::GetHighDensityRect()
{
	cv::Rect HDR;
	int max = 0;
	double ratio = 0;
    std::vector<cv::Rect>::iterator iter_rect;
	iter_rect = rectlist.begin();
	while(iter_rect != rectlist.end())
	{
    //for (std::vector<cv::Rect>::iterator	iter_rect = rectlist.begin(); iter_rect != rectlist.end(); iter_rect++)
	//{
		int count = 0;
		cv::Rect rect = *iter_rect;
        for (std::vector<cv::Point2f>::iterator	iter_point = pointlist.begin(); iter_point != pointlist.end(); iter_point++)
		{
			Point2f point = *iter_point;
			if(rect.x < point.x &&
			   rect.y < point.y &&
			   point.x < (rect.x + rect.width) &&
			   point.y < (rect.y + rect.height))
			   count++;
		}
		/////////////////////kjy-todo-2015.05.08//////////////////////////
		ratio = double(count) / double(rect.width * rect.height);
		if(ratio < OCR_THR_RATIO_COUNTBYSIZE)
			iter_rect= rectlist.erase(iter_rect);
		else
			++iter_rect;
		////////////////////////////////////////////////////////////
		if(count > max)
		{
			HDR = rect;
			max = count;
		}
		
	}
	return HDR;
}
bool isCropped(Mat in)
{
	if(in.empty())
		return FALSE;
	if(double(in.rows) / double(in.cols + 1) > 2||
	   double(in.cols) / double(in.rows + 1) > 2)
	   return TRUE;
	return FALSE;
}
void ocr_test_harris(Mat in)
{
   // Detect Harris Corners
   cv::Mat cornerStrength;
   cv::cornerHarris(in,cornerStrength,
                3,     // neighborhood size
                3,     // aperture size
                0.01); // Harris parameter
   // threshold the corner strengths
   cv::Mat harrisCorners;
   double threshold= 0.0001; 
   cv::threshold(cornerStrength,harrisCorners,
                 threshold,255,cv::THRESH_BINARY);
   imshow("harrisCorners",harrisCorners);
   return;
}
void Degrading(Mat& in)
{
	if(in.empty() || in.type() == 1)
		return;
	//calc V of image//
	double sum = 0;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			double value = *in.ptr(j,i);
			value += *(in.ptr(j,i) + 1);
			value += *(in.ptr(j,i) + 2);
			value /= (255.0 * 3.0);
			sum += value;
		}
	sum /= double(in.rows * in.cols);
	if(sum < 0.5)
		return;
	//////////////////////////////////////////
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			if(*in.ptr(j,i) > 80)
				*in.ptr(j,i) -= 80;
			else
				*in.ptr(j,i) = 0;
			//////////
			if(*(in.ptr(j,i) + 1) > 80)
				*(in.ptr(j,i) + 1)  -= 80;
			else
				*(in.ptr(j,i) + 1)  = 0;
			/////////////////
			if(*(in.ptr(j,i) + 2)  > 80)
				*(in.ptr(j,i) + 2) -= 80;
			else
				*(in.ptr(j,i) + 2) = 0;
		}
	return;
}
void Upgrade(Mat& in)
{
	if(in.empty() || in.type() == 1)
		return;
	//calc V of image//
	double sum = 0;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			double value = *in.ptr(j,i);
			value += *(in.ptr(j,i) + 1);
			value += *(in.ptr(j,i) + 2);
			value /= (255.0 * 3.0);
			sum += value;
		}
	sum /= double(in.rows * in.cols);
	if(sum > 0.85)
		return;
	//////////////////////////////////////////
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			if(*in.ptr(j,i) >= 155)
				*in.ptr(j,i) = 255;
			else
				*in.ptr(j,i) += 100;
			//////////
			if(*(in.ptr(j,i) + 1) >= 155)
				*(in.ptr(j,i) + 1) = 255;
			else
				*(in.ptr(j,i) + 1)  += 100;
			/////////////////
			if(*(in.ptr(j,i) + 2)  >= 155)
				*(in.ptr(j,i) + 2) = 255;
			else
				*(in.ptr(j,i) + 2) += 100;
		}
	return;
}
Mat ocr_resizeByProjection(Mat in)
{
	Mat goodfeatures = ocr_test_goodfeatures_3division(in);//ocr_test_goodfeatures(in);
	int* sum_v,*sum_h;
	sum_v  = new int[in.cols];
	sum_h  = new int[in.rows];
	calcVerticalProjection(goodfeatures,sum_v,OCR_PIXEL_MIN);
	calcHorizontalProjection(goodfeatures,sum_h,OCR_PIXEL_MIN);
	int left,right,top,bottom;
	findLeftRight(sum_v,in.cols,left,right);
	findTopBottom(sum_h,in.rows,top,bottom);
	delete []sum_v;
	delete []sum_h;
	if(left >= right ||
	   (right - left) < double(goodfeatures.cols) * 0.2||
	   top >= bottom ||
	   (bottom - top) < double(goodfeatures.rows) * 0.2)
		return Mat();
	Mat cropped(in,cv::Rect(left,top,right - left,bottom - top));
	return cropped;
}
cv::Rect ocr_resizeByProjection_as_rect(Mat in)
{
	Mat goodfeatures = ocr_test_goodfeatures_3division(in);//ocr_test_goodfeatures(in);
#ifdef OCR_INTEST
	imshow("goodfeatures",goodfeatures);
	waitKey(1000);
#endif
	int* sum_v,*sum_h;
	sum_v  = new int[in.cols];
	sum_h  = new int[in.rows];
	calcVerticalProjection(goodfeatures,sum_v,OCR_PIXEL_MIN);
	calcHorizontalProjection(goodfeatures,sum_h,OCR_PIXEL_MIN);
	int left,right,top,bottom;
	findLeftRight(sum_v,in.cols,left,right);
	findTopBottom(sum_h,in.rows,top,bottom);
#ifdef OCR_INTEST
	//if()
	{
		line(goodfeatures,Point(right,0),Point(right,goodfeatures.rows), Scalar(0,0,255));
		imshow("goodfeatures",goodfeatures);
		waitKey(1000);
	}
#endif
	delete []sum_v;
	delete []sum_h;
	if(left >= right ||
	   (right - left) < double(goodfeatures.cols) * 0.2||
	   top >= bottom ||
	   (bottom - top) < double(goodfeatures.rows) * 0.2)
		return cv::Rect();
	return cv::Rect(left,top,right - left,bottom - top);
}
Mat ocr_test_canny(Mat in)
{
	if(in.empty() || in.type() > 1)
		return Mat();
	//////////////////////////
	// Apply Canny algorithm
    cv::Mat contours;
	cv::Canny(in,    // gray-level image
             contours, // output contours
             225,      // low threshold
             350);     // high threshold
	cv::Mat contoursInv; // inverted image
    cv::threshold(contours,contoursInv,
                 128,   // values below this
                 255,   // becomes this
                 cv::THRESH_BINARY);//_INV);
	//imshow("canny",contoursInv);
	return contoursInv;
}
Mat FindString::findString_kojy(Mat img_)
{
	if(img_.empty()) 
		return Mat();
	img_.copyTo(m_img_);
	m_bIscropped = isCropped(img_);
	////////////////////////
	image_size = cv::Size(img_.cols,img_.rows);
	//step1---------->zoom in an input image/////////////////////////////////////////////////
	zoomin = cvRound(3096 / img_.cols);
	if(zoomin < 1) 
		zoomin = 1;
	resize(img_,image,cv::Size(img_.cols * zoomin,img_.rows * zoomin));
	erode(image,image_eroded,Mat(),cv::Point(-1,-1),2,1);
	//step2---------->zoom out an input image/////////////////////////////////////////////////
	if(image.rows > image.cols) 
		zoomout = cvRound(image.rows / 450);
	else
		zoomout = cvRound(image.cols / 450);
	if(zoomout < 1)
		zoomout = 1;
	image_size_zoomed = cv::Size(image.cols/zoomout,image.rows/zoomout);
	//step3---------->resize an image////////////////////////////////
	resize(image,image_resized,image_size_zoomed);
	resize(image_eroded,image_eroded_resized,image_size_zoomed);
	//step4---------->create an engine/////////////////////////////////////////////
	//Degrading(image_resized);
	poster_edge = posterEdgefilter(image_resized);
	//equalizeHist(poster_edge,poster_edge);
	
	Mat hongjh;
	image_resized.copyTo(hongjh);
	Convert2Gray(hongjh);
	//////////////////////////////////
	getGoodFeaturesAsPoints(poster_edge);
	m_goodfeatures = Mat::zeros(poster_edge.rows,poster_edge.cols,CV_8UC1);
    for (unsigned int i=0;i<pointlist.size();++i)
    {
        cv::ellipse(m_goodfeatures,pointlist[i],cv::Size(4,1),0,0,360,cv::Scalar(255),1,8,0);
    }
	/////////kojy-todo-2015.5.4/////////////////////////////////////////////////////////////////////////
	//if(!isCropped(image))
	//	eraseMargin(m_goodfeatures);
#ifndef OCR_ONCE
	imshow("m_goodfeatures",m_goodfeatures);
	waitKey(1000);
#endif
	//step5---------->get a list of suspicious string rects///////////////////////////////////////
	getRectList();
	if(rectlist.empty())
		return Mat();

	/**/
	refineRectList();
	/**/
	//step6---------->checkLock
	getSSList();
	//step7---------->pick up the highest-density rect////////////////
	cv::Rect rect = GetHighDensityRect();
	//step8---------->final processing(Form an output)////////////////////////////////////////////////////////
	double ratio = double(zoomout) / double(zoomin);
	Mat test(img_,cv::Rect((int)(rect.x * ratio),(int)(rect.y * ratio),(int)(rect.width * ratio),(int)(rect.height * ratio)));
	//
	double proportional = test.rows / 80.0;
	if(proportional <= 1.0)
		proportional = 1.0;
	Mat test_resized;
	resize(test,test_resized,cv::Size((int)(test.cols/proportional),(int)(test.rows/proportional)));

	Mat cropped;
	test_resized.copyTo(cropped);
	//Degrading(cropped);
	///////////////BLACK & WHITE//////////////////////////////////////////////
#if 0
	Mat bw_t;
	resize(cropped,bw_t,cv::Size(cropped.cols / 3,cropped.rows / 3));
	//Degrading(bw_t);
	cv::Size size = cv::Size(bw_t.cols,bw_t.rows);
	bw_t.copyTo(image_hjh_gray_resized);
	Convert2Gray(image_hjh_gray_resized);
	cvtColor(bw_t,image_hsv_resized,COLOR_BGR2HSV);
	HSV2GRAY(image_hsv_resized,image_hsv_gray_resized);
	//////////////////////////////////////////////////
	tess = Mat::zeros(size, CV_8UC1);
	windowthreshold_new(image_hjh_gray_resized,tess);
	threshold(image_hjh_gray_resized, tess, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
	threshold(tess, tess, 128, 255, THRESH_BINARY);
	windowthreshold_new(image_hsv_gray_resized,tess);
	threshold(tess, tess, 128, 255, THRESH_BINARY);
	//Mat tmp = Mat::zeros(image_size_zoomed, CV_8UC1);
	//windowthreshold(poster_edge,tmp);
	//threshold(tmp, original_binary, 128, 255, THRESH_BINARY);
	//min(tess,original_binary,original_binary);
	imshow("tess",tess);
	waitKey(1000);
#endif

	imshow("cropped",cropped);
#if 0
	Convert2Gray(cropped);
	//adaptiveBilateralFilter(test,cropped,cv::Size(7,7),10.0);
#else
	cropped = posterEdgefilter(test_resized);
#endif
	Mat croppedincropped;
	croppedincropped = cropped;//ocr_resizeByProjection(cropped);//
	if(croppedincropped.empty())
		return Mat();
#ifdef OCR_DEBUG
	imshow("croppedincropped",croppedincropped);
	waitKey(1000);
#endif
	Mat thr = Mat::zeros(croppedincropped.size(),CV_8UC1);
	windowthreshold_new(croppedincropped,thr);
	threshold(thr,thr,128,255,THRESH_BINARY);
	removeBoundaryNoise(thr);

#ifdef OCR_DEBUG
	imshow("thr",thr);
	waitKey(1000);
#endif
	//InclCorr
	double	alphaRotate, alphaSkew;
	ProcLinearInterpolation(thr);
	ProcTransformAlpha(thr, alphaRotate, alphaSkew);
	ProcTransformCompensation(thr, alphaRotate, alphaSkew);
	return thr;
}

#define REASONABLE	15
void FindString::RefineString(Mat& in)
{
	Mat thr;
	threshold(in,thr,128,255,THRESH_BINARY_INV);
	int *sum_h = new int[thr.rows],top,bottom;
	calcHorizontalProjection(thr,sum_h,OCR_PIXEL_MAX);
	findTopBottom(sum_h,thr.rows,top,bottom);

#ifdef IN_DEV
	if((top + LINE_HEIGHT) > bottom &&
	   top > 0 &&
	   bottom < in.rows)
	{
		for(int i = top; i <= bottom; i++)
			sum_h[i] = 0;
		findTopBottom(sum_h,thr.rows,top,bottom);
	}
#endif
	top -= 2;
	bottom += 2;
	if(top < 0)
		top = 0;
	if(bottom >= thr.rows)
		bottom = thr.rows - 1;

	Mat tmp(thr,cv::Rect(0,top,thr.cols,bottom - top));
	int *sum_v = new int[tmp.cols],left,right;
	calcVerticalProjection(tmp,sum_v,OCR_PIXEL_MAX);
	findLeftRight(sum_v,tmp.cols,left,right);

	left -= 2;
	right += 2;
	if(left < 0)
		left = 0;
	if(right >= tmp.cols)
		right = tmp.cols - 1;

	if((bottom - top) * REASONABLE < (right - left)) 
	{
		delete []sum_h;
		delete []sum_v;
		return;
	}

	Mat tmp2(tmp,cv::Rect(left, 0, right - left, tmp.rows));
	threshold(tmp2,in,128,255,THRESH_BINARY_INV);

	delete []sum_v;
	delete []sum_h;

	return;
}
void FindString::getRectList()
{
	int top,left,right,bottom;
	int *sum_h, *sum_v;
	sum_h = new int[m_goodfeatures.rows];
	sum_v = new int[m_goodfeatures.cols];
	for(int i = 0; i < OCR_STRING_COUNT_ESTIMATE; i++)
	{
		if(countNonZero(m_goodfeatures) < OCR_REASONABLE_COUNT)
			break;
		////////Get Top & Bottom (an info about the horizontal location of an assumed StringRect)///////////////////////////
		calcHorizontalProjection(m_goodfeatures,sum_h,OCR_PIXEL_MAX);
		blur_(sum_h,m_goodfeatures.rows);
		lower(sum_h,m_goodfeatures.rows);//added by kojy-20150725
		///
#ifdef OCR_ONCE
		showArray_h("horizontal",sum_h,m_goodfeatures.rows);
#endif
		///
		findTopBottom(sum_h,m_goodfeatures.rows,top,bottom);
		top -= USER_DEFINED__H_MARGIN;
		bottom += USER_DEFINED__H_MARGIN;
		if(top < 0) top = 0;
		if(bottom >= m_goodfeatures.rows) bottom = m_goodfeatures.rows - 1;
		/////////Get Left & Right (an info about the vertical location of an assumed StringRect)///////////////////////////
		Mat croppedbyTopBottom(m_goodfeatures,cv::Rect(0,top,m_goodfeatures.cols,bottom - top));
		calcVerticalProjection(croppedbyTopBottom,sum_v,OCR_PIXEL_MAX);
#ifdef OCR_ONCE
		imshow("cropped",croppedbyTopBottom);
		waitKey(4000);
		showArray_v("TEst",sum_v,croppedbyTopBottom.cols);
#endif
		findLeftRight(sum_v,croppedbyTopBottom.cols,left,right);
		////////////////////////////kjy-todo-2015.05.08///////////
		int adaptvie_margin = (int)(USER_DEFINED__W_MARGIN * (double(bottom - top) / 21.0));
		adaptvie_margin = adaptvie_margin > USER_DEFINED__W_MARGIN ? (int)USER_DEFINED__W_MARGIN:adaptvie_margin;
		left -= adaptvie_margin;
		right += adaptvie_margin;
		////////////////////////////////////////////////
		if(left < 0) left = 0;
		if(right >= m_goodfeatures.cols) right = m_goodfeatures.cols - 1;
		/////////Form the rect of the suspicious StringRect///////////////////////////////
		cv::Rect rect = cv::Rect(left,top,right - left,bottom - top);
		/////////Reflect the Engine//////////////////////////////////////////////////////////
		for(int j = top; j < bottom; j++)
			for(int i = 0; i < m_goodfeatures.cols; i++)
			{
				*m_goodfeatures.ptr(j,i) = 0;
			}
		//////////////////////////////////////////////////
		if(top == 0 &&
		   bottom == (m_goodfeatures.rows - 1))
		   continue;
		if(rect.width < 1 ||
			rect.height < 1)
			continue;
		if(double(rect.width) / double(rect.height + 1) < 2)
			continue;
#ifdef OCR_ONCE
		Mat cropped(image_resized,rect);
		imshow("cropped",cropped);
		waitKey(1000);
#endif
		rectlist.push_back(rect);
	}
	delete []sum_v;
	delete []sum_h;
	return;
}


void fillinside(Mat& in)
{
	if(in.empty() || in.type() > 1)
		return;
#if 0
#if 1
	threshold(in,in,128,255,THRESH_BINARY_INV);
	dilate(in,in,Mat());
	//dilate(in,in,Mat());
	//dilate(in,in,Mat());
	threshold(in,in,128,255,THRESH_BINARY_INV);
#else
	erode(in,in,Mat());
	threshold(in,in,128,255,THRESH_BINARY);
#endif
#endif
#if 0
	Mat black = Mat::zeros(in.rows,in.cols,CV_8UC1);
	int first = 0,last = 0;
	bool exist_f = false,exist_l = false;
	for(int j = 1; j < in.rows - 1; j++)
	{
		for(int i = 0; i < in.cols; i++)
		{
			exist_f = false;
			if(*in.ptr(j,i) == 0 ||
			   *in.ptr(j - 1,i) == 0 ||
			   *in.ptr(j + 1,i) == 0)
			{
				first = i;
				exist_f = true;
				break;
			}
		}
		for(int i = (in.cols - 1); i <= 0; i--)
		{
			exist_l = false;
			if(*in.ptr(j,i) == 0 ||
			   *in.ptr(j - 1,i) == 0 ||
			   *in.ptr(j + 1,i) == 0)
			{
				last = i;
				exist_l = true;
				break;
			}
		}
		if(first != last)
			line(black,cv::Point(first,j),cv::Point(last,j),cv::Scalar(255),1.5);
	}
	for(int i = 1; i < in.cols - 1; i++)
	{
		for(int j = 0; j < in.rows; j++)
		{
			exist_f = false;
			if(*in.ptr(j,i) == 0 ||
			   *in.ptr(j,i - 1) == 0 ||
			   *in.ptr(j,i + 1) == 0 )
			{
				first = j;
				exist_f = true;
				break;
			}
		}
		for(int j = (in.rows - 1); j <= 0; j--)
		{
			exist_l = false;
			if(*in.ptr(j,i) == 0 ||
			   *in.ptr(j,i - 1) == 0 ||
			   *in.ptr(j,i + 1) == 0 )
			{
				last = j;
				exist_l = true;
				break;
			}
		}
		if(first != last)
			line(black,cv::Point(i,first),cv::Point(i,last),cv::Scalar(255),1.5);
	}
	threshold(black,black,128,255,THRESH_BINARY_INV);
	black.copyTo(in);
#endif
	return;
}
void removeabove100pixels(Mat& binary,Mat color)
{
	if(color.empty() || color.type() != CV_8UC3)
		return;
	for(int j = 0; j < color.rows; j++)
		for(int i = 0; i < color.cols; i++)
		{
			if(*color.ptr(j,i) > 100 ||
			   *(color.ptr(j,i) + 1) > 100 ||
			   *(color.ptr(j,i) + 2)> 100 )
			   *binary.ptr(j,i) = 255;
		}
}

bool resizeImg(Mat& img)
{
	Mat tmp;
	Mat tmp_reduced;
	img.copyTo(tmp);
	///
	removeBoundaryNoise(tmp);
	tmp.copyTo(tmp_reduced);
	///
	int left, right, top, bottom;
	for(int i = 3; i < 15; i += 2)
		removeGaussianNoiseByWindow(tmp_reduced,255,0,i);
	//
	int* array_h,* array_v;
	array_h = new int[tmp_reduced.rows];
	array_v = new int[tmp_reduced.cols];
	calcVerticalProjection(tmp_reduced,array_v,0);
	calcHorizontalProjection(tmp_reduced,array_h,0);
	findStringinBinaryImage(array_h,tmp_reduced.rows,array_v,tmp.cols,top,bottom,left,right);
	//calcSize_t(array_h,tmp.rows,top,bottom);
	delete []array_h;
	delete []array_v;
	//
	if(bottom - top <= 0 || right - left <= 0)
		return false;
	Mat cropped(tmp,cv::Rect(left,top,right-left,bottom-top));
	if(cropped.rows * 3 > cropped.cols)
		return false;
#ifdef OCR_TEST
	imshow("tmp_reduced",tmp_reduced);
	imshow("cropped",cropped);
	waitKey(1000);
#endif
	img = cropped;
	return true;
}
#define OCR_PLUS 0.5
void FindString::getlocks_new(cv::Rect rect,Mat& string,Mat& firstL,Mat& lastL,cv::Rect& fll,cv::Rect& lll)
{
	if(rect.width == 0||
		rect.height == 0)
		return;

	double ratio = double(zoomout) / double(zoomin);
	//
	Mat test(m_img_,cv::Rect((int)(rect.x * ratio),(int)(rect.y * ratio - OCR_PLUS),(int)(rect.width * ratio),(int)((rect.height + 2 * OCR_PLUS) * ratio)));//-->original
	//Mat test(m_jon,cv::Rect(rect.x * ratio,(rect.y * ratio - OCR_PLUS),rect.width * ratio,(rect.height + 2 * OCR_PLUS) * ratio));//-replaced
#ifdef OCR_TEST
	imshow("TEST",test);
	waitKey(1000);
#endif
	double proportional = test.rows / 160.0;//80
	if(proportional <= 1.0)
		proportional = 1.0;
	Mat test_resized;//(test);
	resize(test,test_resized,cv::Size((int)(test.cols / proportional),(int)(test.rows / proportional)));
	Mat cropped;
	test_resized.copyTo(cropped);

	///////////////BLACK & WHITE//////////////////////////////////////////////
	Mat bw_t = cropped;
	double comp_ratio = 1.0;//1.0
	resize(cropped,bw_t,cv::Size((int)(cropped.cols / comp_ratio),(int)(cropped.rows / comp_ratio)));
	cv::Size size = cv::Size(bw_t.cols,bw_t.rows);
	//
#if 1
	Mat pe = posterEdgefilter(bw_t);//original
#else
	Mat pe = posterEdgefilter(posterEdgefilterJON(bw_t));//replaced
#endif

	tess = Mat::zeros(size, CV_8UC1);
	windowthreshold_new(pe,tess);
	threshold(tess, tess, 128, 255, THRESH_BINARY);
	Mat roi_;//(tess,rect);
	tess.copyTo(roi_);
	removeBoundaryNoise(roi_);
	//-added
	//removeabove100pixels(roi_,bw_t);
	//-by kjy 6.28
	///InclCorr-process
	double	alphaRotate, alphaSkew;
	ProcLinearInterpolation(roi_);
	ProcTransformAlpha(roi_, alphaRotate, alphaSkew);
#if 1//kojy-todo-save Rotation data-20151121-for 90%
	m_rData.push_back(alphaRotate);
#endif
	ProcTransformCompensation(roi_, alphaRotate, alphaSkew);
	//
	string = roi_;
//    imwrite("/home/joonyong/Downloads/before.jpg",string);
#ifdef OCR_TEST
	bool isStringbySize = resizeImg(string);
	if(!isStringbySize)
		return;


#endif	
	////imshow("string",string);
	////waitKey(1000);
#if 0
	comp_ratio = 1.0;//1.0
	Mat string_tmp;
	resize(string,string_tmp,cv::Size((int)(string.cols / comp_ratio),(int)(string.rows / comp_ratio)));
	string_tmp.copyTo(string);
#endif
	//
	size.width = string.cols;
	size.height = string.rows;
	//Search-process
    int left,right,top = 0,height = string.rows,width = string.cols;
	int top_,bottom_;
	int* sum_v,* sum_h;;
	sum_v = new int[width];
	sum_h = new int[height];
	calcVerticalProjection(string,sum_v,OCR_PIXEL_MIN);
	lower(sum_v,string.cols);
	//blur_(sum_v,string.cols);
#ifdef OCR_DEBUG
	showArray_v("v",sum_v,string.cols);//
#endif
	cv::Rect fl1,fl2,ll1,ll2;
	Mat croppedfirstlock,croppedlastlock;
	getFirstLock(sum_v,string.cols,left,right);
	///kjy-todo-2015.4.28//////
	if(left < right)
	{
		fl1 = cv::Rect(left,top,right - left,height);
#if 1
		Mat firstLock(string,fl1);
#else
		Mat firstLock;
		Mat(pe, fl1).copyTo(firstLock);
#endif
		threshold(firstLock,firstLock,128,255,THRESH_BINARY);
	///////////////////removing background///////////////////
		//
		removeBoundaryNoise(firstLock);
		//
		calcHorizontalProjection(firstLock,sum_h,OCR_PIXEL_MIN);
		findLeftRight(sum_h,height,top_,bottom_);
		if(top_ < bottom_)
		{
			fl2 = cv::Rect(0,top_,firstLock.cols,bottom_ - top_);
			Mat croppedfirst_lock(firstLock,fl2);
			croppedfirst_lock.copyTo(croppedfirstlock);
#ifdef OCR_TEST
			imshow("croppedfirstlock",croppedfirstlock);
			waitKey(1000);
#endif
		}
	}
	////////////////////////////////////
	getLastLock(sum_v,string.cols,left,right);
	if(left < right)
	{
		ll1 = cv::Rect(left,top,right - left,string.rows);
#if 1
		Mat lastLock(string,ll1);
#else
		Mat lastLock;
		Mat(pe,ll1).copyTo(lastLock);
#endif
		threshold(lastLock,lastLock,128,255,THRESH_BINARY);
		//
		removeBoundaryNoise(lastLock);
		//
		calcHorizontalProjection(lastLock,sum_h,OCR_PIXEL_MIN);
		findLeftRight(sum_h,height,top_,bottom_);
		if(top_ < bottom_)
		{
			ll2 = cv::Rect(0,top_,lastLock.cols,bottom_ - top_);
			Mat croppedlast_lock(lastLock,ll2);
			croppedlast_lock.copyTo(croppedlastlock);
#ifdef OCR_TEST
			imshow("croppedlastlock",croppedlastlock);
			waitKey(1000);
#endif
		}
	}
	//////////////////////////////////
#ifdef OCR_NEVER
	imshow("croppedfirstlock",croppedfirstlock);
	imshow("croppedlastlock",croppedlastlock);
	waitKey(1000);
#endif
	if(croppedfirstlock.rows > OCR_REASONABLE_LOCK_HEIGHT_LOWER_BOUND * double(size.height) &&
	   croppedfirstlock.cols > OCR_REASONABLE_LOCK_WIDTH_LOWER_BOUND * double(size.width) &&
	   croppedfirstlock.cols < OCR_REASONABLE_LOCK_WIDTH_UPPER_BOUND * double(size.width) &&
	    (croppedfirstlock.rows / (croppedfirstlock.cols + 1) < 7) &&
		(croppedfirstlock.cols / (croppedfirstlock.rows + 1) < 7) &&
		croppedfirstlock.rows > 4 && croppedfirstlock.cols > 4)
	{
		fillinside(croppedlastlock);
		resize(croppedfirstlock,firstL,cv::Size(croppedfirstlock.cols * 2,croppedfirstlock.rows * 2));
		threshold(firstL,firstL,128,255,THRESH_BINARY);
		fll = fl1;//getLockLocation(rect,fl1);
	}
	if(croppedlastlock.rows > OCR_REASONABLE_LOCK_HEIGHT_LOWER_BOUND * double(size.height) &&
	   croppedlastlock.cols > OCR_REASONABLE_LOCK_WIDTH_LOWER_BOUND * double(size.width) &&
	   croppedlastlock.cols < OCR_REASONABLE_LOCK_WIDTH_UPPER_BOUND * double(size.width) &&
	   (croppedlastlock.rows / (croppedlastlock.cols + 1) < 4) &&
	   (croppedlastlock.cols / (croppedlastlock.rows + 1) < 4) &&
	   croppedlastlock.rows > 4 && croppedlastlock.cols > 4)
	{
		fillinside(croppedlastlock);
		resize(croppedlastlock,lastL,cv::Size(croppedlastlock.cols * 2,croppedlastlock.rows * 2));
		threshold(lastL,lastL,128,255,THRESH_BINARY);
		lll = ll1;//getLockLocation(rect,ll1);
	}
	////////////////////
	delete []sum_h;
	delete []sum_v;

	return;
}
void SortByHeight(std::vector<Substitute_String>& sublist)
{
	if(sublist.size() == 0)
		return;
    std::vector<Substitute_String> tmp;
    std::vector<Substitute_String>::iterator itc_tmp;
	tmp = sublist;
	sublist.clear();
	int stringcount = tmp.size();
	
	for(int i = 0; i < stringcount; i++)
	{
		int min = 100000;
        for (std::vector<Substitute_String>::iterator	iter = tmp.begin(); iter != tmp.end(); iter++)
		{
			Substitute_String substring = *iter;
			if(substring.rect.y < min)
			{
				min = substring.rect.y;
				itc_tmp = iter;
			}
		}
		sublist.push_back(*itc_tmp);
		tmp.erase(itc_tmp);
	}
	
	return;
}
void FindString::getSSList()
{
	if(rectlist.empty())	
		return;
	////
	//std::vector<Substitute_String> tmp;
	////
    for (std::vector<cv::Rect>::iterator	iter = rectlist.begin(); iter != rectlist.end(); iter++)
	{
		cv::Rect rect = *iter;
		if(rect.width == 0 ||
			rect.height == 0)
			continue;
		Mat string;
		Mat firstMark,lastMark;
		cv::Rect fmLocation,lmLocation;
		getlocks_new(rect,string,firstMark,lastMark,fmLocation,lmLocation);
		if((fmLocation.width == 0 || fmLocation.height == 0) &&
			(lmLocation.width == 0 || lmLocation.height == 0))
			continue;
		OCR_Substitute_String substring(rect,string,firstMark,lastMark,fmLocation,lmLocation);
		subStringList.push_back(substring);
	}
	//SortByHeight(subStringList);

	//SortByLocks'relation
	return;
}
void FindString::refineRectList()
{
	if(rectlist.empty())	
		return;	
    std::vector<cv::Rect> tmp;
    for (std::vector<cv::Rect>::iterator	iter = rectlist.begin(); iter != rectlist.end(); iter++)
	{
		cv::Rect rect = *iter;
		Mat cropped(image_resized,rect);
		Mat gray;
#ifdef OCR_INTEST//kojy
		gray = posterEdgefilter(posterEdgefilterJON(cropped));
#else
		gray = posterEdgefilter(cropped);
#endif
		cv::Rect subrect = ocr_resizeByProjection_as_rect(gray);
		tmp.push_back(cv::Rect(rect.x + subrect.x,rect.y + subrect.y,subrect.width,subrect.height));
	}
	rectlist.clear();
	rectlist = tmp;
	return;
}
Mat FindString::removeLocks(OCR_Substitute_String& substring)
{
	if(substring.thr.empty())
		return Mat();
	int left = 0, right = substring.thr.cols;
	if(substring.existFirstLock)
		left = substring.firstLock.x + substring.firstLock.width + 1;
	if(substring.existLastLock)
		right = substring.lastLock.x;
	if(left >= right)
	{
		left = 0;
		right = substring.thr.cols;
	}
	return Mat(substring.thr,cv::Rect(left,0,right - left,substring.thr.rows));
}
void ocr_canny_goodfeatures_filter(Mat canny,Mat& goodfeatures)
{
	if(canny.empty() ||
		canny.type() > 1||
		goodfeatures.empty() ||
		goodfeatures.type() > 1 ||
		canny.rows != goodfeatures.rows ||
		canny.cols != goodfeatures.cols)
		return;
	
	for(int j = 0; j < canny.rows; j++)
	{
		int nonzerocount = 0;
		for(int i = 0; i < canny.cols; i++)
		{
			if(*canny.ptr(j,i) != 0)
				nonzerocount++;
		}
		if(nonzerocount == 0)
			for(int i = 0; i < canny.cols; i++)
				*goodfeatures.ptr(j,i) = 0;
	}
	return;
}
void FindString::GetWhatYouWantFirst(Mat img_)
{
    //qDebug("inGWYWF");
	if(img_.empty()) 
		return;
    //qDebug("to be about to do GWYWF!");
#define USE_POSTER
#ifdef USE_POSTER
	m_jon = posterEdgefilterJON(img_);//-added
	//m_jon = posterEdgefilterJON(m_jon);//-added
	//m_jon = posterEdgefilterJON(m_jon);//-added
	m_jon.copyTo(m_img_);
#else
	img_.copyTo(m_img_);
#endif
	m_bIscropped = isCropped(m_img_);
	////////////////////////
	image_size = cv::Size(m_img_.cols,m_img_.rows);
	//step1---------->zoom in an input image/////////////////////////////////////////////////
	zoomin = cvRound(3096 / m_img_.cols);
	if(zoomin < 1) 
		zoomin = 1;
	resize(m_img_,image,cv::Size(m_img_.cols * zoomin,m_img_.rows * zoomin));
	erode(image,image_eroded,Mat(),cv::Point(-1,-1),2,1);
    //qDebug("step1 finished");
	//step2---------->zoom out an input image/////////////////////////////////////////////////
	if(image.rows > image.cols) 
		zoomout = cvRound(image.rows / 450);
	else
		zoomout = cvRound(image.cols / 450);
	if(zoomout < 1)
		zoomout = 1;
	image_size_zoomed = cv::Size(image.cols/zoomout,image.rows/zoomout);
    //qDebug("step2 finised!");
	//step3---------->resize an image////////////////////////////////
	resize(image,image_resized,image_size_zoomed);
	resize(image_eroded,image_eroded_resized,image_size_zoomed);
    //qDebug("step3");
	//step4---------->create an engine/////////////////////////////////////////////
	//Degrading(image_resized);
	poster_edge = posterEdgefilter(image_resized);
#ifdef OCR_TEST
	imshow("posterE",poster_edge);
	waitKey(4000);
#endif
	getGoodFeaturesAsPoints(poster_edge);
	m_goodfeatures = Mat::zeros(poster_edge.rows,poster_edge.cols,CV_8UC1);
    for (unsigned int i=0;i<pointlist.size();++i)
    {
        cv::ellipse(m_goodfeatures,pointlist[i],cv::Size(4,1),0,0,360,cv::Scalar(255),1,8,0);
    }
#ifndef OCR_TEST
	Mat test_kjy = Mat::zeros(poster_edge.rows,poster_edge.cols,CV_8UC1);
	memset(test_kjy.data, 255, poster_edge.rows * poster_edge.cols);
    for (unsigned int i=0;i<pointlist.size();++i)
    {
        cv::ellipse(test_kjy,pointlist[i],cv::Size(4,1),0,0,360,cv::Scalar(0),1,8,0);
		//*test_kjy.ptr(int(pointlist[i].y), int(pointlist[i].x)) = 0;
    }
    //imwrite("/home/joonyong/Downloads/goodf.jpg",test_kjy);
	//cvtColor(test_kjy,test_kjy,CV_BGR2GRAY);
	//threshold(test_kjy,test_kjy,128,255,THRESH_BINARY_INV);
	double	alphaSkew;
	ProcLinearInterpolation(test_kjy);
	ProcTransformAlpha(test_kjy, m_Rotation, alphaSkew);
	////imshow("posterE",test_kjy);
	////waitKey(1000);
#endif
	//step5---------->get a list of suspicious string rects///////////////////////////////////////
	getRectList();
	if(rectlist.empty())
		return;
	/**/
	//step6---------->refine suspicious string rects///////////////////////////////////////
	refineRectList();
	/**/
	//step7---------->get a list of suspicious strings///////////////////////////////////////
	getSSList();
	return;
}

void FindString::GetOutputList(Mat image, vector<Mat> &outputList)
{
	vector<Substitute_String> subStringList;
	subStringList = GetSubstituteStringList(image);

	Substitute_String substring;
	for (vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
	{
		substring = *iter;
		if(CheckLockSymbol_Revised(g_pDicData, substring.firstMark.data,substring.firstMark.rows,substring.firstMark.cols))
			substring.existFirstLock = true;
		if(CheckLockSymbol_Revised(g_pDicData, substring.lastMark.data,substring.lastMark.rows,substring.lastMark.cols))
			substring.existLastLock = true;

		if(substring.existFirstLock || substring.existLastLock)
			outputList.push_back(removeLocks(substring));
	}
	return;
}
double Recommended_Angle[8] = {0.0872,0.1744,0.2617,0.3488,0.4363,0.5233,0.6105,0.6977};
void FindString::RefineRotationData()
{
	BOOL clock_wised = FALSE;
	int count_clw = 0,count_uclw = 0;//clockwise,unclockwise
	/////////////////////////////////////
    vector<double>::iterator itc;
	itc= m_rData.begin();
	while (itc!=m_rData.end()) {
		//if(abs(*itc) < 0.02)
		//{
		//	itc= m_rData.erase(itc);
		//}
		//else
		//{
			if(double(*itc) < 0)
				count_clw++;
			else
				count_uclw++;
			++itc;
		//}
	}
	///////////delete unnecessary///////////////////////////
	int flag = m_Rotation > 0 ? 1 : -1;//(count_clw > count_uclw) ? -1 : 1;
	double prev = -1;
#if 0
	itc= m_rData.begin();
	while (itc!=m_rData.end()) {
		int clw = *itc < 0 ? -1 : 1;
		if(clw != flag || abs(prev - *itc) < 0.03)
			itc = m_rData.erase(itc);
		else
			++itc;
		prev = *itc;
	}
#endif
	////////////////////////////////////////////////
	for(int i = 0; i < 8; i++)
	{
		if(Recommended_Angle[i] < abs(m_Rotation))
			continue;
		bool available = true;
		for(vector<double>::iterator itc = m_rData.begin(); itc != m_rData.end(); itc++)
			if(abs(*itc - flag * Recommended_Angle[i]) < 0.03)
				available = false;
		if(available)
			m_rData.push_back(flag * Recommended_Angle[i]);
	}
}

void FindString::GetOutputListEx(Mat image, vector<Mat>& outputList)
{
	Mat rotated;
	vector<Substitute_String> SSL;
	SSL = GetSubstituteStringList(image);
	////////////////////////////////////////////////////////////////////////////
	RefineRotationData();
	vector<double> rData = GetRotationData();

	int i = 0;
	FindString Next;
	do
	{
		Substitute_String substring;
		for (vector<Substitute_String>::iterator	iter = SSL.begin(); iter != SSL.end(); iter++)
		{
			substring = *iter;
			if(CheckLockSymbol_Revised(g_pDicData, substring.firstMark.data,substring.firstMark.rows,substring.firstMark.cols))
				substring.existFirstLock = true;
			if(CheckLockSymbol_Revised(g_pDicData, substring.lastMark.data,substring.lastMark.rows,substring.lastMark.cols))
				substring.existLastLock = true;

			if(substring.existFirstLock || substring.existLastLock)
				outputList.push_back(removeLocks(substring));
		}
		if(outputList.size() > 0)
			return;//break;
		SSL.clear();
			
		double Angle = rData[i] / double(RATIO);
		rotated = Rotation(image,Angle);
		SSL = Next.GetSubstituteStringList(rotated);
		i++;
	}
	while(i < (int)rData.size());
}

void FindString::GetLockList(Mat image,vector<Mat>& locklist)
{
	GetSubstituteStringList(image);
	for (vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
	{
		Substitute_String substring = *iter;
		if(!substring.firstMark.empty())
			locklist.push_back(substring.firstMark);
		if(!substring.lastMark.empty())
			locklist.push_back(substring.lastMark);
	}
	return;
}
void FindString::GetLockListEx(Mat image,vector<Mat>& locklist)
{
	vector<Substitute_String> SSL;
	SSL = GetSubstituteStringList(image);
	RefineRotationData();
	vector<double> rData = GetRotationData();
	int i = 0;
	FindString Next;
	do{
		for (vector<Substitute_String>::iterator	iter = SSL.begin(); iter != SSL.end(); iter++)
		{
			Substitute_String substring = *iter;
			if(!substring.firstMark.empty())
				locklist.push_back(substring.firstMark);
			if(!substring.lastMark.empty())
				locklist.push_back(substring.lastMark);
		}
		Mat rotated;
		double Angle = rData[i] / double(RATIO);
		rotated = Rotation(image,Angle);
		SSL = Next.GetSubstituteStringList(rotated);
		i++;
	}
	while(i < (int)rData.size());
	return;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void Refinebyfeatures(Mat in,Mat& out)
{
#if 0
	int top,left,right,bottom;
	int *sum_h, *sum_v;
	sum_h = new int[in.rows];
	sum_v = new int[in.cols];

	if(countNonZero(in) < OCR_REASONABLE_COUNT)
		return;
	////////Get Top & Bottom (an info about the horizontal location of an assumed StringRect)///////////////////////////
	calcHorizontalProjection(in,sum_h,OCR_PIXEL_MAX);
	blur_(sum_h,in.rows);
	lower(sum_h,in.rows);//added by kojy-20150725

	findTopBottom(sum_h,in.rows,top,bottom);

	//top -= USER_DEFINED__H_MARGIN;
	//bottom += USER_DEFINED__H_MARGIN;
	if(top < 0) top = 0;
	if(bottom >= in.rows) bottom = in.rows - 1;
	/////////Get Left & Right (an info about the vertical location of an assumed StringRect)///////////////////////////
	Mat croppedbyTopBottom(in,cv::Rect(0,top,in.cols,bottom - top));
	calcVerticalProjection(croppedbyTopBottom,sum_v,OCR_PIXEL_MAX);

	findLeftRight(sum_v,croppedbyTopBottom.cols,left,right);
	////////////////////////////kjy-todo-2015.05.08///////////
	//int adaptvie_margin = (int)(USER_DEFINED__W_MARGIN * (double(bottom - top) / 21.0));
	//adaptvie_margin = adaptvie_margin > USER_DEFINED__W_MARGIN ? (int)USER_DEFINED__W_MARGIN:adaptvie_margin;
	//left -= adaptvie_margin;
	//right += adaptvie_margin;
	////////////////////////////////////////////////
	if(left < 0) left = 0;
	if(right >= in.cols) right = in.cols - 1;

	if( 2 * (bottom - top ) < in.rows|| 2 * (right - left) <  in.cols)
	{
		delete []sum_v;
		delete []sum_h;
		return;
	}
	/////////Form the rect of the suspicious StringRect///////////////////////////////
	cv::Rect rect = cv::Rect(left,top,right - left,bottom - top);

	Mat cropped(out,rect);
#ifndef OCR_ONCE
	
	imshow("cropped",cropped);
	waitKey(1000);
#endif
	cropped.copyTo(out);

	delete []sum_v;
	delete []sum_h;
	return;
#else
	Mat thr;
	threshold(in,thr,128,255,THRESH_BINARY_INV);
	int *sum_h = new int[thr.rows],top,bottom;
	calcHorizontalProjection(thr,sum_h,OCR_PIXEL_MAX);
	findTopBottom(sum_h,thr.rows,top,bottom);

#ifdef IN_DEV
	if((top + LINE_HEIGHT) > bottom &&
	   top > 0 &&
	   bottom < in.rows)
	{
		for(int i = top; i <= bottom; i++)
			sum_h[i] = 0;
		findTopBottom(sum_h,thr.rows,top,bottom);
	}
#endif
	top -= 2;//USER_DEFINED__H_MARGIN;
	bottom += 2;//USER_DEFINED__H_MARGIN;
	if(top < 0)
		top = 0;
	if(bottom >= thr.rows)
		bottom = thr.rows - 1;

	Mat tmp(thr,cv::Rect(0,top,thr.cols,bottom - top));
	int *sum_v = new int[tmp.cols],left,right;
	calcVerticalProjection(tmp,sum_v,OCR_PIXEL_MAX);
	findLeftRight(sum_v,tmp.cols,left,right);

	left -= 2;//USER_DEFINED__W_MARGIN;
	right += 2;//USER_DEFINED__W_MARGIN;
	if(left < 0)
		left = 0;
	if(right >= tmp.cols)
		right = tmp.cols - 1;

	if((bottom - top) * REASONABLE < (right - left)) 
	{
		delete []sum_h;
		delete []sum_v;
		return;
	}

	Mat tmp_(out,cv::Rect(left, top, right - left, bottom - top));
	tmp_.copyTo(out);
	/*threshold(tmp_,out,128,255,THRESH_BINARY_INV);*/
	imshow("cropped",out);
	waitKey(1000);
	delete []sum_v;
	delete []sum_h;

	return;
#endif
}
BOOL existsLockSymbol(Mat& roi_)
{
	/////////////////////////////////////////////////////////
	BOOL hasHeadLock = FALSE;
	BOOL hasTailLock = FALSE;
	/////////////////////////////////////////////
	Mat string = roi_;
	cv::Size size;
	size.width = string.cols;
	size.height = string.rows;
	//Search-process
    int left,right,top = 0,bottom,height = string.rows,width = string.cols;
	int top_,bottom_;
	int* sum_v,* sum_h;;
	sum_v = new int[width];
	sum_h = new int[height];
	calcVerticalProjection(string,sum_v,OCR_PIXEL_MIN);
	lower(sum_v,string.cols);

#ifdef OCR_DEBUG
	showArray_v("v",sum_v,string.cols);//
#endif
	cv::Rect fl1,fl2,ll1,ll2;
	Mat croppedfirstlock,croppedlastlock;
	getFirstLock(sum_v,string.cols,left,right);
	///kjy-todo-2015.4.28//////
	if(left < right)
	{
		fl1 = cv::Rect(left,top,right - left,height);
#if 1
		Mat firstLock(string,fl1);
#else
		Mat firstLock;
		Mat(pe, fl1).copyTo(firstLock);
#endif
		threshold(firstLock,firstLock,128,255,THRESH_BINARY);
		///////////////////removing background///////////////////
		removeBoundaryNoise(firstLock);
		//
		calcHorizontalProjection(firstLock,sum_h,OCR_PIXEL_MIN);
		findLeftRight(sum_h,height,top_,bottom_);
		if(top_ < bottom_)
		{
			fl2 = cv::Rect(0,top_,firstLock.cols,bottom_ - top_);
			Mat croppedfirst_lock(firstLock,fl2);
			croppedfirst_lock.copyTo(croppedfirstlock);
		}
	}
	////////////////////////////////////
	getLastLock(sum_v,string.cols,left,right);
	if(left < right)
	{
		ll1 = cv::Rect(left,top,right - left,string.rows);
#if 1
		Mat lastLock(string,ll1);
#else
		Mat lastLock;
		Mat(pe,ll1).copyTo(lastLock);
#endif
		threshold(lastLock,lastLock,128,255,THRESH_BINARY);
		//
		removeBoundaryNoise(lastLock);
		//
		calcHorizontalProjection(lastLock,sum_h,OCR_PIXEL_MIN);
		findLeftRight(sum_h,height,top_,bottom_);
		if(top_ < bottom_)
		{
			ll2 = cv::Rect(0,top_,lastLock.cols,bottom_ - top_);
			Mat croppedlast_lock(lastLock,ll2);
			croppedlast_lock.copyTo(croppedlastlock);
#ifdef OCR_TEST
			imshow("croppedlastlock",croppedlastlock);
			waitKey(1000);
#endif
		}
	}
	//////////////////////////////////
#ifdef OCR_NEVER
	imshow("croppedfirstlock",croppedfirstlock);
	imshow("croppedlastlock",croppedlastlock);
	waitKey(1000);
#endif
	if(croppedfirstlock.rows > OCR_REASONABLE_LOCK_HEIGHT_LOWER_BOUND * double(size.height) &&
	   croppedfirstlock.cols > OCR_REASONABLE_LOCK_WIDTH_LOWER_BOUND * double(size.width) &&
	   croppedfirstlock.cols < OCR_REASONABLE_LOCK_WIDTH_UPPER_BOUND * double(size.width) &&
	    (croppedfirstlock.rows / (croppedfirstlock.cols + 1) < 7) &&
		(croppedfirstlock.cols / (croppedfirstlock.rows + 1) < 7) &&
		croppedfirstlock.rows > 4 && croppedfirstlock.cols > 4)
	{
		resize(croppedfirstlock,croppedfirstlock,cv::Size(croppedfirstlock.cols * 2,croppedfirstlock.rows * 2));
		threshold(croppedfirstlock,croppedfirstlock,128,255,THRESH_BINARY);
		if(CheckLockSymbol_Revised(g_pDicData, croppedfirstlock.data,croppedfirstlock.rows,croppedfirstlock.cols))
			hasHeadLock = TRUE;
	}
	if(croppedlastlock.rows > OCR_REASONABLE_LOCK_HEIGHT_LOWER_BOUND * double(size.height) &&
	   croppedlastlock.cols > OCR_REASONABLE_LOCK_WIDTH_LOWER_BOUND * double(size.width) &&
	   croppedlastlock.cols < OCR_REASONABLE_LOCK_WIDTH_UPPER_BOUND * double(size.width) &&
	   (croppedlastlock.rows / (croppedlastlock.cols + 1) < 4) &&
	   (croppedlastlock.cols / (croppedlastlock.rows + 1) < 4) &&
	   croppedlastlock.rows > 4 && croppedlastlock.cols > 4)
	{
		resize(croppedlastlock,croppedlastlock,cv::Size(croppedlastlock.cols * 2,croppedlastlock.rows * 2));
		threshold(croppedlastlock,croppedlastlock,128,255,THRESH_BINARY);
		if(CheckLockSymbol_Revised(g_pDicData, croppedlastlock.data,croppedlastlock.rows,croppedlastlock.cols))
			hasTailLock = TRUE;
	}
	////////////////////
	delete []sum_h;
	delete []sum_v;

	//cropping the origins
	left = fl1.x + fl1.width;
	right = ll1.x;
	top = fl1.y < ll1.y ? fl1.y : ll1.y;
	bottom = (fl1.y + fl1.height);
	bottom_ = (ll1.y + ll1.height);
	bottom = bottom > bottom_  ? bottom : bottom_;
	if(hasHeadLock && hasTailLock)
	{
		Mat cropped_roi(string,cv::Rect(left,top,right - left,bottom - top));
		cropped_roi.copyTo(roi_);
		return TRUE;
	}

	if(hasHeadLock)
	{
		Mat cropped_roi(string,cv::Rect(left ,0, string.cols - left, string.rows));
		cropped_roi.copyTo(roi_);		
		return TRUE;
	}

	if(hasTailLock)
	{
		Mat cropped_roi(string,cv::Rect(0, 0, right, string.rows));
		cropped_roi.copyTo(roi_);		
		return TRUE;
	}
	return FALSE;
}
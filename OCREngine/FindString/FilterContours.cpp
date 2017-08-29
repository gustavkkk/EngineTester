#include "stdafx.h"
#include "FilterContours.h"
int calcNumberofNeighbors(cv::Point2f observed,vector<vector<cv::Point>> contours,int stringSize)
{
	int count = 0;
	int radius = 0;
	for(int i = 0; i < (int)contours.size(); i++)
	{
		RotatedRect mr= minAreaRect(Mat(contours[i]));
		cv::Point2f one = mr.center;
		radius = (int)sqrt((observed.x - one.x) * (observed.x - one.x) + (observed.y - one.y) * (observed.y - one.y));
		if(radius < stringSize)
			count++;
	}
	return count;
}
int calcDiameter(RotatedRect mr)
{
	double diameter;
	diameter = mr.boundingRect().height * mr.boundingRect().height + mr.boundingRect().width * mr.boundingRect().width;
	diameter = sqrt(diameter);
	return int(diameter);
}
int calcWindow(vector<vector<cv::Point>>& contours)
{
	if(contours.size() == 0) 
		return USER_DEFINED_WINDOW;

	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	int min = USER_DEFINED_MIN, max = USER_DEFINED_MAX;
	int sum = 0;
	int average;
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		int diameter = calcDiameter(mr);
		sum += diameter;
		min = diameter > min ? min:diameter;
		max = diameter > max ? diameter:max;
		++itc;
	}
	average = sum / contours.size();
	return average;
}
void filterContoursbySize(vector<vector<cv::Point>>& contours)
{
	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	int window = calcWindow(contours);
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		cv::Point2f center = mr.center;
		int diameter = calcDiameter(mr);
		if(diameter > USER_DEFINED_STRING_SIZE)//window)
			itc= contours.erase(itc);
		else
			++itc;
	}
}
void filterContoursbyEraseRatio(Mat colorinfo,vector<vector<cv::Point>>& contours)
{
	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		cv::Point2f center = mr.center;
		if((center.x < float(colorinfo.cols * USER_DEFINED_ERASE_REGION) ||
		   center.x > colorinfo.cols * (1 -USER_DEFINED_ERASE_REGION) ||
		   center.y < float(colorinfo.rows * USER_DEFINED_ERASE_REGION) ||
		   center.y > colorinfo.rows * (1 -USER_DEFINED_ERASE_REGION)))
			itc= contours.erase(itc);
		else
			++itc;
	}
}
void filterContoursbyRelationship(vector<vector<cv::Point>>& contours,int radius,int thr)
{
	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	int window = calcWindow(contours);
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		cv::Point2f center = mr.center;
		int diameter = calcDiameter(mr);
		if(calcNumberofNeighbors(center,contours,2 * radius) < thr)
			itc= contours.erase(itc);
		else
			++itc;
	}
}
int calcThreshold(Mat colorinfo,vector<vector<cv::Point>>& contours)
{
	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	int histSize = 8;
	int *hist;
	hist = new int[histSize];
	//////////initialize hist////////////////////////////////////
	for(int i = 0; i < histSize; i++)
		hist[i] = 0;
	///////////calculate hist////////////////////////////////////////////////
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		cv::Rect boundingRect = mr.boundingRect();
		Mat cropped(colorinfo,cv::Rect(boundingRect.x > 0 ? boundingRect.x:0,
									   boundingRect.y > 0 ? boundingRect.y:0,
									   (boundingRect.x + boundingRect.width) > colorinfo.cols ? (colorinfo.cols - boundingRect.x - 1):boundingRect.width,
									   (boundingRect.y + boundingRect.height) > colorinfo.rows ? (colorinfo.rows - boundingRect.y - 1):boundingRect.height));
		
		for(int j = 0; j < cropped.rows; j++)
			for(int i = 0; i < cropped.cols; i++)
				hist[*cropped.ptr(j,i) / (256 / histSize)]++;
		++itc;
	}
	///////////calculate threshold//////////////////
	int max = USER_DEFINED_MAX;
	int min = USER_DEFINED_MIN;
	int maxindex,minindex;
	for(int i = 0; i < histSize; i++)
	{
		if(hist[i] > max)
		{
			max = hist[i];
			maxindex = i;
		}
		if(min > hist[i] && hist[i] != 0)
		{
			min = hist[i];
			minindex = i;
		}
	}
	delete []hist;
	if(maxindex < minindex) 
		return (minindex + 1) * (256 / histSize);
	else
		return (minindex + maxindex + 2) * (256 / histSize) / 2;
}
//void filterContoursbyColor(Mat colorinfo,vector<vector<cv::Point>>& contours)
//{
//	vector<vector<cv::Point> >::iterator itc= contours.begin();
//	vector<RotatedRect> rects;
//	int threshold = calcThreshold(colorinfo,contours);
//	if(threshold >= 128) return;
//	while (itc!=contours.end()) {
//		//Create bounding rect of object
//		RotatedRect mr= minAreaRect(Mat(*itc));
//		cv::Rect boundingRect = mr.boundingRect();
//		Mat cropped(colorinfo,cv::Rect(boundingRect.x > 0 ? boundingRect.x:0,
//									   boundingRect.y > 0 ? boundingRect.y:0,
//									   (boundingRect.x + boundingRect.width) > colorinfo.cols ? (colorinfo.cols - boundingRect.x - 1):boundingRect.width,
//									   (boundingRect.y + boundingRect.height) > colorinfo.rows ? (colorinfo.rows - boundingRect.y - 1):boundingRect.height));
//		int counts = 0;
//		for(int j = 0; j < cropped.rows; j++)
//			for(int i = 0; i < cropped.cols; i++)
//				if(*cropped.ptr(j,i) < threshold)
//					continue;
//				else
//					counts++;
//		if(counts == (cropped.rows * cropped.cols)) 
//			itc= contours.erase(itc);
//		else
//			++itc;
//	}
//}
#define OCR_INVOLOVE_X 20
#define OCR_INVOLOVE_Y 20
void filterContoursbyColor(Mat colorinfo,vector<vector<cv::Point>>& contours)
{
	vector<vector<cv::Point> >::iterator itc= contours.begin();
	vector<RotatedRect> rects;
	while (itc!=contours.end()) {
		//Create bounding rect of object
		RotatedRect mr= minAreaRect(Mat(*itc));
		cv::Rect boundingRect = mr.boundingRect();
		cv::Point2f center = mr.center;
		int left,right,top,bottom;
		left = (int)(center.x - OCR_INVOLOVE_X);
		right = (int)(center.x + OCR_INVOLOVE_X);
		top = (int)(center.y - OCR_INVOLOVE_Y);
		bottom = (int)(center.y + OCR_INVOLOVE_Y);
		if(left < 0) left = 0;
		if(top < 0) top = 0;
		if(right > colorinfo.cols) right = colorinfo.cols - 1;
		if(bottom > colorinfo.rows) bottom = colorinfo.rows - 1;
		Mat cropped(colorinfo,cv::Rect(left,top,right - left,bottom - top));
		int counts = 0;
		for(int j = 0; j < cropped.rows; j++)
			for(int i = 0; i < cropped.cols; i++)
				if(*cropped.ptr(j,i) == 0)//threshold)
					counts++;
		if(counts < 1) 
			itc= contours.erase(itc);
		else
			++itc;
	}
}
void filterContours(Mat colorinfo,vector<vector<cv::Point>>& contours,bool zoomed)
{
	
	filterContoursbyRelationship(contours,USER_DEFINED_WINDOW,USER_DEFINED_FILTER);
	filterContoursbySize(contours);
	if(!zoomed) return;
	//filterContoursbyEraseRatio(colorinfo,contours);
	filterContoursbyColor(colorinfo,contours);
	filterContoursbyRelationship(contours,2 * USER_DEFINED_WINDOW, 2 * USER_DEFINED_FILTER);
	filterContoursbyRelationship(contours,USER_DEFINED_WINDOW,USER_DEFINED_FILTER);
	return;
}
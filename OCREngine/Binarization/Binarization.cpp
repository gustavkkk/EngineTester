#include "stdafx.h"
#include "Binarization.h"
void windowthreshold(Mat& cropped,Mat& thr)
{
	int ii,jj;
	int start_x,start_y;
	Mat thr_temp;
	thr.copyTo(thr_temp);
	Mat thr_copy(thr);
	for(int winsize = USER_DEFINED_WINDOW_MIN; winsize < USER_DEFINED_WINDOW_MAX; winsize ++)
	{
		if(winsize > 150 && (winsize % 200 != 0))
			continue;
		for(int i = winsize; i < (cropped.cols + winsize); i += winsize)
			for(int j = winsize; j < (cropped.rows + winsize); j += winsize)
			{
				ii = i;
				jj = j;
				if(ii >= cropped.cols)
					ii = cropped.cols - 1;
				if(jj >= cropped.rows)
					jj = cropped.rows - 1;
				start_x = i - winsize;
				start_y = j - winsize;
				if(start_x == cropped.cols || start_y == cropped.rows) continue;
				Mat subcropped(cropped,cv::Rect(start_x,start_y,ii - start_x ,jj - start_y));
				Mat thr_;
				threshold(subcropped, thr_, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
				for(int aa = 0; aa < subcropped.cols; aa++)
					for(int bb = 0; bb < subcropped.rows; bb++)
						*thr_temp.ptr(start_y + bb,start_x + aa) = *thr_.ptr(bb,aa);
			}
		max(thr_temp,thr,thr);
	}
}

void windowthreshold_new(Mat& cropped,Mat& thr)
{
	int ii,jj;
	int start_x,start_y;
	Mat thr_temp;
	thr.copyTo(thr_temp);
	Mat thr_copy(thr);
	for(int winsize = USER_DEFINED_WINDOW_MIN_NEW; winsize < USER_DEFINED_WINDOW_MAX_NEW; winsize ++)
	{
		if(winsize > 20 && (winsize % 20 != 0))
			continue;
		for(int i = winsize; i < (cropped.cols + winsize); i += winsize)
			for(int j = winsize; j < (cropped.rows + winsize); j += winsize)
			{
				ii = i;
				jj = j;
				if(ii >= cropped.cols)
					ii = cropped.cols - 1;
				if(jj >= cropped.rows)
					jj = cropped.rows - 1;
				start_x = i - winsize;
				start_y = j - winsize;
				if(start_x == cropped.cols || start_y == cropped.rows) continue;
				Mat subcropped(cropped,cv::Rect(start_x,start_y,ii - start_x ,jj - start_y));
				Mat thr_;
				threshold(subcropped, thr_, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
				for(int aa = 0; aa < subcropped.cols; aa++)
					for(int bb = 0; bb < subcropped.rows; bb++)
						*thr_temp.ptr(start_y + bb,start_x + aa) = *thr_.ptr(bb,aa);
			}
		max(thr_temp,thr,thr);
	}
	////////////////////////////////////////////////
#if 0
	for(int winsize = USER_DEFINED_WINDOW_MIN_NEW; winsize < USER_DEFINED_WINDOW_MAX_NEW; winsize ++)
	{
		if(winsize > 20 && (winsize % 20 != 0))
			continue;
		for(int i = (cropped.cols - 1); i < winsize; i -= winsize)
			for(int j = winsize; j < (cropped.rows + winsize); j += winsize)
			{
				ii = i;
				jj = j;
				if(jj >= cropped.rows)
					jj = cropped.rows - 1;
				start_x = i - winsize;
				start_y = j - winsize;
				if(start_y == cropped.rows) continue;
				Mat subcropped(cropped,cv::Rect(start_x,start_y,ii - start_x ,jj - start_y));
				Mat thr_;
				threshold(subcropped, thr_, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
				for(int aa = 0; aa < subcropped.cols; aa++)
					for(int bb = 0; bb < subcropped.rows; bb++)
						*thr_temp.ptr(start_y + bb,start_x + aa) = *thr_.ptr(bb,aa);
			}
		max(thr_temp,thr,thr);
	}
#endif
	return;
}
#define OCR_THR_UPPER_SIZE 300
Mat divisionthreshold(Mat cropped)
{
	if(cropped.empty() || cropped.type() > 1)
		return Mat();
	int ii;
	int start_x;
	Mat thr_temp;
	Mat thr = Mat::zeros(cropped.rows,cropped.cols,CV_8UC1);
	thr.copyTo(thr_temp);
	Mat thr_copy(thr);
	for(int winsize = USER_DEFINED_WINDOW_MIN_NEW; winsize < OCR_THR_UPPER_SIZE; winsize ++)
	{
		if(winsize > 20 && (winsize % 100 != 0))
			continue;
		for(int i = winsize; i < (cropped.cols + winsize); i += winsize)
			{
				ii = i;
				if(ii >= cropped.cols)
					ii = cropped.cols - 1;
				start_x = i - winsize;
				if(start_x == cropped.cols) continue;
				Mat subcropped(cropped,cv::Rect(start_x,0,ii - start_x ,cropped.rows));
				Mat thr_;
				threshold(subcropped, thr_, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
				for(int aa = 0; aa < subcropped.cols; aa++)
					for(int bb = 0; bb < subcropped.rows; bb++)
						*thr_temp.ptr(bb,start_x + aa) = *thr_.ptr(bb,aa);
			}
		max(thr_temp,thr,thr);
	}
	/////////////////////////////////////////////////////////////////////////
#if 0
	for(int winsize = USER_DEFINED_WINDOW_MIN_NEW; winsize < OCR_THR_UPPER_SIZE; winsize ++)
	{
		if(winsize > 20 && (winsize % 100 != 0))
			continue;
		for(int i = (cropped.cols - 1); i < winsize; i -= winsize)
			{
				start_x = i - winsize;
				Mat subcropped(cropped,cv::Rect(start_x,0,i - start_x ,cropped.rows));
				Mat thr_;
				threshold(subcropped, thr_, 0, 255,CV_THRESH_OTSU+CV_THRESH_BINARY);
				for(int aa = 0; aa < subcropped.cols; aa++)
					for(int bb = 0; bb < subcropped.rows; bb++)
						*thr_temp.ptr(bb,start_x + aa) = *thr_.ptr(bb,aa);
			}
		max(thr_temp,thr,thr);
	}
#endif
	return thr;
}

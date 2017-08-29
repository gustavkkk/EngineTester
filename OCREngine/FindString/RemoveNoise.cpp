#include "stdafx.h"
#include "RemoveNoise.h"
void removeBoundaryNoise(Mat& thr)
{
	if(!thr.data) return;
	for(int j = 0; j < thr.rows; j++)
		for(int i = 0; i < thr.cols; i++)
			if(i == 0 || j == 0 || j == (thr.rows - 1) || i == (thr.cols - 1))
				*thr.ptr(j,i) = 255;
	return;
}


double calcNonZero(Mat thr)
{
	int count = 1;
	for(int j = 1; j < thr.rows - 1; j++)
	for(int i = 0; i < thr.cols; i++)
		if(*thr.ptr(j,i) == 0)
			count++;
	return double(count);
}
#define OCR_IS_NO_FEATURES 1000
int calcCount(Mat in,int fg_value)
{
	int count = 1;
	for(int j = 1; j < in.rows - 1; j++)
	for(int i = 0; i < in.cols; i++)
		if(*in.ptr(j,i) == fg_value)
			count++;
	return count;
}
void removeGaussianNoise(Mat& thr,int bg_value,int fg_value)
{
	if(!thr.data) return;
	Mat temp;
	thr.copyTo(temp);
	for(int j = 1; j < thr.rows - 1; j++)
		for(int i = 1; i < thr.cols - 1; i++)
			if(*thr.ptr(j,i) == fg_value)
			{
				if(*thr.ptr(j - 1,i) == bg_value ||
				   *thr.ptr(j + 1,i) == bg_value ||
				   *thr.ptr(j,i - 1) == bg_value ||
				   *thr.ptr(j,i + 1) == bg_value)
					*temp.ptr(j,i) = bg_value;
			}
		if(calcCount(thr,fg_value) > OCR_IS_NO_FEATURES)
		temp.copyTo(thr);
	return;
}
void removeGaussianNoiseByWindow(Mat& thr,int bg_value,int fg_value,int window_size)
{
	if(!thr.data || thr.cols < window_size || thr.rows < window_size || window_size % 2 == 0) 
		return;
	int radius = window_size / 2;
	Mat temp;
	thr.copyTo(temp);
	for(int j = radius; j < thr.rows - radius; j++)
		for(int i = radius; i < thr.cols - radius; i++)
		{
			if(*thr.ptr(j,i) != fg_value)
				continue;
			bool flag = true;
			for(int k = -radius; k <= radius; k++)
			{
				if(*thr.ptr(j - radius,i + k) != bg_value ||
				   *thr.ptr(j + k,i + radius) != bg_value ||
				   *thr.ptr(j + radius, i + k) != bg_value ||
				   *thr.ptr(j + k,i - radius) != bg_value)
				{
					flag = false;
					break;
				}
			}
			if(flag)
				*temp.ptr(j,i) = bg_value;
		}
	temp.copyTo(thr);
	return;
}
void removeGaussianNoise_revised(Mat& thr,int bg_value,int fg_value)
{
#if 1
	removeGaussianNoiseByWindow(thr,bg_value,fg_value,3);
	removeGaussianNoiseByWindow(thr,bg_value,fg_value,5);
	removeGaussianNoiseByWindow(thr,bg_value,fg_value,7);
	removeGaussianNoiseByWindow(thr,bg_value,fg_value,9);
#else
	if(!thr.data || thr.cols < 5 || thr.rows < 5) 
		return;
	Mat temp;
	thr.copyTo(temp);
	for(int j = 2; j < thr.rows - 2; j++)
		for(int i = 2; i < thr.cols - 2; i++)
			if(*thr.ptr(j,i) == fg_value)
			{
				if(*thr.ptr(j - 1,i - 1) == bg_value &&
				   *thr.ptr(j - 1,i) == bg_value &&
				   *thr.ptr(j - 1,i + 1) == bg_value &&
				   *thr.ptr(j,i + 1) == bg_value &&
				   *thr.ptr(j + 1,i + 1) == bg_value &&
				   *thr.ptr(j + 1,i) == bg_value &&
				   *thr.ptr(j + 1,i - 1) == bg_value &&
				   *thr.ptr(j - 1,i) == bg_value)
				{
				   *temp.ptr(j,i) = bg_value;
				   continue;
				}
				if(*thr.ptr(j - 2,i - 2) == bg_value &&
				   *thr.ptr(j - 2,i - 1) == bg_value &&
				   *thr.ptr(j - 2,i) == bg_value &&
				   *thr.ptr(j - 2,i + 1) == bg_value &&
				   *thr.ptr(j - 2,i + 2) == bg_value &&
				   *thr.ptr(j - 1,i + 2) == bg_value &&
				   *thr.ptr(j,i + 2) == bg_value &&
				   *thr.ptr(j + 1,i + 2) == bg_value &&
				   *thr.ptr(j + 2,i + 1) == bg_value &&
				   *thr.ptr(j + 2,i) == bg_value &&
				   *thr.ptr(j + 2,i - 1) == bg_value &&
				   *thr.ptr(j + 2,i - 2) == bg_value &&
				   *thr.ptr(j + 1,i - 2) == bg_value &&
				   *thr.ptr(j,i - 2) == bg_value &&
				   *thr.ptr(j - 1,i - 2) == bg_value)
					*temp.ptr(j,i) = bg_value;
			}
	temp.copyTo(thr);
	return;
#endif
}
void removeRemainedNoiseByVerticalWindow(Mat& thr,Mat thr_temp)
{
	int step = 5;//10;(revised)
	//for(int step = 10; step < 20; step += 2)
	//{
		int window = thr.cols / step;
		for(int i = 0; i < step; i++)
		{
			Mat cropped(thr,cv::Rect(i * window,0,window,thr.rows));
			Mat cropped_temp(thr_temp,cv::Rect(i * window,0,window,thr.rows));
			double count = calcNonZero(cropped);
			double count_temp = calcNonZero(cropped_temp);
			if(count / count_temp < USER_DEFINED_NOISE_THR ||
				((i == 0 || i == (step - 1)) && count / count_temp < USER_DEFINED_NOISE_THR * 2))
				for(int jj = 0; jj < thr.rows; jj++)
					for(int ii = i * window; ii < (i+1) * window; ii++)
						*thr.ptr(jj,ii) = 255;
		}
	//}
}
void removeRemainedNoiseByHorizontalWindow(Mat& thr,Mat thr_temp)
{
	int step = 10;
	int window = thr.rows / step;
	for(int i = 0; i < step; i++)
	{
		Mat cropped(thr,cv::Rect(0,i * window,thr.cols,window));
		Mat cropped_temp(thr_temp,cv::Rect(0,i * window,thr.cols,window));
		double count = calcNonZero(cropped);
		double count_temp = calcNonZero(cropped_temp);
		if(count / count_temp < USER_DEFINED_NOISE_THR || 
			((i == 0 || i == (step - 1)) && count / count_temp < USER_DEFINED_NOISE_THR * 2))
			for(int jj = i * window; jj < (i+1) * window; jj++)
				for(int ii = 0; ii < thr.cols; ii++)
					*thr.ptr(jj,ii) = 255;
	}
}
void removeGaussianNoisebyWindow(Mat& thr,Mat& thr_temp,int step_h,int step_w)
{
	if(step_h < 0 || step_w < 0 || thr.empty() || thr_temp.empty()) return;
	int winsz_h = (thr.cols / step_h);
	int winsz_w = (thr.rows / step_w);
	for(int i = 0; i < step_h; i++)
		for(int j = 0; j < step_w; j++)
		{
			Mat cropped(thr,cv::Rect(i * winsz_h,j * winsz_w,winsz_h ,winsz_w));
			Mat cropped_temp(thr_temp,cv::Rect(i * winsz_h,j * winsz_w,winsz_h ,winsz_w));
			double count = calcNonZero(cropped);
			double count_temp = calcNonZero(cropped_temp);
			if(count / count_temp < USER_DEFINED_NOISE_THR)
				for(int jj = j * winsz_w; jj < (j+1) * winsz_w; jj++)
					for(int ii = i * winsz_h; ii < (i+1) * winsz_h; ii++)
						*thr_temp.ptr(jj,ii) = 255;
		}
}
void removeBoundaryNoiseSTEP2(Mat& thr,Mat& thr_temp)
{
	return;
}
void removeNoise(Mat& thr,bool zoomed)
{
	if(!thr.data) return;
	removeBoundaryNoise(thr);
	if(!zoomed) return;
	Mat thr_temp;
	thr.copyTo(thr_temp);
	removeGaussianNoise(thr,OCR_PIXEL_MAX,OCR_PIXEL_MIN);
	removeGaussianNoise(thr,OCR_PIXEL_MAX,OCR_PIXEL_MIN);
	removeGaussianNoise(thr,OCR_PIXEL_MAX,OCR_PIXEL_MIN);
	//removeGaussianNoise(thr);
	removeRemainedNoiseByVerticalWindow(thr,thr_temp);
	//
	//removeBoundaryNoise_(thr,thr_temp);
	//removeRemainedNoiseByHorizontalWindow(thr,thr_temp);
	//removeGaussianNoisebyWindow(thr,thr_temp,10,2);
	//removeGaussianNoise(thr);
	//removeGaussianNoise(thr);
	//removeGaussianNoise(thr);
}
void removeNoise_revised(Mat& thr,bool zoomed)
{
	if(!thr.data) return;
	removeBoundaryNoise(thr);
	if(!zoomed) return;
	Mat thr_temp;
	thr.copyTo(thr_temp);
	removeGaussianNoise(thr,OCR_PIXEL_MAX,OCR_PIXEL_MIN);
	removeRemainedNoiseByVerticalWindow(thr,thr_temp);
}
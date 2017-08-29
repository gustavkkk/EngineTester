#include "stdafx.h"
#include "PosterEdge.h"
#include "ExtractBlack.h"
#include "opencv2/highgui/highgui.hpp"

Mat ocr_AdaptiveCvtColor_new(Mat in)//,Mat& gray)
{
	//determine background color spectrum
#if 1
	double R_ = 0,G_ = 0,B_ = 0;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B_ += double(*in.ptr(j,i)) / 255.0;
			G_ += double(*(in.ptr(j,i) + 1)) / 255.0;
			R_ += double(*(in.ptr(j,i) + 2)) / 255.0;
		}
	int selected = 0;
	selected = (R_ >= B_ && R_ >= G_) ? 0:(G_ >= R_&& G_ >= B_) ? 1:2;
#endif 
	Mat gray = cvCreateMat(in.rows,in.cols,CV_8UC1);
	double  R,G,B;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B = *in.ptr(j,i);
			G = *(in.ptr(j,i) + 1);
			R = *(in.ptr(j,i) + 2);
#if 1
			switch(selected)
			{
			case 0:
				if(G_ > B_)
					*gray.ptr(j,i) = (uchar)((R * 50.0 + G * 10.0 + B * 1.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 50.0 + G * 1.0 + B * 10.0) / (61.0 * 3.0));
				break;
			case 1:
				if(R_ > B_)
					*gray.ptr(j,i) = (uchar)((R * 10.0 + G * 50.0 + B * 1.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 1.0 + G * 50.0 + B * 10.0) / (61.0 * 3.0));
				break;
			default:
				if(R_ > G_)
					*gray.ptr(j,i) = (uchar)((R * 10.0 + G * 1.0 + B * 50.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 1.0 + G * 10.0 + B * 50.0) / (61.0 * 3.0));
				break;
			}

#else 
			*gray.ptr(j,i) = (R * 20.0 + G * 40.0 + B * 1.0) / 61.0;
#endif
			*gray.ptr(j,i) = *gray.ptr(j,i) > 85 ? 255 : *gray.ptr(j,i)*3;
		}
	return gray;
}
Mat posterEdgefilter_new(Mat in)
{
	Mat tmp,gray;
	in.copyTo(gray);
	//Convert2Gray(gray);
	gray = ocr_AdaptiveCvtColor_new(in);
	gray.copyTo(tmp);
	return tmp;
}
Mat posterEdgefilter(Mat in)
{
	Mat tmp,gray;
#ifndef HSV_GRAY
	in.copyTo(gray);
	//Convert2Gray(gray);
	gray = ocr_AdaptiveCvtColor(in);
	gray.copyTo(tmp);
#else
	cvtColor(in,tmp,COLOR_BGR2GRAY);
	tmp.copyTo(gray);
#endif
#if 0
	int winsize = 10;
	int width = in.cols, height = in.rows;
	Mat log,B,D,B_;
	int start_x,start_y;
	int ii,jj;
	for(int i = winsize; i < (width + winsize); i += winsize)
		for(int j = winsize; j < (height + winsize); j += winsize)
		{
			ii = i;
			jj = j;
			if(ii >= width)
				ii = width - 1;
			if(jj >= height)
				jj = height - 1;
			start_x = i - winsize;
			start_y = j - winsize;
			if(start_x == width || start_y == height) continue;
			Mat subcropped(tmp,cv::Rect(start_x,start_y,ii - start_x ,jj - start_y));
			subcropped += 1.0;
			ocr_log(subcropped,log);
			adaptiveBilateralFilter(log,B,cv::Size(3,3),3.0);
			//crossBilateralFilter(log,log,B,3,5);
			//D = log - B;
			//int contrast = 100,r;
			//B_ = B ;
			//ocr_exp(B,subcropped);
			//equalizeHist(B,subcropped);
			//int min = ocr_min(subcropped);
			for(int aa = 0; aa < subcropped.cols; aa++)
			for(int bb = 0; bb < subcropped.rows; bb++)
#if 1
				*tmp.ptr(start_y + bb,start_x + aa) = *subcropped.ptr(bb,aa);
#else
				if(*subcropped.ptr(bb,aa) == min)
					*tmp.ptr(start_y + bb,start_x + aa) = 0;//*subcropped.ptr(bb,aa);
				else
					*tmp.ptr(start_y + bb,start_x + aa) = 255;
#endif
			subcropped.release();
		}
		min(tmp,gray,tmp);
#endif
		//equalizeHist(tmp,tmp);
		//threshold(tmp, tmp, 1, 255, THRESH_BINARY_INV);
	return tmp;
}

void ocr_log(Mat in,Mat& out)
{
	if(in.empty())
		return;
	in.copyTo(out);
	bool flag = out.type() == CV_8UC3 ? true : false;
	for(int j = 0; j < out.rows; j++)
		for(int i = 0; i < out.cols; i++)
		{
			double value;
			if(flag){
				value = double(*out.ptr(j,i));
				*out.ptr(j,i) = (uchar)(log(value)/log(2.0));//int(log(value));
				value = double(*(out.ptr(j,i) + 1));
				*(out.ptr(j,i) + 1) = (uchar)(log(value)/log(2.0));//int(log(value));
				value = double(*(out.ptr(j,i) + 2));
				*(out.ptr(j,i) + 2) = (uchar)(log(value)/log(2.0));//int(log(value));
			}
			else{
				value = double(*out.ptr(j,i));
				*out.ptr(j,i) = (uchar)(log(value)/log(2.0));//int(log(value));
			}
		}
	return;
}
void ocr_exp(Mat in,Mat& out)
{
	if(in.empty())
		return;
	in.copyTo(out);
	bool flag = out.type() == CV_8UC3 ? true : false;
	for(int j = 0; j < out.rows; j++)
		for(int i = 0; i < out.cols; i++)
		{
			if(flag){
				double value = *out.ptr(j,i);
				*out.ptr(j,i) = (uchar)(pow(2.0,value));//exp(value);//int(exp(value));
				value = *(out.ptr(j,i) + 1);
				*(out.ptr(j,i) + 1) = (uchar)(pow(2.0,value));//exp(value);//int(exp(value));
				value = *(out.ptr(j,i) + 2);
				*(out.ptr(j,i) + 2) = (uchar)(pow(2.0,value));//exp(value);//int(exp(value));
			}
			else{
				double value = double(*out.ptr(j,i));
				*out.ptr(j,i) = (uchar)(pow(2.0,value));//exp(value);//int(exp(value));
			}
		}
	return;
}
Mat ocr_multiply(Mat operand1,Mat operand2)
{
	if(operand1.empty() || operand2.empty() || operand1.type() != CV_8UC1 || operand2.type() != CV_8UC1)
		return Mat();
	Mat result;
	int value;
	operand1.copyTo(result);
		for(int j = 0; j < result.rows; j++)
		for(int i = 0; i < result.cols; i++)
		{
			value = *result.ptr(j,i) * *operand2.ptr(j,i);
			if(value >255) value =255;
			if(value < 0) value = 0;
			*result.ptr(j,i) = value;
		}
	return result;
}
Mat ocr_minus(Mat operand1,Mat operand2)
{
	if(operand1.empty() || operand2.empty() || operand1.type() != CV_8UC1 || operand2.type() != CV_8UC1)
		return Mat();
	Mat result;
	int value;
	operand1.copyTo(result);
		for(int j = 0; j < result.rows; j++)
		for(int i = 0; i < result.cols; i++)
		{
			value = abs(*result.ptr(j,i) - *operand2.ptr(j,i));
			*result.ptr(j,i) = value;
		}
	return result;
}
Mat ocr_AdaptiveCvtColor(Mat in)//,Mat& gray)
{
	//determine background color spectrum
#if 1
	double R_ = 0,G_ = 0,B_ = 0;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B_ += double(*in.ptr(j,i)) / 255.0;
			G_ += double(*(in.ptr(j,i) + 1)) / 255.0;
			R_ += double(*(in.ptr(j,i) + 2)) / 255.0;
		}
	int selected = 0;
	selected = (R_ >= B_ && R_ >= G_) ? 0:(G_ >= R_&& G_ >= B_) ? 1:2;
#endif 
	Mat gray = cvCreateMat(in.rows,in.cols,CV_8UC1);
	double  R,G,B;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B = *in.ptr(j,i);
			G = *(in.ptr(j,i) + 1);
			R = *(in.ptr(j,i) + 2);
#if 1
			switch(selected)
			{
			case 0:
				if(G_ > B_)
					*gray.ptr(j,i) = (uchar)((R * 40.0 + G * 20.0 + B * 1.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 40.0 + G * 1.0 + B * 20.0) / (61.0 * 3.0));
				break;
			case 1:
				if(R_ > B_)
					*gray.ptr(j,i) = (uchar)((R * 20.0 + G * 40.0 + B * 1.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 1.0 + G * 40.0 + B * 20.0) / (61.0 * 3.0));
				break;
			default:
				if(R_ > G_)
					*gray.ptr(j,i) = (uchar)((R * 20.0 + G * 1.0 + B * 40.0) / (61.0 * 3.0));
				else
					*gray.ptr(j,i) = (uchar)((R * 1.0 + G * 20.0 + B * 40.0) / (61.0 * 3.0));
				break;
			}

#else 
			*gray.ptr(j,i) = (R * 20.0 + G * 40.0 + B * 1.0) / 61.0;
#endif
			*gray.ptr(j,i) = *gray.ptr(j,i) > 85 ? 255 : *gray.ptr(j,i)*3;
		}
	return gray;
}
void ocr_inv_AdaptiveCvtColor(Mat& gray)
{
	if(gray.empty())
		return;
	for(int j = 0; j < gray.rows; j++)
		for(int i = 0; i < gray.cols; i++)
		{
			*gray.ptr(j,i) *= 85; 
		}
		return;
}
void ocr_copyfrom32FC3to8UC3(Mat from,Mat& to)
{
	if(from.empty() || from.type() != CV_32FC3)
		return;
	to.release();
	to = cvCreateMat(from.rows,from.cols,CV_8UC3);
	for(int j = 0; j < from.rows; j++)
		for(int i = 0; i < from.cols; i++)
		{
			*to.ptr(j,i) = int(*to.ptr(j,i));
			*(to.ptr(j,i) + 1) = int(*(to.ptr(j,i) + 1));
			*(to.ptr(j,i) + 2) = int(*(to.ptr(j,i) + 2));
		}
	return;
}
void ocr_copyfrom8UC3To32FC3(Mat from,Mat& to)
{
	if(from.empty())
		return;
	to = cvCreateMat(from.rows,from.cols,CV_32FC3);
	for(int j = 0; j < from.rows; j++)
		for(int i = 0; i < from.cols; i++)
		{
			*to.ptr(j,i) = 255;//float(*to.ptr(j,i));
			*(to.ptr(j,i) + 1) = 255;//float(*(to.ptr(j,i) + 1));
			*(to.ptr(j,i) + 2) = 255;//float(*(to.ptr(j,i) + 2));
		}
	return;
}
int ocr_max(Mat input)
{
	if(input.empty())
		return 0;
	int max = 0;
	for(int j = 0; j < input.rows; j++)
	for(int i = 0; i < input.cols; i++)
		if(*input.ptr(j,i) > max)
			max = *input.ptr(j,i);
	return max;
}
int ocr_min(Mat input)
{
	if(input.empty())
		return 0;
	int min = 8;
	for(int j = 0; j < input.rows; j++)
	for(int i = 0; i < input.cols; i++)
		if(*input.ptr(j,i) < min)
			min = *input.ptr(j,i);
	return min;
}
void ocr_filteringMargin(Mat& binary)
{
	if(binary.empty() || binary.type() != CV_8UC1)
		return;
	for(int j = 0; j < binary.rows; j++)
		for(int i = 0; i < binary.cols; i++)
		{
			if(j < OCR_MARGIN_TB * binary.rows ||
			   j > (1 - OCR_MARGIN_TB) * binary.rows ||
			   i < OCR_MARGIN_LR * binary.cols ||
			   i > (1 - OCR_MARGIN_LR) * binary.cols)
				*binary.ptr(j,i) = 0;//float(*to.ptr(j,i));
		}
	return;
}
/*
Mat ocr_AdaptiveCvtColor(Mat in)//,Mat& gray)
{
	//determine background color spectrum
#if 1
	double R_ = 0,G_ = 0,B_ = 0;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B_ += double(*in.ptr(j,i)) / 255.0;
			G_ += double(*(in.ptr(j,i) + 1)) / 255.0;
			R_ += double(*(in.ptr(j,i) + 2)) / 255.0;
		}
	int selected = 0;
	selected = (R_ >= B_ && R_ >= G_) ? 0:(G_ >= R_&& G_ >= B_) ? 1:2;
#endif 
	Mat gray = cvCreateMat(in.rows,in.cols,CV_8UC1);
	double  R,G,B;
	for(int j = 0; j < in.rows; j++)
		for(int i = 0; i < in.cols; i++)
		{
			B = *in.ptr(j,i);
			G = *(in.ptr(j,i) + 1);
			R = *(in.ptr(j,i) + 2);
#if 1
			switch(selected)
			{
			case 0:
				if(G_ > B_)
					*gray.ptr(j,i) = (R * 40.0 + G * 20.0 + B * 1.0) / (61.0 * 3.0);
				else
					*gray.ptr(j,i) = (R * 40.0 + G * 1.0 + B * 20.0) / (61.0 * 3.0);
				break;
			case 1:
				if(R_ > B_)
					*gray.ptr(j,i) = (R * 20.0 + G * 40.0 + B * 1.0) / (61.0 * 3.0);
				else
					*gray.ptr(j,i) = (R * 1.0 + G * 40.0 + B * 20.0) / (61.0 * 3.0);
				break;
			default:
				if(R_ > G_)
					*gray.ptr(j,i) = (R * 20.0 + G * 1.0 + B * 40.0) / (61.0 * 3.0);
				else
					*gray.ptr(j,i) = (R * 1.0 + G * 20.0 + B * 40.0) / (61.0 * 3.0);
				break;
			}

#else 
			*gray.ptr(j,i) = (R * 20.0 + G * 40.0 + B * 1.0) / (61.0 * 3.0);
#endif
			*gray.ptr(j,i) *= 3.0;
		}
	return gray;
}
*/

//by JON

Mat posterEdgefilterJON(const Mat &image)
{
	Mat imageHSV;
	cvtColor(image, imageHSV, CV_RGB2HSV);
	
	Mat imageH(image.rows,image.cols,CV_8UC1);
	Mat imageS(image.rows,image.cols,CV_8UC1);
	Mat imageV(image.rows,image.cols,CV_8UC1);

	for(int i=0;i<image.rows;++i)
	{
		for(int j=0;j<image.cols;++j)
		{
			*imageH.ptr(i, j) = *imageHSV.ptr(i, j);
			*imageS.ptr(i, j) = *(imageHSV.ptr(i, j)+1);
			*imageV.ptr(i, j) = *(imageHSV.ptr(i, j)+2);
		}
	}
	//cv::imshow("V", imageV);
	//Mat boundaryV = boundaryDetect(imageV, 15, 0.5);
	//cv::imshow("boundaryV", boundaryV);
	int windowSize = 11;//9;
	Mat result = Mat::zeros(imageV.rows, imageV.cols, CV_8UC1);
	for(int x = 0; x < imageV.rows; ++x)
	{
		for(int y = 0; y < imageV.cols; ++y)
		{
			//if (*boundaryV.ptr(x, y) > 0)
			{
				int temp=0;
				for(int f = x - int(windowSize / 2); f < x + int(windowSize / 2); ++f)
				{
					for(int s = y - int(windowSize / 2); s < y + int(windowSize / 2); ++s)
					{
						if ((f > 0 && s > 0 && f < imageV.rows && s < imageV.cols) && (*imageV.ptr(x, y) < *imageV.ptr(f, s)))
							temp += abs(*imageV.ptr(x, y) - *imageV.ptr(f, s));
					}
				}
				*result.ptr(x, y) = temp / (windowSize * windowSize);
			}
		}
	}
	//cv::imshow("result", result);
	double resultMax = mat8Max(result);
	double resultMin = mat8Min(result);
	Mat posterImage(image.rows, image.cols, CV_8UC3);
	double coefficient = 1.0;
	for(int i = 0; i < image.rows; ++i)
	{
		for(int j = 0; j < image.cols; ++j)
		{
			double rColor = *image.ptr(i, j);
			double gColor = *(image.ptr(i, j) + 1);
			double bColor = *(image.ptr(i, j) + 2);
			//if((*result.ptr(i, j) - resultMin) * 6 > resultMax)
			//{
				rColor -= coefficient*rColor*((*result.ptr(i, j) - resultMin) / resultMax);
				gColor -= coefficient*gColor*((*result.ptr(i, j) - resultMin) / resultMax);
				bColor -= coefficient*bColor*((*result.ptr(i, j) - resultMin) / resultMax);
			//}
            if (rColor<0) rColor=0;
            if (gColor<0) gColor=0;
            if (bColor<0) bColor=0;

            *posterImage.ptr(i, j)=uchar(rColor);
			*(posterImage.ptr(i, j)+1)=uchar(gColor);
            *(posterImage.ptr(i, j)+2)=uchar(bColor);

			
		}
	}
	//imshow("posterImage", posterImage);
	return posterImage;
}

Mat boundaryDetect(Mat in, int winSize=WINDOWSIZE, double rate=TH_RATE)
{
	Mat result = Mat::zeros(in.rows, in.cols, CV_8UC1);
	Mat region = Mat(winSize, winSize, CV_8UC1);
	for(int i = winSize; i < in.rows - winSize; ++i)
	{
		for(int j = winSize; j < in.cols - winSize; ++j)
		{
			region = in(Range(i - int(winSize/2), i + int(winSize/2)), 
				Range(j - int(winSize/2), j + int(winSize/2)));
			double regionMean = mat8Mean(region);
			double regionMin = mat8Min(region);
			double regionMax = mat8Max(region);
			if(regionMax - regionMin > 8)
			{
				double threshold = (regionMean + regionMin) / 2;
				for(int f = i - int(region.rows / 2); f <= i + int(region.rows / 2); ++f)
				{
					for(int s = j - int(region.cols / 2); s <= j + int(region.cols / 2); ++s)
					{
						if (*in.ptr(i, j) <= threshold)
						{
							*result.ptr(i, j) += 1;
						}
					}
				}
			}
		}
	}
    //int TH_RESULT=(int)(winSize * winSize * TH_RATE);
	//cv::threshold(result, result, TH_RESULT, 255, CV_THRESH_TOZERO);
	return result;
}

double mat8Max(Mat in)
{
	double max=0;
	for(int i = 0; i < in.rows; ++i)
	{
		for(int j = 0;j < in.cols; ++j)
		{
			if(max < (*in.ptr(i, j)))
				max=*in.ptr(i, j);
		}
	}
	return max;
}

double mat8Min(Mat in)
{
	double min=500;
	for(int i = 0; i < in.rows; ++i)
	{
		for(int j = 0;j < in.cols; ++j)
		{
			if(min > (*in.ptr(i, j)))
				min=*in.ptr(i, j);
		}
	}
	return min;
}

double mat8Mean(Mat in)
{
	double mean=0;
	for(int i = 0; i < in.rows; ++i)
	{
		for(int j = 0;j < in.cols; ++j)
		{
			mean += *in.ptr(i, j);
		}
	}
	mean=mean / (in.rows * in.cols);
	return mean;
}

/////////////////////////////////////////////////-added by Kojy 20150730///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void  filterbyKJY(Mat in,Mat out)
{
	if(in.type() != CV_8UC3 || in.cols < 10 || in.rows < 10)
		return;
	return;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
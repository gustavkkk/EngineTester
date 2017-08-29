#pragma once

#include "opencv/cv.h"

using namespace cv;

#define OCR_MARGIN_LR 0.15
#define OCR_MARGIN_TB 0.2

Mat posterEdgefilter(Mat in);
Mat posterEdgefilter_new(Mat in);
void ocr_log(Mat in,Mat& out);
void ocr_exp(Mat in,Mat& out);
Mat ocr_multiply(Mat operand1,Mat operand2);
Mat ocr_minus(Mat operand1,Mat operand2);
Mat ocr_AdaptiveCvtColor(Mat in);
void ocr_copyfrom8UC3To32FC3(Mat from,Mat& to);
int ocr_max(Mat input);
int ocr_min(Mat input);
#define OCR_MARGIN_LR 0.15
#define OCR_MARGIN_TB 0.2
void ocr_filteringMargin(Mat& binary);

//by jon
#define TH_RATE 0.5
#define WINDOWSIZE 13
Mat boundaryDetect(Mat in, int winSize, double rate);
Mat posterEdgefilterJON(const Mat &image);

double mat8Max(Mat in);
double mat8Min(Mat in);
double mat8Mean(Mat in);
#pragma once

#include "opencv/cv.h"

using namespace cv;

#define USER_DEFINED_NOISE_THR 0.1

void removeNoise(Mat& thr,bool zoomed);
void removeNoise_revised(Mat& thr,bool zoomed);
void removeGaussianNoiseByWindow(Mat& thr,int bg_value,int fg_value,int window_size);
void removeBoundaryNoise(Mat& thr);
void removeGaussianNoise_revised(Mat& thr,int bg_value,int fg_value);
void removeGaussianNoise(Mat& thr,int bg_value,int fg_value);
void removeGaussianNoisebyWindow(Mat& thr,Mat& thr_temp,int step_h,int step_w);
void removeRemainedNoiseByHorizontalWindow(Mat& thr,Mat thr_temp);
void removeRemainedNoiseByVerticalWindow(Mat& thr,Mat thr_temp);
void removeBoundaryNoiseSTEP2(Mat& thr,Mat& thr_temp);
double calcZero(Mat thr);
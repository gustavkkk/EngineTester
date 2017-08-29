#pragma once

#include "opencv/cv.h"

using namespace cv;

#define USER_DEFINED_WINDOW_MIN 50//50//4
#define USER_DEFINED_WINDOW_MAX 1200//800//101//200
#define USER_DEFINED_WINDOW_MIN_NEW 8//10//50//4
#define USER_DEFINED_WINDOW_MAX_NEW 200//800//101//200

void windowthreshold(Mat& cropped,Mat& thr);
void windowthreshold_new(Mat& cropped,Mat& thr);
Mat divisionthreshold(Mat cropped);

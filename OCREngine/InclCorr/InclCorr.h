#pragma once

#include "OCREngine.h"


void ProcLinearInterpolation(Mat &image);
void ProcTransformAlpha(const Mat &image, double &alphaRotate, double &alphaSkew);
int ProcTransformCompensation(Mat &image, double duInclineAlpha, double duSkewAlpha);
void ProcCurveCorrection(Mat &image);

#pragma once

#include "OCREngine.h"


void FixSegmentSize(const Mat &image, Rect &rcSegment);
int Segmentation(const Mat &image, PSEGMENT_RESULT pSegRes);

#pragma once

#include "opencv/cv.h"
#include "global.h"
using namespace cv;

int getFirstIndex(int* array_,int arraySize,int value);
int getLastIndex(int* array_,int arraySize,int value);
void getFirstLock(int* array_,int arraySize,int& start,int& end);
void getLastLock(int* array_,int arraySize,int& start,int& end);
void calcSize(int* array_,int arraySize,int& firstindex,int& lastindex);
int calcGroupCount(int* array_,int arraySize,group_& biggest);
int calcGroupCount_(int* array_,int arraySize,group_& biggest);
int calcGroupCount_t(int* array_,int arraySize);
int calcMax(int* array_,int arraySize);
int calcMin(int* array_,int arraySize);
int calcMean(int* array_,int arraySize);
void lower(int* array_,int arraySize);
void blur_(int* array_,int arraySize);
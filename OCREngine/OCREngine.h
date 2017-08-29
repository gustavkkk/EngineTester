// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the OCRENGINE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// OCRENGINE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#include "opencv/cv.h"

using namespace cv;

typedef struct _tINCCORR_RESULT
{
    Mat				imgIncCorr;
} INCCORR_RESULT, *PINCCORR_RESULT;

typedef struct _tSEGMENT_RESULT
{
    Mat				imgNR;
    vector<cv::Rect>	segments;
    Mat				imgSegments;
} SEGMENT_RESULT, *PSEGMENT_RESULT;

typedef struct _tCONFIDENCE
{
	vector<WORD>	codes;
	vector<float>	degrees;
} CONFIDENCE, *PCONFIDENCE;

typedef struct _tOCR_RESULT
{
    Mat					imgIncCorr;
    SEGMENT_RESULT		seg_res;
    vector<char>		string;
	vector<CONFIDENCE>	confidence;
} OCR_RESULT, *POCR_RESULT;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _tCarNumber
{
	int		x;
	int		y;
	int		width;
	int		height;
	wchar_t number[16];
	int		nRej;
	int		nColor;
	int		charNum;
#ifdef _DEBUG
	int		char_x[30];
	int		char_y[30];
	int		char_w[30];
	int		char_h[30];
#endif
} CarNumber;
typedef struct _tFace
{
	int		x;
	int		y;
	int		width;
	int		height;
	double	sim;
} Face;

typedef struct _tCar
{
	int		x;
	int		y;
	int		width;
	int		height;
	double	sim;
} Car;
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#ifdef MSVC_OCR

#ifdef OCRENGINE_EXPORTS
#define OCRENGINE_API __declspec(dllexport)
#else
#define OCRENGINE_API __declspec(dllimport)
#endif

// This class is exported from the OCREngine.dll
class OCRENGINE_API COCREngine {
public:
	//CREngine(void);
	// TODO: add your methods here.
};

extern OCRENGINE_API int nOCREngine;
OCRENGINE_API vector<Mat> GetLockList(const Mat &image);
OCRENGINE_API int OCR(const Mat &image, POCR_RESULT result);
OCRENGINE_API BOOL LearningLockSymbol(BYTE *pCharImgData,int nHeight,int nWidth);
OCRENGINE_API BOOL LearningChar(BYTE *pCharImgData,int nHeight,int nWidth, WORD wCode, int nKind);
////////////////////////////////////////////////////////////////////////////////////////
__declspec(dllexport) int __stdcall anpr(BYTE *frame, int cols, int rows, int step, int bpp, int flip, double scale_w, double scale_h, CarNumber *car_numbers);
OCRENGINE_API BOOL IsInvolved(Mat &img);

__declspec(dllexport) int __stdcall getFeature(unsigned char *frame, int cols, int rows, int step, int bpp, int flip, unsigned char *feature);
__declspec(dllexport) int __stdcall findFace1(unsigned char *frame, int cols, int rows, int step, int bpp, int flip, unsigned char *feature, Face *faces);
__declspec(dllexport) int findCar(Mat &frame, Mat &car_image, Car *cars);
OCRENGINE_API int containFaces(Mat &img);

__declspec(dllexport) int getCarFeatures(Mat &car_image, vector<KeyPoint> &f1, Mat &f2, float *f3, Mat &f4);
__declspec(dllexport) int findCarWithFeatures(Mat &frame, vector<KeyPoint> &f1, Mat &f2, float *f3, Mat &f4, Car *cars);
__declspec(dllexport) int findCar(Mat &frame, Mat &car_image, Car *cars);
OCRENGINE_API int containCars(Mat &img);

OCRENGINE_API void GetFeatures(Mat &img);
////////////////////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include <stdio.h>
//#include <sys/time.h>
//void msleep (unsigned int ms) {
//    int microsecs;
//    struct timeval tv;
//    microsecs = ms * 1000;
//    tv.tv_sec  = microsecs / 1000000;
//    tv.tv_usec = microsecs % 1000000;
//    select (0, NULL, NULL, NULL, &tv);  
//}
//static double now_ms(void)
//{
//    struct timespec res;
//    clock_gettime(CLOCK_REALTIME, &res);
//    return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
//}
inline ULONGLONG GetMilliTickCount()
{
#if defined WIN32 || defined _WIN32
	LARGE_INTEGER	liPerfCount, liFreq;

	QueryPerformanceCounter(&liPerfCount);
	QueryPerformanceFrequency(&liFreq);

	return (liPerfCount.QuadPart * 1000LL / liFreq.QuadPart);
#else
	return 0;
#endif
}

inline ULONGLONG GetMicroTickCount()
{
#if defined WIN32 || defined _WIN32
	LARGE_INTEGER	liPerfCount, liFreq;

	QueryPerformanceCounter(&liPerfCount);
	QueryPerformanceFrequency(&liFreq);

	return (liPerfCount.QuadPart * 1000000LL / liFreq.QuadPart);
#else
	return 0;
#endif
}
inline ULONGLONG GetNanoTickCount()
{
#if defined WIN32 || defined _WIN32
	LARGE_INTEGER	liPerfCount, liFreq;

	QueryPerformanceCounter(&liPerfCount);
	QueryPerformanceFrequency(&liFreq);

	return (liPerfCount.QuadPart * 1000000000LL / liFreq.QuadPart);
#else
	return 0;
#endif
}
#else
int OCR(const Mat &image, POCR_RESULT result);
#endif

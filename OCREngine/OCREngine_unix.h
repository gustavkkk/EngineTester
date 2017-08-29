// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the OCRENGINE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// OCRENGINE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#include "opencv/cv.h"

using namespace cv;

#define DIC_COUNT	5

#ifdef MSVC_OCR
#ifdef OCRENGINE_EXPORTS
#define OCRENGINE_API __declspec(dllexport)
#else
#define OCRENGINE_API __declspec(dllimport)
#endif


typedef struct _tINCCORR_RESULT
{
	Mat				imgIncCorr;
} INCCORR_RESULT, *PINCCORR_RESULT;

typedef struct _tSEGMENT_RESULT
{
	Mat				imgNR;
    std::vector<Rect>	segments;
	Mat				imgSegments;
} SEGMENT_RESULT, *PSEGMENT_RESULT;

typedef struct _tOCR_RESULT
{
	Mat				imgIncCorr;
	SEGMENT_RESULT	seg_res;
    std::vector<char>	string;
} OCR_RESULT, *POCR_RESULT;



// This class is exported from the OCREngine.dll
class OCRENGINE_API COCREngine {
public:
	//CREngine(void);
	// TODO: add your methods here.
};

extern OCRENGINE_API int nOCREngine;

OCRENGINE_API int OCR(const Mat &image, POCR_RESULT result);
#else
    typedef struct _tINCCORR_RESULT
    {
        Mat				imgIncCorr;
    } INCCORR_RESULT, *PINCCORR_RESULT;

    typedef struct _tSEGMENT_RESULT
    {
        Mat				imgNR;
        std::vector<Rect>	segments;
        Mat				imgSegments;
    } SEGMENT_RESULT, *PSEGMENT_RESULT;

    typedef struct _tOCR_RESULT
    {
        Mat				imgIncCorr;
        SEGMENT_RESULT	seg_res;
        std::vector<char>	string;
    } OCR_RESULT, *POCR_RESULT;

    int OCR(const Mat &image, POCR_RESULT result);
#endif

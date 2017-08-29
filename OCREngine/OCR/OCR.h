#pragma once

#include "OCREngine.h"
#include "stdafx.h"
typedef struct _tRECOG_DEGREE
{
	float fDegree;
	int  nKind;
} RECOG_DEGREE, *PRECOG_DEGREE;

#define DIC_COUNT	5

int RecognitionChar(BYTE **ppDicData, BYTE *pCharImgData, int nHeight, int nWidth, vector<WORD> &codes, vector<float> &degrees);
BOOL CheckLockSymbol(BYTE *pDicData, BYTE *pCharImgData,int nHeight,int nWidth);
BOOL CheckLockSymbol_Revised(BYTE **ppDicData, BYTE *pCharImgData, int nHeight, int nWidth);
BOOL LearningLockSymbol(BYTE *pCharImgData,int nHeight,int nWidth);
BOOL LearningChar(BYTE *pCharImgData,int nHeight,int nWidth, WORD wCode, int nKind);

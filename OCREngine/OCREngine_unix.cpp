// OCREngine.cpp : Defines the exported functions for the DLL application.
//

#include "OCREngine/global.h"
#include "OCREngine.h"
#include "ExtractBlack.h"
#include "FindString/FindString.h"
#include "Segmentation/Segmentation.h"
#include "OCR/OCR.h"
#include "opencv2/highgui/highgui.hpp"
#include "QDebug"

using namespace cv;

#ifdef MSVC_OCR
extern uchar *pDicData[DIC_COUNT];
extern uchar *pLockSymDic;
OCRENGINE_API int nOCREngine=0;

OCRENGINE_API int OCR(const Mat &image, POCR_RESULT result)
{
	Mat		imgGray;
    bool	bZoom = FALSE;

	result->seg_res.segments.clear();
	result->string.clear();
#if 1
    std::vector<Substitute_String> subStringList;
	Mat output;
    std::vector<Mat> outputList;
	FindString ocr_string;
	ocr_string.GetWhatYouWantFirst(image);
	subStringList = ocr_string.retrieveSSL();
	if(subStringList.size() == 0)
		return 0;
	///////////////////////////////////
	Substitute_String substring;
    for (std::vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
	{
		substring = *iter;
		if(CheckLockSymbol(pLockSymDic, substring.firstMark.data,substring.firstMark.rows,substring.firstMark.cols))
			substring.existFirstLock = true;
		if(CheckLockSymbol(pLockSymDic, substring.lastMark.data,substring.lastMark.rows,substring.lastMark.cols))
			substring.existLastLock = true;
		if(substring.existFirstLock || substring.existLastLock)
		{
			output = ocr_string.removeLocks(substring);
			outputList.push_back(output);
			break;
		}
	}
	/////////////////////////////////////////
	
	if(outputList.size() == 0)
		return 0;
#else
	FindString ocr_string;
    std::vector<Mat> outputList;
	Mat output;
	output = ocr_string.findString_kojy(image);
	if(output.empty())
		return 0;
	outputList.push_back(output);
#endif
	////////////////////////////////////////
    for (std::vector<Mat>::iterator	iter = outputList.begin(); iter != outputList.end(); iter++)
	{
		SEGMENT_RESULT	SegRes;

		imgGray = *iter;

		if (imgGray.rows < 64)
		{
			resize(imgGray, imgGray, Size(imgGray.cols*3, imgGray.rows*3), 0.0, 0.0, INTER_NEAREST);
			erode(imgGray, imgGray, Mat());
			bZoom = TRUE;
		}

		// Character segmentation
		//erode(imgGray, imgGray, Mat(), Point(-1, -1), 1);
		//imshow("ssss", imgGray);waitKey(3000);
		Segmentation(imgGray, &SegRes);
		if (SegRes.segments.size() > 30 || SegRes.segments.size() < result->seg_res.segments.size())
		{
			continue;
		}

		result->seg_res = SegRes;

		// OCR
		if (pDicData[0] && pDicData[1])
		{
			for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
			{
				Mat		imgChar;
				float	degree;
				WORD	ch = 0x30;

				SegRes.imgNR(result->seg_res.segments[i]).copyTo(imgChar);
				ch = RecognitionChar(pDicData, imgChar.data, imgChar.rows, imgChar.cols, &degree);
				if (ch != 0xFFFF)
				{
					if (*(char *)&ch)
					{
						result->string.push_back(*(char *)&ch);
					}

					if (*((char *)&ch+1))
					{
						result->string.push_back(*((char *)&ch+1));
					}
				}
			}
		}

		cvtColor(imgGray, result->imgIncCorr, CV_GRAY2BGR);
		cvtColor(result->seg_res.imgNR, result->seg_res.imgNR, CV_GRAY2BGR);
		if (result->imgIncCorr.cols % 4)
		{
			result->imgIncCorr(Rect(0, 0, result->imgIncCorr.cols/4*4, result->imgIncCorr.rows)).copyTo(result->imgIncCorr);
		}
	}

	return 0;

	//Mat output;
	//for(int i = 0; i < outputList.size(); i++)
	//{
	//	output = outputList.at(i);
	//	char name[64];
	//	sprintf(name,"output%d",i);
	//	imshow(name,output);
	//}
	if(outputList.size() == 0)
		return 0;
	imgGray = outputList.at(0);

	if (imgGray.rows < 64)
	{
		resize(imgGray, imgGray, Size(imgGray.cols*3, imgGray.rows*3), 0.0, 0.0, INTER_NEAREST);
		erode(imgGray, imgGray, Mat());
		bZoom = TRUE;
	}

	//ProcLinearInterpolation(imgGray);
	//ProcTransformAlpha(imgGray, alphaRotate, alphaSkew);

	//ProcTransformCompensation(imgGray, alphaRotate, alphaSkew);

	// Character segmentation
	//erode(imgGray, imgGray, Mat(), Point(-1, -1), 1);
	Segmentation(imgGray, &result->seg_res);

	// OCR
	if (pDicData[0] && pDicData[1])
	{
		for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
		{
			Mat		imgChar;
			float	degree;
			WORD	ch = 0x30;

			imgGray(result->seg_res.segments[i]).copyTo(imgChar);
			ch = RecognitionChar(pDicData, imgChar.data, imgChar.rows, imgChar.cols, &degree);
			if (ch != 0xFFFF)
			{
				if (*(char *)&ch)
				{
					result->string.push_back(*(char *)&ch);
				}

				if (*((char *)&ch+1))
				{
					result->string.push_back(*((char *)&ch+1));
				}
			}
		}
	}

	cvtColor(imgGray, result->imgIncCorr, CV_GRAY2BGR);
	cvtColor(result->seg_res.imgNR, result->seg_res.imgNR, CV_GRAY2BGR);
	if (result->imgIncCorr.cols % 4)
	{
		result->imgIncCorr(Rect(0, 0, result->imgIncCorr.cols/4*4, result->imgIncCorr.rows)).copyTo(result->imgIncCorr);
	}
	return 0;
}

#else
int nOCREngine=0;
extern BYTE *pDicData[DIC_COUNT];
extern BYTE *pLockSymDic;

int OCR(const Mat &image, POCR_RESULT result)
{
    //qDebug("Stepped into OCREngine");
    Mat		imgGray;

    result->seg_res.segments.clear();
    result->string.clear();
#if 1
    //qDebug("1");
    std::vector<Substitute_String> subStringList;
    Mat output;
    std::vector<Mat> outputList;
    FindString ocr_string;
    //qDebug("2");
    ocr_string.GetWhatYouWantFirst(image);
    //qDebug("GWYWF finished");
    subStringList = ocr_string.retrieveSSL();
    //qDebug("SSL achieved!");
    if(subStringList.size() == 0)
    {
        //qDebug("findStringFailed!");
        return 0;
    }
    ///////////////////////////////////
    //qDebug("3");
    Substitute_String substring;
    for (std::vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
    {
        substring = *iter;
        if(CheckLockSymbol(pLockSymDic, substring.firstMark.data,substring.firstMark.rows,substring.firstMark.cols))
            substring.existFirstLock = true;
        if(CheckLockSymbol(pLockSymDic, substring.lastMark.data,substring.lastMark.rows,substring.lastMark.cols))
            substring.existLastLock = true;
        if(substring.existFirstLock || substring.existLastLock)
        {
            output = ocr_string.removeLocks(substring);
            outputList.push_back(output);
            break;
        }
    }
    /////////////////////////////////////////
    //qDebug("4");
    if(outputList.size() == 0)
        return 0;
#else
    FindString ocr_string;
    std::vector<Mat> outputList;
    Mat output;
    output = ocr_string.findString_kojy(image);
    if(output.empty())
        return 0;
    outputList.push_back(output);
#endif
    //qDebug("Finding String succeeded!");
    ////////////////////////////////////////
    for (std::vector<Mat>::iterator	iter = outputList.begin(); iter != outputList.end(); iter++)
    {
        SEGMENT_RESULT	SegRes;

        imgGray = *iter;

        if (imgGray.rows < 64)
        {
            resize(imgGray, imgGray, Size(imgGray.cols*3, imgGray.rows*3), 0.0, 0.0, INTER_NEAREST);
            erode(imgGray, imgGray, Mat());
        }

        // Character segmentation
        //erode(imgGray, imgGray, Mat(), Point(-1, -1), 1);
        //imshow("ssss", imgGray);waitKey(3000);
        Segmentation(imgGray, &SegRes);
        if (SegRes.segments.size() > 30 || SegRes.segments.size() < result->seg_res.segments.size())
        {
            continue;
        }

        result->seg_res = SegRes;

        // OCR
        if (pDicData[0] && pDicData[1])
        {
            for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
            {
                Mat		imgChar;
                float	degree;
                WORD	ch = 0x30;

                SegRes.imgNR(result->seg_res.segments[i]).copyTo(imgChar);
                ch = RecognitionChar(pDicData, imgChar.data, imgChar.rows, imgChar.cols, &degree);
                if (ch != 0xFFFF)
                {
                    if (*(char *)&ch)
                    {
                        result->string.push_back(*(char *)&ch);
                    }

                    if (*((char *)&ch+1))
                    {
                        result->string.push_back(*((char *)&ch+1));
                    }
                }
            }
        }

        cvtColor(imgGray, result->imgIncCorr, CV_GRAY2BGR);
        cvtColor(result->seg_res.imgNR, result->seg_res.imgNR, CV_GRAY2BGR);
        if (result->imgIncCorr.cols % 4)
        {
            result->imgIncCorr(Rect(0, 0, result->imgIncCorr.cols/4*4, result->imgIncCorr.rows)).copyTo(result->imgIncCorr);
        }
    }

    return 0;

    if(outputList.size() == 0)
        return 0;
    imgGray = outputList.at(0);

    if (imgGray.rows < 64)
    {
        resize(imgGray, imgGray, Size(imgGray.cols*3, imgGray.rows*3), 0.0, 0.0, INTER_NEAREST);
        erode(imgGray, imgGray, Mat());
    }

    Segmentation(imgGray, &result->seg_res);

    // OCR
    if (pDicData[0] && pDicData[1])
    {
        for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
        {
            Mat		imgChar;
            float	degree;
            WORD	ch = 0x30;

            imgGray(result->seg_res.segments[i]).copyTo(imgChar);
            ch = RecognitionChar(pDicData, imgChar.data, imgChar.rows, imgChar.cols, &degree);
            if (ch != 0xFFFF)
            {
                if (*(char *)&ch)
                {
                    result->string.push_back(*(char *)&ch);
                }

                if (*((char *)&ch+1))
                {
                    result->string.push_back(*((char *)&ch+1));
                }
            }
        }
    }
    else{
        //qDebug("pDicData doesn't exist!");
    }

    cvtColor(imgGray, result->imgIncCorr, CV_GRAY2BGR);
    cvtColor(result->seg_res.imgNR, result->seg_res.imgNR, CV_GRAY2BGR);
    if (result->imgIncCorr.cols % 4)
    {
        result->imgIncCorr(Rect(0, 0, result->imgIncCorr.cols/4*4, result->imgIncCorr.rows)).copyTo(result->imgIncCorr);
    }
    return 1;
}
#endif

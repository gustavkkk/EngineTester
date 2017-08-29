// OCREngine_unix.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ExtractBlack.h"
#include "FindString.h"
#include "RemoveNoise.h"
#include "Binarization.h"
#include "FindString\PosterEdge.h"
#include "InclCorr.h"
#include "Segmentation.h"
#include "OCR.h"
#include "opencv2/highgui/highgui.hpp"

#include <baseapi.h>
#include <sys\utime.h>

using namespace cv;
using namespace std;

#ifdef MSVC_OCR//kjy-todo-switching

extern BYTE *g_pDicData[DIC_COUNT];
extern BYTE *g_pLockSymDic;

OCRENGINE_API int nOCREngine=0;

OCRENGINE_API vector<Mat> GetLockList(const Mat &image)
{
	vector<Mat> result;
	FindString ocr_string;
#ifdef USE_NOT_ROTATION
	ocr_string.GetLockList(image,result);
#else
	ocr_string.GetLockListEx(image,result);
#endif
	return result;

}
//////////////////////////////////////////////////////////////
#define MAX_LIC_NUM_CNT	10
#define MAX_BRAND_CNT	10
#define MAX_FACE_SIZE	2000
#define MAX_FACE_CNT	100
#define MAX_CAR_CNT		10

OCRENGINE_API BOOL IsInvolved(Mat &frame)
{
	ULONGLONG ullTime = 0, ullLicNumTime = 0;
	CarNumber	plates[MAX_LIC_NUM_CNT];
	int count = 0;
	ullTime = GetMilliTickCount();
	count = anpr(frame.data, frame.cols, frame.rows, frame.step, 24, 1, 1.0, 1.0, plates);
	ullLicNumTime += (GetMilliTickCount() - ullTime);
	rectangle(frame,Rect(plates[0].x,plates[0].y,plates[0].width,plates[0].height),Scalar(0, 255, 255),2);	

	if(count >0 )
	{
		std::stringstream buffer;
		buffer<<ullLicNumTime;
		MessageBoxA(NULL,buffer.str().c_str(), "Elapsed Time(ms)", MB_OK);
		MessageBox(NULL,plates[0].number, L"NumberPlate", MB_OK);
		return true;
	}
	else
	{
		return false;
	}
}
BYTE g_FaceFeature[MAX_FACE_SIZE];
Mat g_photo;
OCRENGINE_API int containFaces(Mat &img)
{
	Face	faces[MAX_FACE_CNT];
	int		face_cnt;
	if(g_photo.empty())
		return 0;
	/*getFeature(g_photo.data, g_photo.cols, g_photo.rows, g_photo.step, 24, 1, g_FaceFeature);*/
	ULONGLONG ullTime = 0, ullLicNumTime = 0;
	ullTime = GetMilliTickCount();
	face_cnt = findFace1(img.data, img.cols, img.rows, img.step, img.channels()*8, 1, g_FaceFeature, faces);
	ullLicNumTime += (GetMilliTickCount() - ullTime);
	//double2string
	std::ostringstream sstream;
	sstream << faces[0].sim;
	MessageBoxA(NULL,sstream.str().c_str(), "Similarity(%)", MB_OK);
	//ull2string
	std::stringstream buffer;
	buffer<<ullLicNumTime;
	MessageBoxA(NULL,buffer.str().c_str(), "Elapsed Time(ms)", MB_OK);

	if(faces[0].sim > 0.6)
	{
		MessageBox(NULL,L"Found!", L"NumberPlate", MB_OK);
	}
	return face_cnt;
}
vector<KeyPoint> g_CarFeature1;
Mat g_CarFeature2;
float g_CarFeature3[768];
Mat g_CarFeature4;
int FindCar(Mat &img, Car *cars)
{
#if 1
	double	mag = (double)img.rows / g_photo.rows;//sqrt((double)img.cols*img.rows/(m_photo.cols*m_photo.rows));
#else
	double mag = 1.0;
#endif
	Mat		reduced_img;

	if (mag > 1.0)
	{
		resize(img, reduced_img, Size((int)(img.cols/mag+0.5), (int)(img.rows/mag+0.5)));
		return findCarWithFeatures(reduced_img, g_CarFeature1, g_CarFeature2, g_CarFeature3, g_CarFeature4, cars);
	}
	else if (mag < 1.0)
	{
		return findCarWithFeatures(img, g_CarFeature1, g_CarFeature2, g_CarFeature3, g_CarFeature4, cars);
	}

	return findCarWithFeatures(img, g_CarFeature1, g_CarFeature2, g_CarFeature3, g_CarFeature4, cars);
}
OCRENGINE_API int containCars(Mat &img)
{
	Car	cars[MAX_CAR_CNT];
	int	car_cnt;
	if(g_photo.empty())
		return 0;
	/*getCarFeatures(g_photo, g_CarFeature1, g_CarFeature2, g_CarFeature3, g_CarFeature4);*/
	ULONGLONG ullTime = 0, ullLicNumTime = 0;
	ullTime = GetMilliTickCount();
	car_cnt = FindCar(img,cars);
#if 1
	double	mag = (double)img.rows / g_photo.rows;//sqrt((double)img.cols*img.rows/(m_photo.cols*m_photo.rows));
#else
	double mag = 1.0;
#endif
	rectangle(img,Rect(cars[0].x*mag,cars[0].y*mag,cars[0].width*mag,cars[0].height*mag),Scalar(0, 255, 255),2);	
	ullLicNumTime += (GetMilliTickCount() - ullTime);
	//double2string
	std::ostringstream sstream;
	sstream << cars[0].sim;
	MessageBoxA(NULL,sstream.str().c_str(), "Similarity(%)", MB_OK);
	//ull2string
	std::stringstream buffer;
	buffer<<ullLicNumTime;
	MessageBoxA(NULL,buffer.str().c_str(), "Elapsed Time(ms)", MB_OK);
	if(cars[0].sim > 0.6)
	{
		MessageBox(NULL,L"Found!", L"Y/N", MB_OK);
	}
	return 0;
}

OCRENGINE_API void GetFeatures(Mat &img)
{
	ULONGLONG ullTime = 0, ullLicNumTime = 0;
	ullTime = GetMilliTickCount();
	getCarFeatures(img, g_CarFeature1, g_CarFeature2, g_CarFeature3, g_CarFeature4);
	getFeature(img.data, img.cols, img.rows, img.step, 24, 1, g_FaceFeature);
	img.copyTo(g_photo);
	/*Sleep(10);*/
	ullLicNumTime += (GetMilliTickCount() - ullTime);
	//Timing in seconds
	time_t start,end;
	time(&start);
	time(&end);
	double dif = difftime (end,start);
	//double2string
	std::ostringstream sstream;
	sstream << ullLicNumTime;//dif;
	MessageBoxA(NULL,sstream.str().c_str(), "Elapsed Time = ", MB_OK);
	
}
//////////////////////////////////////////////////////////////
#define CHARACTER_NUM 12
OCRENGINE_API int OCR(const Mat &image, POCR_RESULT result)
{
	Mat		imgGray;
	vector<Mat> outputList;
#ifndef OCR_TEST
	#define OCR_TEST
#endif
#ifdef OCR_TEST

	//--gray scale
#ifdef USE_POSTER_EDGE
	cvtColor(image,imgGray,CV_BGR2GRAY);
#else
	int zoomout;
	Mat sized,jonfiltered;
	if(image.rows > image.cols) 
		zoomout = cvRound(image.rows / 450);
	else
		zoomout = cvRound(image.cols / 450);
	
	resize(image,sized,cv::Size(image.cols/zoomout,image.rows/zoomout));
	jonfiltered = posterEdgefilterJON(sized);
	resize(jonfiltered,jonfiltered,cv::Size(image.cols,image.rows));
	cvtColor(jonfiltered,imgGray,CV_BGR2GRAY);
#endif
	//
	Mat goodfeatures = ocr_test_goodfeatures(imgGray);
	//refine by feature points
	Refinebyfeatures(goodfeatures,imgGray);
	//--binarization
	Mat bin;
	bin = Mat::zeros(imgGray.rows,imgGray.cols,CV_8UC1);
	windowthreshold_new(imgGray,bin);
	removeBoundaryNoise(bin);
	imshow("bin before inclcorr",bin);
	waitKey(1000);
	//inclcorr
	double	alphaRotate, alphaSkew;
	ProcLinearInterpolation(bin);
	ProcTransformAlpha(bin, alphaRotate, alphaSkew);
	ProcTransformCompensation(bin, alphaRotate, alphaSkew);
	imshow("bin after inclcorr",bin);
	waitKey(1000);
	//refine
	FindString::RefineString(bin);
	//
	if(!existsLockSymbol(bin))
		return 0;
	imshow("bin after locksymbolremoving",bin);
	waitKey(1000);

	outputList.push_back(bin);
#else
	result->seg_res.segments.clear();
	result->string.clear();
	
	FindString ocr_string;
#ifdef USE_NOT_ROTATION
	ocr_string.GetOutputList(image,outputList);
#else
	ocr_string.GetOutputListEx(image,outputList);
#endif
	if(outputList.size() == 0)
		return 0;
#endif
	///////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_TESSERACT
#ifdef TEST_TESSERACT
	tesseract::TessBaseAPI *myOCR =
		new tesseract::TessBaseAPI();
	const char tessdata[] = "E:\\Tesseract\\tesseract\\tessdata/"; //"E:\\Tesseract\\openalpr-master\\runtime_data\\ocr\\tessdata";//"E:\\Tesseract\\tesseract\\tessdata-master\\";
	bool isInitSuccess = !myOCR->Init(tessdata, "eng");
#endif
	////////////////////////////////////////////////////////////////////////////////////////////////////
	for (vector<Mat>::iterator	iter = outputList.begin(); iter != outputList.end(); iter++)
	{
		imgGray = *iter;

		ProcCurveCorrection(imgGray);
#define TEST_TESSERACT_LINE
#ifdef TEST_TESSERACT_LINE
		if (isInitSuccess) {

			myOCR->SetVariable("tessedit_char_whitelist", "0123456789-.");
			tesseract::PageSegMode pagesegmode = static_cast<tesseract::PageSegMode>(7); // treat the image as a single text line
			myOCR->SetPageSegMode(pagesegmode);

			Mat conv;
			threshold(imgGray, conv, 128, 255, THRESH_BINARY_INV);
			dilate(conv, conv, Mat(), cv::Point(-1, -1), 1, 1);
			imshow("conv", conv);
			waitKey(1000);

			myOCR->TesseractRect(conv.data, 1, conv.step1(), 0, 0, conv.cols, conv.rows);
			const char *text = myOCR->GetUTF8Text();

			string t = text;
			t.erase(std::remove(t.begin(), t.end(), '\n'), t.end());

			MessageBoxA(NULL, text, "Tesseract Says", MB_OK);

			delete[] text;
		}
#else
#define TEST_TESSERACT_CHAR
#endif

		//FindString::RefineString(imgGray);
		//Initialize result;-kjy-todo
#define CLEAN_RESULT
#ifdef CLEAN_RESULT
		result->confidence.clear();
		result->string.clear();
		result->seg_res.segments.clear();
#endif
		// Character segmentation
		SEGMENT_RESULT	SegRes;

		//erode(imgGray, imgGray, Mat(), Point(-1, -1), 1);
		//imshow("ssss", imgGray);waitKey(3000);
		Segmentation(imgGray, &SegRes);
		if (SegRes.segments.size() > 35 || SegRes.segments.size() < result->seg_res.segments.size())
		{
			continue;
		}

		result->seg_res = SegRes;

		// OCR
		if (g_pDicData[0] && g_pDicData[1])
		{
			result->confidence.resize(result->seg_res.segments.size());

			for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
			{
				Mat				imgChar;
				int				cnt;

				SegRes.imgNR(result->seg_res.segments[i]).copyTo(imgChar);

#ifdef TEST_TESSERACT_CHAR

				if (isInitSuccess) {
					myOCR->SetVariable("save_blob_choices", "T");
					myOCR->SetVariable("debug_file", "/dev/null");
					myOCR->SetVariable("tessedit_char_whitelist", "0123456789-.");
					myOCR->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);

					Mat conv;
					threshold(imgChar, conv, 128, 255, THRESH_BINARY_INV);
					//dilate(conv, conv, Mat(), cv::Point(-1, -1), 1, 1);

					myOCR->TesseractRect(conv.data, 1, conv.step1(), 0, 0, conv.cols, conv.rows);
					const char *text = myOCR->GetUTF8Text();

					imshow(text, conv);
					waitKey(1000);

					//MessageBoxA(NULL, text, "Tesseract Says", MB_OK);

					delete[] text;

				}
#endif
				cnt = RecognitionChar(g_pDicData, imgChar.data, imgChar.rows, imgChar.cols, result->confidence[i].codes, result->confidence[i].degrees);
				if (result->confidence[i].codes[0] != 0xFFFF)
				{
					if (*(char *)&result->confidence[i].codes[0])
					{
						result->string.push_back(*(char *)&result->confidence[i].codes[0]);
					}

					if (*((char *)&result->confidence[i].codes[0]+1))
					{
						result->string.push_back(*((char *)&result->confidence[i].codes[0]+1));
					}
				}
			}
		}
		/***********************************************************************************************************************/
#undef OCR_REFINE_STRING
#ifdef OCR_REFINE_STRING
		int PosOfA = -1;
		int count = 0;
		int countofA = 0;
		vector<char>::iterator	PosOfA_iter;
		for (vector<char>::iterator	iter = result->string.begin(); iter != result->string.end(); iter++)
		{
			
			char letter = *iter;

			if(letter == 'O' ||
			   letter == '0')
			{
				if(!countofA)
				{
					PosOfA = count;
					PosOfA_iter = iter;
				}
				countofA++;
			}
			count++;
		}
		if(countofA > 2)
			continue;
		if(PosOfA > (CHARACTER_NUM - 1))
		{
			vector<char>::iterator iter;
			vector<cv::Rect>::iterator iter_rect;
			for(int i = PosOfA; i < count; i++)
			{
				iter = result->string.end();
				iter--;
				result->string.erase(iter);
				iter_rect = result->seg_res.segments.end();
				iter_rect--;
				result->seg_res.segments.erase(iter_rect);
			}
		}
		else if(PosOfA + CHARACTER_NUM < count)
		{
			vector<char>::iterator iter;
			vector<cv::Rect>::iterator iter_rect;
			for(int i = 0; i <= PosOfA; i++)
			{
				iter = result->string.begin();
				result->string.erase(iter);
				iter_rect = result->seg_res.segments.begin();
				result->seg_res.segments.erase(iter_rect);
			}
		}
#endif
		/************************************************************************************************************************/
		cvtColor(imgGray, result->imgIncCorr, CV_GRAY2BGR);
		cvtColor(result->seg_res.imgNR, result->seg_res.imgNR, CV_GRAY2BGR);
		if (result->imgIncCorr.cols % 4)
		{
			result->imgIncCorr(Rect(0, 0, result->imgIncCorr.cols/4*4, result->imgIncCorr.rows)).copyTo(result->imgIncCorr);
		}
		if(result->string.size() >= CHARACTER_NUM)
			break;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef TEST_TESSERACT
	if (isInitSuccess)
	{
		myOCR->Clear();
		myOCR->End();
	}
#endif
	///////////////////////////////////////////////////////////////////////////////////////////////////
	return 0;

#if 0
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
	}

	//ProcLinearInterpolation(imgGray);
	//ProcTransformAlpha(imgGray, alphaRotate, alphaSkew);

	//ProcTransformCompensation(imgGray, alphaRotate, alphaSkew);

	// Character segmentation
	//erode(imgGray, imgGray, Mat(), Point(-1, -1), 1);
	Segmentation(imgGray, &result->seg_res);

	// OCR
	if (g_pDicData[0] && g_pDicData[1])
	{
		for (int i = 0; i < (int)result->seg_res.segments.size(); i++)
		{
			Mat		imgChar;
			float	degree;
			WORD	ch = 0x30;

			imgGray(result->seg_res.segments[i]).copyTo(imgChar);
			ch = RecognitionChar(g_pDicData, imgChar.data, imgChar.rows, imgChar.cols, &degree);
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
#endif
}

#else

int nOCREngine=0;
extern BYTE *pDicData[DIC_COUNT];
extern BYTE *pLockSymDic;

int OCR(const Mat &image, POCR_RESULT result)
{
    Mat		imgGray;

    result->seg_res.segments.clear();
    result->string.clear();
#if 1
    vector<Substitute_String> subStringList;
    Mat output;
    vector<Mat> outputList;
    FindString ocr_string;
    subStringList = ocr_string.GetSubstituteStringList(image);
    if(subStringList.size() == 0)
        return 0;
    //kojy-totod-test-getlocklist//
#ifdef TEST_GETLOCKLIST
    vector<Mat> locklist = ocr_string.getlockList();
    for (vector<Mat>::iterator	iter = locklist.begin(); iter != locklist.end(); iter++)
    {
        Mat lock = *iter;
        imshow("test-locklist",lock);
        waitKey(1000);
    }
    return 0;
#endif
    ///////////////////////////////////
    Substitute_String substring;
    for (vector<Substitute_String>::iterator	iter = subStringList.begin(); iter != subStringList.end(); iter++)
    {
        substring = *iter;
        if(CheckLockSymbol(pLockSymDic, substring.firstMark.data,substring.firstMark.rows,substring.firstMark.cols))
            substring.existFirstLock = true;
        if(CheckLockSymbol(pLockSymDic, substring.lastMark.data,substring.lastMark.rows,substring.lastMark.cols))
            substring.existLastLock = true;
#ifdef OCR_ONCE
        imshow("test",substring.thr);
        waitKey(1000);
#endif
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
    vector<Mat> outputList;
    Mat output;
    output = ocr_string.findString_kojy(image);
    if(output.empty())
        return 0;
    outputList.push_back(output);
#endif
    ////////////////////////////////////////
    for (vector<Mat>::iterator	iter = outputList.begin(); iter != outputList.end(); iter++)
    {
        imgGray = *iter;

        ProcCurveCorrection(imgGray);

        // Character segmentation
        SEGMENT_RESULT	SegRes;

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
#endif

#include "stdafx.h"
#include "InclCorr.h"
#include "Segmentation.h"
#include <opencv2/highgui/highgui.hpp>


#define PI	3.1415926535897932384626433832795


void fnGetMinMaxValForAutoLevel(const Mat &image, BYTE &ucMin, BYTE &ucMax)
{
	BYTE	*p = image.data;
	int		x, y,
			cntPerLevels[256];
	int		delta = (int)(0.006 * image.cols * image.rows);

    //ZeroMemory(cntPerLevels, sizeof(cntPerLevels));//kjy-todo-2015.6.17
    memset(cntPerLevels,0,sizeof(cntPerLevels));
	// Histogram
	for (y = 0; y < image.rows; y++)
	{
		for (x = 0; x < image.cols; x++,p++)
		{
			cntPerLevels[*p]++;
		}
	}

	x = y = 0;

	// Foreground color
	for (ucMin = 0; ucMin < 255; ucMin++)
	{
		x += cntPerLevels[ucMin];

		if (x > delta)
		{
			break;
		}
	}

	// Background color
	for (ucMax = 255; ucMax > 0; ucMax--)
	{
		y += cntPerLevels[ucMax];

		if (y > delta)
		{
			break;
		}
	}

	if (ucMin > ucMax)
	{
		for (ucMin = 0; ucMin < 255; ucMin++)
		{
			if (cntPerLevels[ucMin] > 0)
			{
				break;
			}
		}

		for (ucMax = 255; ucMax > 0; ucMax--)
		{
			if (cntPerLevels[ucMax] > 0)
			{
				break;
			}
		}
	}
}

void ProcLinearInterpolation(Mat &image)
{
	BYTE	*p = NULL;
	BYTE	ucMax = 0, ucMin = 0;

	fnGetMinMaxValForAutoLevel(image, ucMin, ucMax);

	if (ucMax-ucMin >= 250 || ucMax-ucMin <= 0)
	{
		return;
	}

	p = image.data;

	double	temp = 255 / (double)(ucMax - ucMin);

	for (int y = 0; y < image.rows; y++)
	{
		for (int x = 0; x < image.cols; x++, p++)
		{
			if (*p <= ucMin)
			{
				*p = 0;
				continue;
			}

			if (*p >= ucMax)
			{
				*p = 255;
				continue;
			}

			*p = (int)ceil(temp*(double)(*p-ucMax));
		}
	}
}

void ProcTransformAlpha(const Mat &image, double &alphaRotate, double &alphaSkew)
{
	if (image.empty())
	{
		alphaRotate = 0.0;
		alphaSkew = 0.0;

		return;
	}

	double	step = 0.01,
			alphaUp = 0.17, alphaDown = -0.17,
			alphaMin, alphaMax,
			tanalpha;
	double	*P,
			*dOffDelta = new double[image.cols],
			val;//pi / 10
	int		max, min,
			sx = image.cols / 4, sy,
			ex = image.cols * 3 / 4, ey,
			x, y,
			*p, temp = ex - sx;
	int		count, COUNT, TEMP,
			*iOffset = new int[image.cols];
	BOOL	bFlag;

	do
	{
		max = min = 0;
		bFlag = FALSE;

		for (double j = alphaDown; j <= alphaUp; j += step)
		{
			tanalpha = tan(j);
			count = 0;

			if (j >= 0)
			{
				for (x = 0, p = &iOffset[sx], P = &dOffDelta[sx]; x <= temp; x++, p++, P++)
				{
					val = tanalpha * x;
					*p = (int)val;
					*P = val - (double)(*p);
				}

				ey = image.rows - iOffset[ex] - 1;
				COUNT = ey;//- sy

				for (y = 0; y < ey; y++)
				{//, COUNT++
					for (x = sx; x <= ex; x++)
					{
						val = (1 - dOffDelta[x]) * (double)(image.data[(y+iOffset[x])*image.cols+x]) + 
							dOffDelta[x] * (double)(image.data[(y+iOffset[x]+1)*image.cols+x]);
						if (val < 200)
						{
							break;
						}
					}

					if (x > ex)
					{
						count++;
					}
				}
			}
			else
			{
				for (x = 0, p = &iOffset[sx], P = &dOffDelta[sx]; x <= temp; x++, p++, P++)
				{
					val = tanalpha * x;
					*p = (int)val;
					*P = (double)(*p) - val;
				}

				COUNT = image.rows - (1 - iOffset[ex]);

				for (y = 1 - iOffset[ex]; y < image.rows; y++)
				{//, COUNT++
					for (x = sx; x <= ex; x++)
					{
						val = (1 - dOffDelta[x]) * (double)(image.data[(y+iOffset[x])*image.cols+x]) + 
							dOffDelta[x] * (double)(image.data[(y+iOffset[x]-1)*image.cols+x]);
						if (val < 200)
						{
							break;
						}
					}

					if (x > ex)
					{
						count++;
					}
				}
			}

			if (count > 0)
			{
				TEMP = count + (min - COUNT);
				if (TEMP > max || max == 0)
				{
					max = count;
					min = COUNT;
					alphaMin = j - step;
					alphaMax = j + step;
				}
				else
				{
					if (TEMP == max)
					{
						alphaMax = j + step;
					}
				}

				bFlag = TRUE;
			}
		}

		if (!bFlag)
		{
			alphaMin = 0;
			alphaMax = 0;

			break;
		}

		alphaDown = alphaMin;
		alphaUp = alphaMax;
		step /= 10;
	} while (step > 0.00001);

	delete []iOffset;
	delete []dOffDelta;

	alphaRotate = (alphaMin + alphaMax) / 2;

	step = 0.0018;
	alphaUp = alphaRotate + 0.009;
	alphaDown = alphaRotate - 0.009;

	dOffDelta = new double[image.rows];

	sy = image.rows / 4;
	ey = image.rows * 3 / 4;
	temp = ey - sy;

	iOffset = new int[image.rows];

	do
	{
		max = min = 0;
		bFlag = FALSE;

		for (double j = alphaDown; j <= alphaUp; j += step)
		{
			tanalpha = -tan(j);
			count = 0;

			if (j >= 0)
			{
				for (y = 0, p = &iOffset[sy], P = &dOffDelta[sy]; y <= temp; y++, p++, P++)
				{
					val = tanalpha * y;
					*p = (int)val;
					*P = val - (double)(*p);
				}

				ex = image.cols - iOffset[ey] - 1;
				COUNT = ex;//- sx

				for (x = 0; x < ex; x++)
				{//, COUNT++
					for (y = sy; y <= ey; y++)
					{
						val = (1 - dOffDelta[y]) * (double)(image.data[y*image.cols+(x+iOffset[y])]) + 
							dOffDelta[y] * (double)(image.data[y*image.cols+(x+iOffset[y]+1)]);
						if (val < 200)
						{
							break;
						}
					}

					if (y > ey)
					{
						count++;
					}
				}
			}
			else
			{
				for (y = 0, p = &iOffset[sy], P = &dOffDelta[sy]; y <= temp; y++, p++, P++)
				{
					val = tanalpha * y;
					*p = (int)val;
					*P = (double)(*p) - val;
				}

				COUNT = image.cols - (1 - iOffset[ey]);

				for (x = 1 - iOffset[ey]; x < image.cols; x++)
				{//, COUNT++
					for (y = sy; y <= ey; y++)
					{
						val = (1 - dOffDelta[y]) * (double)(image.data[y*image.cols+(x+iOffset[y])]) + 
							dOffDelta[y] * (double)(image.data[y*image.cols+(x+iOffset[y]-1)]);
						if (val < 200)
						{
							break;
						}
					}

					if (y > ey)
					{
						count++;
					}
				}
			}

			if (count > 0)
			{
				TEMP = count + (min - COUNT);
				if (TEMP > max || max == 0)
				{
					max = count;
					min = COUNT;
					alphaMin = j - step;
					alphaMax = j + step;
				}
				else
				{
					if (TEMP == max)
					{
						alphaMax = j + step;
					}
				}

				bFlag = TRUE;
			}
		}

		if (!bFlag)
		{
			alphaMin = alphaRotate;
			alphaMax = alphaRotate;

			break;
		}

		alphaDown = alphaMin;
		alphaUp = alphaMax;
		step /= 10;
	} while (step > 0.00001);

	delete []iOffset;
	delete []dOffDelta;

	alphaSkew = (alphaMin + alphaMax) / 2;
}

inline double fnInterpolation(double x)
{
	x = fabs(x);
	if (x <= 1)
	{
		return (1.75 * x*x*x - 2.75 * x*x + 1);
	}

	return (-0.25 * x*x*x + 1.25 * x*x - 2 * x + 1);
}

inline double fnInterpolation1(double x)
{
	double	d;

	x = fabs(x);
	if (x <= 1)
	{
		d = x * x;

		return (0.5 * d*x - d + 0.666667);
	}

	d = 2 - x;

	return (0.166666667 * d * d * d);
}

BYTE fnGetCompensateInclinationValue(const Mat &image, double x, double y)
{
	if (x <= 1 || x >= image.cols-1)
	{
		return 0xFF;
	}

	if (y <= 1 || y >= image.rows-1)
	{
		return 0xFF;
	}

	int		m, n,
			p = (int)y, q = (int)x;
	double	a = ((double)y - p), b = ((double)x - q);
	double	pv, ev = 0;
	int		tom = 3, ton = 3;
	int		pys = image.rows - p, pxs = image.cols - q;

	if (pys < 3)
	{
		tom = pys;
	}

	if (pxs < 3)
	{
		ton = pxs;
	}

	for (m = -1; m < tom; m++)
	{
		for (n = -1; n < ton; n++)
		{
			pv = (double)image.data[(p+m)*image.cols+(q+n)];
			ev += (pv * fnInterpolation((double)m-a) * fnInterpolation(b-(double)n));
		}
	}
	
	BYTE	ipv = (BYTE)ev;

	if (ipv < 20 || ipv > 235)
	{
		ev = 0;

		for (m = -1; m < tom; m++)
		{
			for (n = -1; n < ton; n++)
			{
				pv = (double)image.data[(p+m)*image.cols+(q+n)];
				ev += (pv * fnInterpolation1((double)m-a) * fnInterpolation1(b-(double)n));
			}
		}
	}

	return (ev > 127 ? 0xFF : 0x00);//ev;//
}

int ProcTransformCompensation(Mat &image, double duInclineAlpha, double duSkewAlpha)
{
	if (image.empty())
	{
		return 0;
	}

	if (duInclineAlpha < 0.001 && duInclineAlpha > -0.001 && duSkewAlpha-duInclineAlpha < 0.001 && duSkewAlpha-duInclineAlpha > -0.001)
	{
		return 0;
	}

	int		ohx, ohy;

	ohx = image.cols / 2, 
	ohy = image.rows / 2;

	double	ml = sqrt((double)ohx*ohx+ohy*ohy); 
	double	angle = atan2((double)ohy, (double)ohx),
			_sin = sin(duInclineAlpha),
			_cos = cos(duInclineAlpha),
			_tan = tan(duSkewAlpha-duInclineAlpha),
			len;

	// Center of rotation
	int		cx = (int)ceil(max(ml*cos(angle+duInclineAlpha), ml*cos(angle-duInclineAlpha)));
	int		cy = (int)ceil(max(ml*sin(angle+duInclineAlpha), ml*sin(angle-duInclineAlpha)));

	cx = cx + (int)(cy * abs(_tan));

	int		esx = 2 * cx + 1;//	int esx = (2 * cx + 1 + 3) / 4 * 4;
	int		esy = 2 * cy + 1;
	int		imgSize = esx * esy,
			x, y;
	double	x0, y0;
	Mat		imgTarget(esy, esx, CV_8UC1);
	BYTE	*p1, *p2, *p3, *p4;

	memset(imgTarget.data, 0xff, imgSize);

	imgTarget.data[cy*esx+cx] = image.data[ohy*image.cols+ohx];//Center

	p1 = &imgTarget.data[cy*esx+cx+1];
	p2 = &imgTarget.data[cy*esx+cx-1];
	len = 1;

	for (x = 1; x < cx; x++, p1++, p2--, len++)
	{//x-axis
		x0 = x * _cos;
		y0 = x * _sin;

		*p1 = fnGetCompensateInclinationValue(image, ohx+x0, ohy+y0);
		*p2 = fnGetCompensateInclinationValue(image, ohx-x0, ohy-y0);
	}

	p1 = &imgTarget.data[cy*esx+cx+esx];
	p2 = &imgTarget.data[cy*esx+cx-esx];
	ml = PI / 2;
	len = 1;

	for (y = 1; y < cy; y++, p1 += esx, p2 -= esx, len++)
	{//y-axis
		x0 = y * (_cos * -_tan - _sin);
		y0 = y * (_sin * -_tan + _cos);

		*p1 = fnGetCompensateInclinationValue(image, ohx+x0, ohy+y0);
		*p2 = fnGetCompensateInclinationValue(image, ohx-x0, ohy-y0);
	}

	for (y = 1; y < cy; y++)
	{
		p1 = &imgTarget.data[(cy+y)*esx+cx+1];
		p2 = &imgTarget.data[(cy+y)*esx+cx-1];
		p3 = &imgTarget.data[(cy-y)*esx+cx-1];
		p4 = &imgTarget.data[(cy-y)*esx+cx+1];

		for (x = 1; x < cx; x++, p1++, p2--, p3--, p4++)
		{
			x0 = x * _cos + y * (_cos * -_tan - _sin);
			y0 = x * _sin + y * (_sin * -_tan + _cos);

			*p1 = fnGetCompensateInclinationValue(image, ohx+x0, ohy+y0);
			*p3 = fnGetCompensateInclinationValue(image, ohx-x0, ohy-y0);

			x0 = -x * _cos + y * (_cos * -_tan - _sin);
			y0 = -x * _sin + y * (_sin * -_tan + _cos);

			*p2 = fnGetCompensateInclinationValue(image, ohx+x0, ohy+y0);
			*p4 = fnGetCompensateInclinationValue(image, ohx-x0, ohy-y0);
		}
	}

	image.release();
	image = imgTarget;

	return imgSize;
}

void ProcCurveCorrection(Mat &image)
{
#define SPLIT_CNT	3

	Mat		org[SPLIT_CNT], dst[SPLIT_CNT],
			result;
	double	alphaRotate[SPLIT_CNT], alphaSkew;
	int		top = 0, bottom = 0, max_t = 0, max_b = 0;

	//imshow("bin", image);waitKey(1000);
	for (int i = 0; i < SPLIT_CNT; i++)
	{
		int		w, h;
		double	tan_alpha;
		Mat		tmp;
		Rect	rc1, rc2;

		if (SPLIT_CNT-1 == i)
		{
			w = image.cols - image.cols / SPLIT_CNT * i;
		}
		else
		{
			w = image.cols / SPLIT_CNT;
		}

		Mat(image, Rect(image.cols/SPLIT_CNT*i, 0, w, image.rows)).copyTo(org[i]);
		ProcLinearInterpolation(org[i]);
		ProcTransformAlpha(org[i], alphaRotate[i], alphaSkew);
		tan_alpha = tan(alphaRotate[i]);

		rc1 = Rect(0, 0, org[i].cols, org[i].rows);
		FixSegmentSize(org[i], rc1);

		h = image.rows + (int)abs(w*tan_alpha);
		dst[i].create(h, w, CV_8UC1);

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				int	yy = y + (int)(x * tan_alpha);

				if (tan_alpha > 0)
				{
					yy -= h - image.rows;
				}

				if (yy >= 0 && yy < org[i].rows)
				{
					dst[i].data[dst[i].step*y+x] = org[i].data[org[i].step*yy+x];
				}
				else
				{
					dst[i].data[dst[i].step*y+x] = 255;
				}
			}
		}

		rc2 = Rect(0, 0, dst[i].cols, dst[i].rows);
		FixSegmentSize(dst[i], rc2);

		if (rc2.height > rc1.height)
		{
			org[i].copyTo(dst[i]);
			h = image.rows;
		}

		int	prev_t = max_t, prev_b = max_b;

		if (tan_alpha < 0)
		{
			top -= h - image.rows;
			bottom += h - image.rows;
		}
		else
		{
			top += h - image.rows;
			bottom -= h - image.rows;
		}

		max_t = max(max_t, top);
		max_b = max(max_b, bottom);

		if (!result.empty())
		{
			result.copyTo(tmp);
		}

		result.create(image.rows+max_t+max_b, image.cols, CV_8UC1);
		result.setTo(255);

		if (!tmp.empty())
		{
			if (max_t > prev_t)
			{
				Mat	roi(result, Rect(0, max_t-prev_t, tmp.cols, tmp.rows));

				bitwise_and(roi, tmp, roi);
			}
			else //if (max_b > prev_b)
			{
				Mat	roi(result, Rect(0, 0, tmp.cols, tmp.rows));

				bitwise_and(roi, tmp, roi);
			}
			//imshow("tmp", tmp);waitKey(1);
			//imshow("result", result);waitKey(1000);
		}

		if (max_t > prev_t)
		{
			Mat	roi(result, Rect(image.cols/SPLIT_CNT*i, 0, dst[i].cols, dst[i].rows));

			bitwise_and(roi, dst[i], roi);
		}
		else if (max_b > prev_b)
		{
			Mat	roi(result, Rect(image.cols/SPLIT_CNT*i, result.rows-dst[i].rows, dst[i].cols, dst[i].rows));

			bitwise_and(roi, dst[i], roi);
		}
		else
		{
			 if (tan_alpha > 0)
			 {
				Mat	roi(result, Rect(image.cols/SPLIT_CNT*i, max_t-top, dst[i].cols, dst[i].rows));

				bitwise_and(roi, dst[i], roi);
			 }
			 else
			 {
				 Mat	roi(result, Rect(image.cols/SPLIT_CNT*i, result.rows-(max_b-bottom+dst[i].rows), dst[i].cols, dst[i].rows));

				bitwise_and(roi, dst[i], roi);
			 }
		}

		//imshow("org", org[i]);waitKey(1);
		//imshow("dst", dst[i]);waitKey(5000);
	}

	//imshow("result", result);waitKey(1000);
	result.copyTo(image);
}

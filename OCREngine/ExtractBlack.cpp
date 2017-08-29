#include "stdafx.h"
#include "ExtractBlack.h"
//#include "opencv2/highgui/highgui.hpp"


BOOL CorrectK(Mat &image)
{
    BYTE	*pTmp;
	int		x, y;
	int		k_cnt[256];
	BYTE	black;

	pTmp = image.data;

	memset(k_cnt, 0, 256*sizeof(int));

	for (y = 0; y < image.rows; y++)
	{
		for (x = 0; x < image.cols; x++)
		{
			k_cnt[255-*pTmp++]++;
		}
	}

#define RANGE		25
#define MIN_MAX		192
#define MAX_MIN		64
#define MAX_RATIO	10

	int	min_k, max_k,
		s, ds,
		i;

	min_k = 0;
	s = 0;
	ds = 0;

	for (i = 0; i < MAX_MIN; i++)
	{
		ds += k_cnt[i];

		if (k_cnt[i] >= k_cnt[min_k])
		{
			min_k = i;

			s += ds;
			ds = 0;
		}
	}

	if (min_k < RANGE / 2)
	{
		min_k = RANGE;//RANGE / 2;
	}
	else
	{
		min_k *= 2;
	}

	s = 0;

	for (i = min_k+(255-min_k)/2+1; i < 256; i++)
	{
		s += k_cnt[i];
	}

	s /= MAX_RATIO;

	for (max_k = 255; max_k >= 0; max_k--)
	{
		s -= k_cnt[max_k];
		if (s < 0)
		{
			break;
		}
	}

	pTmp = image.data;

	for (y = 0; y < image.rows; y++)
	{
		for (x = 0; x < image.cols; x++)
		{
			black  = 255 - *pTmp;

			if (black < min_k)
			{
				black = 0;
			}
			else if (black > max_k)
			{
				black = 255;
			}
			else
			{
				black = (BYTE)(((int)black - min_k) * 255 / (max_k - min_k));
			}

			*(pTmp++) = 255 - black;
		}
	}

	return TRUE;
}

int GetThreshold(const Mat &image)
{
	int		offset = 0;
	int		k_cnt[256], avg = 0,
			_min = 255, _max = 0,
			min_histo, new_histo;
	int		threshold = 127;

	memset(k_cnt, 0, 256*sizeof(int));

	for (int y = 0; y < image.rows; y++)
	{
		for (int x = 0; x < image.cols; x++)
		{
			int	n = 255 - image.data[offset+x];

			if (n > _max)
			{
				_max = n;
			}

			if (n < _min)
			{
				_min = n;
			}

			k_cnt[n]++;
			avg += (int)image.data[offset+x];
		}

		offset += image.step;
	}

	avg /= image.rows * image.cols;

#define RANGE		25
#define MIN_HISTO	80
#define MAX_HISTO	176

	new_histo = 0;

	for (int i = 0; i < 25; i++)
	{
		new_histo += k_cnt[i];
	}

	threshold = MIN_HISTO;

	for (int i = RANGE/2+1; i < 256-RANGE/2; i++)
	{
		new_histo = new_histo - k_cnt[i-RANGE/2-1] + k_cnt[i+RANGE/2];

		if (i >= MIN_HISTO && i < MAX_HISTO)
		{
			if (min_histo > new_histo)
			{
				min_histo = new_histo;
				threshold = i;
			}
		}
		else
		{
			min_histo = new_histo;
		}
	}

	threshold = 255 - (threshold - RANGE / 2 - 1);

	if (_max-_min < 63)
	{
		if (avg > threshold)
		{
			return 0;
		}
		else
		{
			return 256;
		}
	}

	return threshold;
}

void Convert2Gray(Mat &image)
{
	//int	width = image.cols * 1600 / max(image.cols, image.rows),
	//	height = image.rows * 1600 / max(image.cols, image.rows);
	Mat	imgGray(image.rows, image.cols, CV_8UC1);
	int	offset = 0;

	//resize(image, image, Size(width, height));
	cvtColor(image, image, CV_BGR2HSV);

	for (int y = 0; y < image.rows; y++)
	{
		for (int x = 0; x < image.cols; x++)
		{
#if 0
			if (image.data[offset+1] < 40
				|| image.data[offset+2] < 64)
			{
				image.data[offset] = image.data[offset+1] = image.data[offset+2];
			}
			else
			{
				image.data[offset] = image.data[offset+1] = image.data[offset+2] = 255;
			}
#else
			double	d = (double)image.data[offset+2] * sqrt(1.0+(double)image.data[offset+1]*image.data[offset+1]/65025.0);

			d = d * 255.0 / 360.6244584;
			if (d > 255.0)
			{
				d = 255.0;
			}

			imgGray.data[offset/3] = (uchar)d;//d < 128 ? (uchar)d : 255;
#endif

			offset += 3;
		}
	}

	CorrectK(imgGray);

#if 0
#define K	8

	Mat	m(image.rows, image.cols, CV_16UC1);

	m.setTo(0);
	offset = 0;

	for (int y = K; y < imgGray.rows-K; y++)
	{
		for (int x = K; x < imgGray.cols-K; x++)
		{
			Mat	imgWindow(imgGray, Rect(x-K, y-K, K*2+1, K*2+1));
			int	threshold;

			threshold = GetThreshold(imgWindow);
			if (threshold)
			{
				if ((int)imgGray.data[offset+x] < threshold)
				{
					(*(unsigned short *)(m.data+y*m.step+x*2))++;
				}
			}
		}

		offset += imgGray.step;
	}

	offset = 0;

	for (int y = 0; y < imgGray.rows; y++)
	{
		for (int x = 0; x < imgGray.cols; x++)
		{
			int nn = *(unsigned short *)(m.data+y*m.step+x*2);
			//imgGray.data[offset+x] = *(unsigned short *)(m.data+y*m.step+x*2) * 255 / (4 * K * K + 4 * K + 1);
			imgGray.data[offset+x] = *(unsigned short *)(m.data+y*m.step+x*2) ? 0 : 255;
		}

		offset += imgGray.step;
	}
#endif

	imgGray.copyTo(image);
	//cvtColor(image, image, CV_GRAY2BGR);
}

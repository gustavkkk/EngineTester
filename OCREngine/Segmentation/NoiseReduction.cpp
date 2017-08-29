#include "stdafx.h"
#include "NoiseReduction.h"


inline bool SetRectByteImage(Mat &image, Rect rect, const BYTE *pByteImage)
{
	BYTE	*pImage = image.data;
	int		x, y;

	pImage += image.step * rect.y;

	for (y = rect.y; y < rect.br().y; y++)
	{
		for (x = rect.x; x < rect.br().x; x++)
		{
			*(pImage+x) = *(pByteImage++);
		}

		pImage += image.step;
	}

	return true;
}

void CleanNoise(const Mat &imgSrc, Mat &imgDst)
{
	int	cxWidth, cyHeight;

	imgSrc.copyTo(imgDst);
	cxWidth = imgDst.cols;
	cyHeight = imgDst.rows;

	if (cxWidth < 3 || cyHeight < 3)
	{
		return;
	}

	const int	m = max(1, ((int)(0.5+sqrt((double)max(1, cxWidth*cxWidth/190000)))-1)/2);
	const int	side = 2 * m + 1,
				init_d_offset1 = -m * cxWidth - m, init_d_offset2 = m * cxWidth - m,
				final_d_offset1 = -m * cxWidth + m + 1, final_d_offset2 = m * cxWidth + m + 1;
	BYTE	*pImage,
			*pWhiteRectImage;
	int		x, y,
			offset, d_offset1, d_offset2,
			s1, s2;

	pImage = imgDst.data;

	pWhiteRectImage = new BYTE[side*side];
	memset(pWhiteRectImage, 0xFF, side*side);

	offset = m * cxWidth + m;
	for (y = m; y < cyHeight-m; y++)
	{
		for (x = m; x < cxWidth-m; x++,offset++)
		{
			if (*(pImage+offset) == 255)
			{
				continue;
			}

			d_offset1 = init_d_offset1;
			d_offset2 = init_d_offset2;
			s1 = 0;
			while (d_offset1 < final_d_offset1)
			{
				s1 += (1 - *(pImage+offset+d_offset1) / 255) + (1 - *(pImage+offset+d_offset2) / 255);

				d_offset1++;
				d_offset2++;
			}

			d_offset1 = init_d_offset1;
			d_offset2 = final_d_offset1 - 1;
			while (d_offset1 < init_d_offset2)
			{
				s1 += (1 - *(pImage+offset+d_offset1) / 255) + (1 - *(pImage+offset+d_offset2) / 255);

				d_offset1 += cxWidth;
				d_offset2 += cxWidth;
			}

			if (s1 < m)
			{
				Rect	rc;

				rc.x = x - m;
				rc.y = y - m;
				rc.width = 2 * m + 1;
				rc.height = 2 * m + 1;
				SetRectByteImage(imgDst, rc, pWhiteRectImage);
			}

			s2 = (1 - *(pImage+offset-cxWidth) / 255) + (1 - *(pImage+offset-1) / 255)
				+ (1 - *(pImage+offset+1) / 255) + (1 - *(pImage+offset+cxWidth) / 255);

			if (s2 == 0)
			{
				Rect	rc;

				rc.x = x;
				rc.y = y;
				rc.width = 1;
				rc.height = 1;
				SetRectByteImage(imgDst, rc, pWhiteRectImage);
			}
		}

		offset = offset + side - 1;
	}

	delete []pWhiteRectImage;
}

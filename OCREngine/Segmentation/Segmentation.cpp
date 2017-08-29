#include "stdafx.h"
#include "Segmentation.h"
#include "NoiseReduction.h"
#include "InclCorr.h"
#include "opencv2/highgui/highgui.hpp"


#define FEATUREDIMENSION	196


typedef struct
{
	BOOL bOpenTag;
	int nTagIndex;
} USERTAG_INFO_T, *USERTAG_INFO_P;

typedef vector<USERTAG_INFO_T>	VECTOR_USER_TAG;

typedef struct 
{
	int				nMultiFrameIndex;		// 0:전체프레임상태이다.

	DWORD			dwSequence;
	unsigned short	nCharFlag;				// 0:군집에서 미사용시, 1:기본문자
	DWORD			nCharType;				// 0:not include, 1:방점포함, 2:간점포함
	unsigned short	nCharCode;				// 문자코드
	DWORD			dwSpaceWordCount;			// 세그먼트 이전에 스페이스 갯수
	Rect			rChar;
	BOOL			bWrongIndex;	

	// 유사도 처리를 위한 특징정보
	DWORD			dwTotalBlackPixel;		// 검은점의 갯수
	DWORD			dwStrokeCount;			// 독립적인 획의 갯수
	DWORD			dwEndPointCount;		// 양끝점의 갯수
	DWORD			dwCrossCount;			// 교차점의 갯수
	DWORD			dwCrossStroke3;			// 교차점의 획수가 3인것
	DWORD			dwCrossStroke4;			// 교차점의 획수가 4인것
	DWORD			dwCrossStroke5;			// 교차점의 획수가 5인것
	DWORD			dwCrossStroke6;			// 교차점의 획수가 6인것
	DWORD			dwCrossStroke7;			// 교차점의 획수가 7이상 인것
	DWORD			dwMaxCrossStroke;
	
	// 특징점추출
	DWORD			dwStroke1_top;				// 가로획 갯수
	DWORD			dwStroke1_middle;			// 가로획 갯수
	DWORD			dwStroke1_bottom;			// 가로획 갯수

	DWORD			dwStroke3_left;				// 세로획 갯수
	DWORD			dwStroke3_middle;			// 세로획 갯수
	DWORD			dwStroke3_right;			// 세로획 갯수

	DWORD			dwStroke1_total;			// 가로획 갯수
	DWORD			dwStroke2;					// 역사선 갯수
	DWORD			dwStroke3_total;			// 세로획 갯수
	DWORD			dwStroke4;					// 사선 갯수

	VECTOR_USER_TAG	vectorUserTag;

	// 아래 변수는 Serialize 는 하지 않는다.
	BOOL			bShow;
	DWORD			dwOldSequence;
	DWORD			dwVirtualSequence;
} SEGMENT_CHAR_INFO, *LPSEGMENT_CHAR_INFO;

typedef vector<SEGMENT_CHAR_INFO>	RESULT_SEGMENT_GROUP;
typedef RESULT_SEGMENT_GROUP		RES_SEG_GROUP;
typedef SEGMENT_CHAR_INFO			SEG_CHAR_INFO;

typedef vector<Rect>						SEGMENT_GROUP;
typedef vector< std::pair<Rect, SEGMENT_GROUP> >	COLUMN_SEGMENT_GROUP;
typedef vector<Rect>						COLUMN_GROUP;
typedef SEGMENT_GROUP						SEG_GROUP;
typedef COLUMN_GROUP						COL_GROUP;
typedef COLUMN_SEGMENT_GROUP				COL_SEG_GROUP;


const int	nb = 4;


BOOL TracePixelSegment(const Mat &image, int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, BYTE byPixelColor)
{
	// 련결점단위로 세그먼트화 진행(HJH)
	BYTE	*pBits, *pTmp;
	BYTE	byInvColor = 255 - byPixelColor,
			color;
	int		nWidth = image.cols,
			nHeight = image.rows;
	long	offset,
			size = nWidth * nHeight;
	int		x, y,
			Xmax, Xmin, Ymax, Ymin;

	pBits = new BYTE[size];
	pTmp = new BYTE[size];
	offset = 0;
	for (y = 0; y < nHeight; y++)
	{
		image.data[offset] = byPixelColor;
		image.data[offset+1] = byInvColor;
		offset = offset + nWidth - 1;
		image.data[offset-1] = byInvColor;
		image.data[offset] = byPixelColor;
		offset++;
	}

	memset(image.data, byPixelColor, nWidth);
	memset(image.data+nWidth+1, byInvColor, nWidth-2);
	memset(image.data+nWidth*(nHeight-2)+1, byInvColor, nWidth-2);
	memset(image.data+nWidth*(nHeight-1), byPixelColor, nWidth);

	memcpy(pBits, image.data, size);
	memcpy(pTmp, image.data, size);

	const int	k1 = image.cols * image.rows / (bHandWrite ? 190000 : 450000);
	long	*queue, *tmp;
	POINT	*pt_queue, *pt_tmp;
	int		s, n, _n,
			i;
	long	MinOffset, MaxOffset;
	Rect	rect;

	offset = nWidth + 1;
	for (y = 1; y < nHeight-1; y++)
	{
		for (x = 1; x < nWidth-1; x++)
		{
			s = 0;
			color = pTmp[offset];
			if (color == byPixelColor)
			{
				pBits[offset] = byInvColor;
				queue = new long[1];
				queue[0] = offset;
				pt_queue = new POINT[1];
				pt_queue[0].x = x, pt_queue[0].y = y;
				n = 1;
				MinOffset = MaxOffset = offset;
				Xmax = Xmin = x, Ymax = Ymin = y;
				while (n)
				{
					s += n;
					tmp = new long[n*nb];
					pt_tmp = new POINT[n*nb];
					_n = 0;
					for (i=0; i<n; i++)
					{
						if (MinOffset > queue[i])
							MinOffset = queue[i];
						if (MaxOffset < queue[i])
							MaxOffset = queue[i];

						if (Xmin > pt_queue[i].x)
							Xmin = pt_queue[i].x;
						if (Xmax < pt_queue[i].x)
							Xmax = pt_queue[i].x;
						if (Ymin > pt_queue[i].y)
							Ymin = pt_queue[i].y;
						if (Ymax < pt_queue[i].y)
							Ymax = pt_queue[i].y;

						if (pBits[queue[i]+1] == color)
						{
							pBits[queue[i]+1] = pTmp[queue[i]+1] = byInvColor;
							tmp[_n] = queue[i] + 1;
							pt_tmp[_n].x = pt_queue[i].x + 1;
							pt_tmp[_n++].y = pt_queue[i].y;
						}
						if (pBits[queue[i]+nWidth] == color)
						{
							pBits[queue[i]+nWidth] = pTmp[queue[i]+nWidth] = byInvColor;
							tmp[_n] = queue[i] + nWidth;
							pt_tmp[_n].x = pt_queue[i].x;
							pt_tmp[_n++].y = pt_queue[i].y + 1;
						}
						if (pBits[queue[i]-1] == color)
						{
							pBits[queue[i]-1] = pTmp[queue[i]-1] = byInvColor;
							tmp[_n] = queue[i] - 1;
							pt_tmp[_n].x = pt_queue[i].x - 1;
							pt_tmp[_n++].y = pt_queue[i].y;
						}
						if (pBits[queue[i]-nWidth] == color)
						{
							pBits[queue[i]-nWidth] = pTmp[queue[i]-nWidth] = byInvColor;
							tmp[_n] = queue[i] - nWidth;
							pt_tmp[_n].x = pt_queue[i].x;
							pt_tmp[_n++].y = pt_queue[i].y - 1;
						}

						if (nb == 4)
							continue;

						if (pBits[queue[i]+nWidth+1] == color)
						{
							pBits[queue[i]+nWidth+1] = pTmp[queue[i]+nWidth+1] = byInvColor;
							tmp[_n] = queue[i] + nWidth + 1;
							pt_tmp[_n].x = pt_queue[i].x + 1;
							pt_tmp[_n++].y = pt_queue[i].y + 1;
						}
						if (pBits[queue[i]+nWidth-1] == color)
						{
							pBits[queue[i]+nWidth-1] = pTmp[queue[i]+nWidth-1] = byInvColor;
							tmp[_n] = queue[i] + nWidth - 1;
							pt_tmp[_n].x = pt_queue[i].x - 1;
							pt_tmp[_n++].y = pt_queue[i].y + 1;
						}
						if (pBits[queue[i]-nWidth+1] == color)
						{
							pBits[queue[i]-nWidth+1] = pTmp[queue[i]-nWidth+1] = byInvColor;
							tmp[_n] = queue[i] - nWidth + 1;
							pt_tmp[_n].x = pt_queue[i].x + 1;
							pt_tmp[_n++].y = pt_queue[i].y - 1;
						}
						if (pBits[queue[i]-nWidth-1] == color)
						{
							pBits[queue[i]-nWidth-1] = pTmp[queue[i]-nWidth-1] = byInvColor;
							tmp[_n] = queue[i] - nWidth - 1;
							pt_tmp[_n].x = pt_queue[i].x - 1;
							pt_tmp[_n++].y = pt_queue[i].y - 1;
						}
					}
					delete []queue;
					delete []pt_queue;
					queue = tmp;
					pt_queue = pt_tmp;
					n = _n;
				}

				rect.x = Xmin;
				rect.y = Ymin;
				rect.width = Xmax - Xmin + 1;
				rect.height = Ymax - Ymin + 1;
				if (s > k1)
				{
					//if ((nDirection == 0 && rect.width < image.cols/10) || 
					//	(nDirection == 1 && rect.height < image.rows/10))
					{
						if (rect.width*20 > rect.height && rect.height*30 > rect.width)
							SegGroup.push_back(rect);
					}
				}

				delete []queue;
				delete []pt_queue;
			}

			offset++;
		}
		offset = offset + 2;
	}

	delete []pBits;
	delete []pTmp;

	return TRUE;
}

void RemoveInvalidSegment1(const Mat &image, BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	int		nSize, i;
	Rect	rect;

	nSize = SegGroup.size();
	for (i = nSize-1; i >= 0; i--)
	{
		rect = SegGroup.at(i);
		if (rect.width*10 > rect.height*(bHandWrite? 80:140))
		{
			SegGroup.erase(SegGroup.begin()+i);
		}
		else if (rect.width >= image.cols/2)
		{
			SegGroup.erase(SegGroup.begin()+i);
		}
	}
}

Rect UnionRect(const Rect &r1, const Rect &r2)
{
	Rect	r;
	int		left, top, right, bottom;

	left   = min(r1.x, r2.x);
	top    = min(r1.y, r2.y);
	right  = max(r1.x+r1.width, r2.x+r2.width);
	bottom = max(r1.y+r1.height, r2.y+r2.height);

	r.x = left;
	r.y = top;
	r.width = right - left;
	r.height = bottom - top;

	return r;
}

BOOL FirstMergeSegment(BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	BOOL	bRet = FALSE;
	int		nSize;
	Rect	rcSegment, rcNextSegment;
	int		nSquare, nNextSquare, nIntersectSquare, nUnionSquare,
			nUnionWidth, nUnionHeight,
			i, j;

	nSize = SegGroup.size();
	for (i = 0; i < nSize-1; i++)
	{
		rcSegment = SegGroup.at(i);
		nSquare = rcSegment.width * rcSegment.height;
		for (j = i+1; j < nSize; j++)
		{
			rcNextSegment = SegGroup.at(j);
			nNextSquare = rcNextSegment.width * rcNextSegment.height;

			nIntersectSquare = min(rcSegment.br().x, rcNextSegment.br().x) - max(rcSegment.x, rcNextSegment.x);
			if (nIntersectSquare > 0)
				nIntersectSquare *= (min(rcSegment.br().y, rcNextSegment.br().y) - max(rcSegment.y, rcNextSegment.y));
			else
				nIntersectSquare = 0;
			if (nIntersectSquare < 0)
				nIntersectSquare = 0;

			nUnionWidth = max(rcSegment.br().x, rcNextSegment.br().x) - min(rcSegment.x, rcNextSegment.x);
			nUnionHeight = max(rcSegment.br().y, rcNextSegment.br().y) - min(rcSegment.y, rcNextSegment.y);
			nUnionSquare = nUnionWidth * nUnionHeight;

			if (bHandWrite)
			{
				if (nIntersectSquare*10/3 >= nSquare ||
					nIntersectSquare*10/3 >= nNextSquare ||
					nSquare > nUnionSquare*95/100 ||
					nNextSquare > nUnionSquare*95/100)
				{
					rcSegment = UnionRect(rcSegment, rcNextSegment);
					SegGroup.at(i) = rcSegment;
					SegGroup.erase(SegGroup.begin()+j);
					nSize--;
					j--;
					bRet = TRUE;
				}
			}
			else
			{
				if (nIntersectSquare*10 >= nSquare ||
					nIntersectSquare*10 >= nNextSquare ||
					nSquare > nUnionSquare*95/100 ||
					nNextSquare > nUnionSquare*95/100)
				{
					rcSegment = UnionRect(rcSegment, rcNextSegment);
					SegGroup.at(i) = rcSegment;
					SegGroup.erase(SegGroup.begin()+j);
					nSize--;
					j--;
					bRet = TRUE;
					continue;
				}

				//if (m_nLang == 0 && m_nDocumentType == 1) // Korean
				//{
				//	int	nDistance;

				//	if (rcSegment.x > rcNextSegment.x && rcSegment.br().x > rcNextSegment.br().x)
				//		continue;
				//	if (rcSegment.x < rcNextSegment.x && rcSegment.br().x < rcNextSegment.br().x)
				//		continue;
				//	if (nUnionHeight > 2*max(rcSegment.height, rcNextSegment.height))
				//		continue;
				//	nDistance = max(rcSegment.y, rcNextSegment.y)-min(rcSegment.br().y, rcNextSegment.br().y);
				//	if (nDistance*5 > max(rcSegment.height, rcNextSegment.height))
				//		continue;

				//	rcSegment.UnionRect(&rcSegment, &rcNextSegment);
				//	SegGroup.at(i) = rcSegment;
				//	SegGroup.erase(SegGroup.begin()+j);
				//	nSize--;
				//	j--;
				//	bRet = TRUE;
				//}
			}
		}
	}

	return bRet;
}

void SortWidthSegment(SEGMENT_GROUP &SegmentGroup)
{
	int				i, j;
	int				nSize, nSortSize, nInsert;
	BOOL			bFind;
	Rect			r, rSort;
	SEGMENT_GROUP	SortSegment;

	nSize = SegmentGroup.size();
	for (i = 0; i < nSize; i++)
	{
		r = SegmentGroup.at(i);

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortSegment.size();
		for (j = 0; j < nSortSize; j++)
		{
			rSort = SortSegment.at(j);

			if (r.width < rSort.width)
			{
				nInsert = j;
				bFind = TRUE;

				break;
			}
		}

		if (bFind)
		{
			SortSegment.insert(SortSegment.begin()+nInsert, SegmentGroup.at(i));
		}
		else
		{
			SortSegment.push_back(SegmentGroup.at(i));
		}
	}

	SegmentGroup.clear();
	for (i = 0; i < nSize; i++)
	{
		SegmentGroup.push_back(SortSegment.at(i));
	}
}

void SortHeightSegment(SEGMENT_GROUP &SegmentGroup)
{
	int				i, j;
	int				nSize, nSortSize, nInsert;
	int				nSquar, nSortSquar;
	BOOL			bFind;
	Rect			r, rSort;
	SEGMENT_GROUP	SortSegment;

	nSize = SegmentGroup.size();
	for (i = 0; i < nSize; i++)
	{
		r = SegmentGroup.at(i);
		nSquar = r.height;

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortSegment.size();
		for (j = 0; j < nSortSize; j++)
		{
			rSort = SortSegment.at(j);
			nSortSquar=rSort.height;

			if (nSquar>nSortSquar)
			{
				nInsert = j;
				bFind = TRUE;

				break;
			}
		}

		if (bFind)
		{
			SortSegment.insert(SortSegment.begin()+nInsert, SegmentGroup.at(i));
		}
		else
		{
			SortSegment.push_back(SegmentGroup.at(i));
		}
	}

	SegmentGroup.clear();
	for (i = 0; i < nSize; i++)
	{
		SegmentGroup.push_back(SortSegment.at(i));
	}
}

Size CalcCharSize(BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	SEG_GROUP	TempGroup;
	Size		sizeChar;
	int			nSize,
				width, height,
				nCurrWidth = 0, nCurrHeight = 0,
				nCurrCountW = 0, nCurrCountH = 0,
				nMaxNumW = 0, nMaxCountW = 0,
				nMaxNumH = 0, nMaxCountH = 0,
				i;

	if (bHandWrite)
	{
		Rect	rcSegment;

		nSize = SegGroup.size();
		for (i = 0; i < nSize; i++)
		{
			rcSegment = SegGroup.at(i);
			if (rcSegment.width*100 > rcSegment.height*92 && rcSegment.width*100 < rcSegment.height*152)
			{
				TempGroup.push_back(rcSegment);
			}
		}
	}
	else
	{
		Rect	rcSegment;

		nSize = SegGroup.size();
		for (i = 0; i < nSize; i++)
		{
			rcSegment = SegGroup.at(i);
			if (rcSegment.width*100 > rcSegment.height*40 && rcSegment.width*100 < rcSegment.height*120)
			{
				TempGroup.push_back(rcSegment);
			}
		}
	}

	nSize = TempGroup.size();
	SortWidthSegment(TempGroup);

	for (i=(/*m_nLetterStyle==0? nSize-min(nSize, 40):*/nSize*3/4); i<nSize; i++)
	{
		width = TempGroup.at(i).width / 2 * 2;
		if (width != nCurrWidth)
		{
			if (nMaxCountW <= nCurrCountW)
			{
				nMaxCountW = nCurrCountW;
				nMaxNumW = nCurrWidth;
			}
			nCurrWidth = width;
			nCurrCountW = 0;
		}
		nCurrCountW++;
	}
	if (nMaxNumW == 0)
		nMaxNumW = nCurrWidth;

	SortHeightSegment(TempGroup);

	for (i=0; i<(/*m_nLetterStyle==0? min(nSize, 40):*/(nSize+3)/4); i++)
	{
		height = TempGroup.at(i).height / 2 * 2;
		if (height != nCurrHeight)
		{
			if (nMaxCountH <= nCurrCountH)
			{
				nMaxCountH = nCurrCountH;
				nMaxNumH = nCurrHeight;
			}
			nCurrHeight = height;
			nCurrCountH = 0;
		}
		nCurrCountH++;
	}
	if (nMaxNumH == 0)
		nMaxNumH = nCurrHeight;

	sizeChar.width = nMaxNumW;
	if (nMaxNumH >= nMaxNumW*3)
	{
		sizeChar.height = nMaxNumW * 3;
	}
	else
	{
		if (nMaxNumH < nMaxNumW*2/3)
		{
			sizeChar.height = nMaxNumW * 2 / 3;
		}
		else
		{
			sizeChar.height = nMaxNumH;
		}
	}

	return sizeChar;
}

Size CalcCharSize(BOOL bHandWrite, const COL_SEG_GROUP &ColSegGroup)
{
	SEG_GROUP	SegGroup;
	int			nColumnSize;
	int			nHeight = 0;

	nColumnSize = ColSegGroup.size();
	for (int i = 0; i < nColumnSize; i++)
	{
		nHeight += ColSegGroup.at(i).first.height;
	}

	nHeight /= nColumnSize;

	return Size(nHeight*3/5, nHeight);
}

Size CalcCharSize(BOOL bHandWrite, const RES_SEG_GROUP &SegmentList)
{
	SEG_GROUP	SegGroup;
	int			nSize, i;

	nSize = SegmentList.size();
	for (i = 0; i < nSize; i++)
	{
		SegGroup.push_back(SegmentList.at(i).rChar);
	}

	return CalcCharSize(bHandWrite, SegGroup);
}

void SortTopSegment(SEGMENT_GROUP &SegmentGroup)
{
	int				i, j;
	int				nSize, nSortSize, nInsert;
	BOOL			bFind;
	Rect			r, rSort;
	SEGMENT_GROUP	SortSegment;

	nSize = SegmentGroup.size();
	for (i = 0; i < nSize; i++)
	{
		r = SegmentGroup.at(i);

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortSegment.size();
		for (j = 0; j < nSortSize; j++)
		{
			rSort = SortSegment.at(j);

			if (r.y < rSort.y)
			{
				nInsert = j;
				bFind = TRUE;

				break;
			}
		}

		if (bFind)
		{
			SortSegment.insert(SortSegment.begin()+nInsert, SegmentGroup.at(i));
		}
		else
		{
			SortSegment.push_back(SegmentGroup.at(i));
		}
	}

	SegmentGroup.clear();
	for (i = 0; i < nSize; i++)
	{
		SegmentGroup.push_back(SortSegment.at(i));
	}
}

BOOL IntersectRect(const Rect &r1, const Rect &r2, Rect &r)
{
	int	left, top, right, bottom;

	left   = max(r1.x, r2.x);
	top    = max(r1.y, r2.y);
	right  = min(r1.x+r1.width, r2.x+r2.width);
	bottom = min(r1.y+r1.height, r2.y+r2.height);

	if (left > right || top > bottom)
	{
		return FALSE;
	}

	r.x = left;
	r.y = top;
	r.width = right - left;
	r.height = bottom - top;

	return TRUE;
}

void PICSortLeftSegment(SEG_GROUP &SegGroup)
{
	SEG_GROUP	SortGroup;
	Rect		rcSegment, rcSort;
	long		center;
	int			nSize, nSortSize, nInsert,
				i, j;
	BOOL		bFind;

	nSize = SegGroup.size();
	for (i = 0; i < nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		center = rcSegment.x + rcSegment.width / 2;

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortGroup.size();
		for (j=0; j<nSortSize; j++)
		{
			rcSort = SortGroup.at(j);

			if (center < rcSort.x+rcSort.width/2)
			{
				nInsert = j;
				bFind = TRUE;
				break;
			}
		}

		if (bFind)
		{
			SortGroup.insert(SortGroup.begin()+nInsert, SegGroup.at(i));
		}
		else
		{
			SortGroup.push_back(SegGroup.at(i));
		}
	}

	SegGroup.clear();
	for (i = 0; i < nSize; i++)
	{
		SegGroup.push_back(SortGroup.at(i));
	}
}

void PICSortLeftSegment(RES_SEG_GROUP &SegmentList)
{
	RES_SEG_GROUP	SortList;
	Rect			rcSegment, rcSort;
	long			center;
	int				nSize, nSortSize, nInsert,
					i, j;
	BOOL			bFind;

	nSize = SegmentList.size();
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegmentList.at(i).rChar;
		center = (rcSegment.x + rcSegment.br().x) / 2;

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortList.size();
		for (j=0; j<nSortSize; j++)
		{
			rcSort = SortList.at(j).rChar;

			if (center < (rcSort.x + rcSort.br().x)/2)
			{
				nInsert = j;
				bFind = TRUE;
				break;
			}
		}

		if (bFind)
			SortList.insert(SortList.begin()+nInsert, SegmentList.at(i));
		else
			SortList.push_back(SegmentList.at(i));
	}

	SegmentList.clear();
	for (i=0; i<nSize; i++)
	{
		SegmentList.push_back(SortList.at(i));
	}
}

void PICSortTopSegment(SEG_GROUP &SegGroup)
{
	SEG_GROUP	SortGroup;
	Rect		rcSegment, rcSort;
	long		center;
	int			nSize, nSortSize, nInsert,
				i, j;
	BOOL		bFind;

	nSize = SegGroup.size();
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		center = (rcSegment.y + rcSegment.br().y) / 2;

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortGroup.size();
		for (j=0; j<nSortSize; j++)
		{
			rcSort = SortGroup.at(j);

			if (center < (rcSort.y+rcSort.br().y)/2)
			{
				nInsert = j;
				bFind = TRUE;
				break;
			}
		}

		if (bFind)
			SortGroup.insert(SortGroup.begin()+nInsert, SegGroup.at(i));
		else
			SortGroup.push_back(SegGroup.at(i));
	}

	SegGroup.clear();
	for (i=0; i<nSize; i++)
		SegGroup.push_back(SortGroup.at(i));
}

void PICSortTopSegment(RES_SEG_GROUP &SegmentList)
{
	RES_SEG_GROUP	SortList;
	Rect			rcSegment, rcSort;
	long			center;
	int				nSize, nSortSize, nInsert,
					i, j;
	BOOL			bFind;

	nSize = SegmentList.size();
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegmentList.at(i).rChar;
		center = (rcSegment.y + rcSegment.br().y) / 2;

		bFind = FALSE;
		nInsert = 0;
		nSortSize = SortList.size();
		for (j=0; j<nSortSize; j++)
		{
			rcSort = SortList.at(j).rChar;

			if (center < (rcSort.y+rcSort.br().y)/2)
			{
				nInsert=j;
				bFind=TRUE;
				break;
			}
		}

		if (bFind)
			SortList.insert(SortList.begin()+nInsert, SegmentList.at(i));
		else
			SortList.push_back(SegmentList.at(i));
	}

	SegmentList.clear();
	for (i=0; i<nSize; i++)
	{
		SegmentList.push_back(SortList.at(i));
	}
}

void FixSegmentSize(const Mat &image, Rect &rcSegment)
{
	BOOL	bPixelExist;
	int		x, y,
			width = rcSegment.width, height = rcSegment.height,
			offset;

	if (rcSegment.x >= rcSegment.br().x || rcSegment.y >= rcSegment.br().y)
		return;

	bPixelExist = FALSE;
	for (x = 0; x < width; x++)
	{
		offset = image.cols * rcSegment.y + rcSegment.x + x;
		for (y = 0; y < height; y++)
		{
			if (image.data[offset] == 0)
			{
				bPixelExist = TRUE;
				break;
			}

			offset += image.cols;
		}

		if (bPixelExist == TRUE)
			break;
	}

	if (x != width)
	{
		rcSegment.width -= x;
		width = rcSegment.width;
		rcSegment.x += x;
	}

	bPixelExist = FALSE;
	for (x = width-1; x >= 0; x--)
	{
		offset = image.cols * rcSegment.y + rcSegment.x + x;
		for (y = 0; y < height; y++)
		{
			if (image.data[offset] == 0)
			{
				bPixelExist = TRUE;
				break;
			}

			offset += image.cols;
		}

		if (bPixelExist == TRUE)
			break;
	}

	if (x != -1)
	{
		rcSegment.width = x + 1;
		width = rcSegment.width;
	}

	bPixelExist = FALSE;
	for (y = 0; y < height; y++)
	{
		offset = image.cols * (rcSegment.y + y) + rcSegment.x;
		for (x = 0; x < width; x++)
		{
			if (image.data[offset] == 0)
			{
				bPixelExist = TRUE;
				break;
			}

			offset++;
		}

		if (bPixelExist == TRUE)
			break;
	}

	if (y != height)
	{
		rcSegment.y += y;
		rcSegment.height -= y;
		height = rcSegment.height;
	}

	bPixelExist = FALSE;
	for (y = height-1; y >= 0; y--)
	{
		offset = image.cols * (rcSegment.y + y) + rcSegment.x;
		for (x = 0; x < width; x++)
		{
			if (image.data[offset] == 0)
			{
				bPixelExist = TRUE;
				break;
			}

			offset++;
		}

		if (bPixelExist == TRUE)
			break;
	}

	if (y != -1)
	{
		rcSegment.height = y + 1;
		//height = rcSegment.height;
	}
}

#if 0
BOOL CalcVertColGroupByHistogram(const Mat &image, int nDirection, COL_GROUP &ColGroup, SEG_GROUP &ArticleSymbolGroup)
{
	SEG_GROUP	SegGroup;
	Rect		rcSegment, rcNextSegment;
	int			nOverlap,
				nSize, i, j;

	TracePixelSegment(image, nDirection, SegGroup, 0);

	nSize = SegGroup.size();
	for (i = 0; i < nSize; i++)
	{
		ColGroup.push_back(SegGroup.at(i));
	}

	SortTopSegment(ColGroup);

	//련결성분들을 수직방향으로 합친다.
	nSize = ColGroup.size();
	for (i=0; i<nSize-1; i++)
	{
		rcSegment = ColGroup.at(i);
		for (j=i+1; j<nSize; j++)
		{
			rcNextSegment = ColGroup.at(j);
			if ((rcSegment.x >= rcNextSegment.x && rcSegment.x < rcNextSegment.x+rcNextSegment.width) ||
				(rcNextSegment.x >= rcSegment.x && rcNextSegment.x < rcSegment.x+rcSegment.width))
			{
				if (rcSegment.x >= rcNextSegment.x)
				{
					nOverlap = rcNextSegment.x + rcNextSegment.width - rcSegment.x;
				}
				else
				{
					nOverlap = rcSegment.x + rcSegment.width - rcNextSegment.x;
				}

				if ((float)nOverlap > (float)rcSegment.width*0.5 || (float)nOverlap > (float)rcNextSegment.width*0.5)
				{
					if (rcSegment.x > rcNextSegment.x)
					{
						rcSegment.x = rcNextSegment.x;
					}

					if (rcSegment.y > rcNextSegment.y)
					{
						rcSegment.y = rcNextSegment.y;
					}

					if (rcSegment.x+rcSegment.width < rcNextSegment.x+rcNextSegment.width)
					{
						rcSegment.width = rcNextSegment.x + rcNextSegment.width - rcSegment.x;
					}

					if (rcSegment.y+rcSegment.height < rcNextSegment.y+rcNextSegment.height)
					{
						rcSegment.height = rcNextSegment.y + rcNextSegment.height - rcSegment.y;
					}

					ColGroup.at(i) = rcSegment;
					ColGroup.erase(ColGroup.begin()+j);
					nSize--;
					j--;
				}
			}
		}
	}

	Rect	rcTmp;
	float	ratio, ratio1;

	nSize = ColGroup.size();
	for (i = 0; i < nSize-1; i++)
	{
		rcSegment = ColGroup.at(i);
		if (rcSegment.height < 50/2)
		{
			ColGroup.erase(ColGroup.begin()+i);
			nSize--;
			i--;
			if (i == -1)
			{
				continue;
			}
		}

		rcSegment = ColGroup.at(i);
		
		for (j = i+1; j < nSize; j++)
		{
			rcNextSegment = ColGroup.at(j);
			if (IntersectRect(rcSegment, rcNextSegment, rcTmp))
			{
				ratio = ((float)rcTmp.width * rcTmp.height) / (rcSegment.width * rcSegment.height);
				ratio1 = ((float)rcTmp.width * rcTmp.height) / (rcNextSegment.width * rcNextSegment.height);
				if (ratio > 0.5 || ratio1 > 0.5)
				{
					if (rcSegment.x > rcNextSegment.x)
					{
						rcSegment.x = rcNextSegment.x;
					}

					if (rcSegment.y > rcNextSegment.y)
					{
						rcSegment.y = rcNextSegment.y;
					}

					if (rcSegment.x+rcSegment.width < rcNextSegment.x+rcNextSegment.width)
					{
						rcSegment.width = rcNextSegment.x + rcNextSegment.width - rcSegment.x;
					}

					if (rcSegment.y+rcSegment.height < rcNextSegment.y+rcNextSegment.height)
					{
						rcSegment.height = rcNextSegment.y + rcNextSegment.height - rcSegment.y;
					}

					ColGroup.at(i) = rcSegment;
					ColGroup.erase(ColGroup.begin()+j);
					nSize--;
					j--;
				}
			}
		}
	}

	int		x, y,
			offset,
			nHistogram, nMinHis, nMinHisX;

	PICSortLeftSegment(ColGroup);
	nSize = ColGroup.size();
	for (i = 1; i < nSize; i++)
	{
		rcSegment.x = min(ColGroup.at(i-1).x+ColGroup.at(i-1).width, ColGroup.at(i).x);
		rcSegment.width = max(ColGroup.at(i-1).x+ColGroup.at(i-1).width, ColGroup.at(i).x) - rcSegment.x;
		rcSegment.y = 0;
		rcSegment.height = image.rows;

		if (rcSegment.x+3 > rcSegment.x+rcSegment.width)
		{
			continue;
		}

		nMinHis = INT_MAX;
		for (x = 1; x < rcSegment.width-1; x++)
		{
			offset = image.cols * rcSegment.y + rcSegment.x + x;
			nHistogram = 0;
			for (y = 0; y < rcSegment.height; y++)
			{
				nHistogram += image.data[offset] + image.data[offset-1] + image.data[offset+1];
				offset += image.cols;
			}

			if (nMinHis > nHistogram)
				nMinHis = nHistogram, nMinHisX = x;
		}

		ColGroup.at(i).x = rcSegment.x + nMinHisX;
		ColGroup.at(i-1).width = ColGroup.at(i).x - ColGroup.at(i-1).x;
		FixSegmentSize(image, ColGroup.at(i-1));
		FixSegmentSize(image, ColGroup.at(i));
	}

	nSize = ColGroup.size();
	for (i = 1; i < nSize-1; i++)
	{
		rcSegment = ColGroup.at(i);
		if (rcSegment.width > 20)
			continue;

		ColGroup.erase(ColGroup.begin()+i);
		nSize--;
		i--;

		if (rcSegment.x-ColGroup.at(i).br().x != ColGroup.at(i+1).x-rcSegment.br().x)
		{
			if (rcSegment.x-ColGroup.at(i).br().x < ColGroup.at(i+1).x-rcSegment.br().x)
				ColGroup.at(i) = UnionRect(ColGroup.at(i), rcSegment);
			else
				ColGroup.at(i+1) = UnionRect(ColGroup.at(i+1), rcSegment);

			continue;
		}

		rcSegment.y = 0;
		rcSegment.height = image.rows;

		if (rcSegment.x+3 > rcSegment.br().x)
		{
			continue;
		}

		nMinHis = INT_MAX;
		for (x = 1; x < rcSegment.width-1; x++)
		{
			offset = image.cols * rcSegment.y + rcSegment.x + x;
			nHistogram = 0;
			for (y = 0; y < rcSegment.height; y++)
			{
				nHistogram += image.data[offset] + image.data[offset-1] + image.data[offset+1];
				offset += rcSegment.width;
			}

			if (nMinHis > nHistogram)
				nMinHis = nHistogram, nMinHisX = x;
		}

		ColGroup.at(i+1).x = rcSegment.x + nMinHisX;
		ColGroup.at(i).width = ColGroup.at(i+1).x - ColGroup.at(i).x;
		FixSegmentSize(image, ColGroup.at(i));
		FixSegmentSize(image, ColGroup.at(i+1));
	}

	if (ColGroup.size() > 0)
	{
		if (ColGroup.at(0).width < 20)
		{
			if (ColGroup.size() > 1)
				ColGroup.at(1) = UnionRect(ColGroup.at(0), ColGroup.at(1));
			ColGroup.erase(ColGroup.begin());
		}
	}
	if (ColGroup.size() > 0)
	{
		if (ColGroup.at(ColGroup.size()-1).width < 20)
		{
			if (ColGroup.size() > 1)
				ColGroup.at(ColGroup.size()-2) = UnionRect(ColGroup.at(ColGroup.size()-2), ColGroup.at(ColGroup.size()-1));
			ColGroup.erase(ColGroup.begin()+ColGroup.size()-1);
		}
	}

	return TRUE;
}

BOOL CalcColGroupByHistogram(const Mat &image, int nDirection, COL_GROUP &ColGroup, SEG_GROUP &ArticleSymbolGroup)
{
	if (nDirection == 0)	// Vertical document
	{
		if (!CalcVertColGroupByHistogram(image, nDirection, ColGroup, ArticleSymbolGroup))
		{
			return FALSE;
		}
	}
	else	// Horizontal document
	{
		if (!CalcHorzColGroupByHistogram(ColGroup, ArticleSymbolGroup))
		{
			return FALSE;
		}
	}

	return TRUE;
}
#endif

void SetAutoVertSequence(SEG_GROUP &SegGroup, Size sizeChar)
{
	SEG_GROUP	SortGroup, TempGroup;
	Rect		rcSegment;
	int			center, prev_center,
				nSize, nTempSize,
				i, j;

	PICSortLeftSegment(SegGroup);

	nSize = SegGroup.size();
	if (nSize == 0)
		return;

	center = (SegGroup.at(nSize-1).x + SegGroup.at(nSize-1).br().x) / 2;
	prev_center = center;
	for (i=nSize-1; i>=0; i--)
	{
		rcSegment = SegGroup.at(i);
		if (center-(rcSegment.x+rcSegment.br().x)/2 >= sizeChar.width
			|| prev_center-(rcSegment.x+rcSegment.br().x)/2 > sizeChar.width/2)
		{
			PICSortTopSegment(TempGroup);

			nTempSize = TempGroup.size();
			for (j=0; j<nTempSize; j++)
				SortGroup.push_back(TempGroup.at(j));
			TempGroup.clear();

			center = (rcSegment.x + rcSegment.br().x) / 2;
		}

		TempGroup.push_back(rcSegment);
		prev_center = (rcSegment.x + rcSegment.br().x) / 2;
	}

	PICSortTopSegment(TempGroup);

	nTempSize = TempGroup.size();
	for (j=0; j<nTempSize; j++)
		SortGroup.push_back(TempGroup.at(j));

	SegGroup.clear();
	for (i=0; i<nSize; i++)
		SegGroup.push_back(SortGroup.at(i));
}

void SetAutoVertSequence(RES_SEG_GROUP &SegmentList, Size sizeChar)
{
	RES_SEG_GROUP	SortList, TempList;
	Rect			rcSegment;
	int				center, prev_center,
					nSize, nTempSize,
					i, j;

	PICSortLeftSegment(SegmentList);

	nSize = SegmentList.size();
	if (nSize == 0)
		return;

	center = (SegmentList.at(nSize-1).rChar.x + SegmentList.at(nSize-1).rChar.br().x) / 2;
	prev_center = center;
	for (i=nSize-1; i>=0; i--)
	{
		rcSegment = SegmentList.at(i).rChar;
		if (center-(rcSegment.x+rcSegment.br().x)/2 >= sizeChar.width*10/10
			|| prev_center-(rcSegment.x+rcSegment.br().x)/2 > sizeChar.width/2)
		{
			PICSortTopSegment(TempList);

			nTempSize = TempList.size();
			for (j=0; j<nTempSize; j++)
				SortList.push_back(TempList.at(j));
			TempList.clear();

			center = (rcSegment.x + rcSegment.br().x) / 2;
		}

		TempList.push_back(SegmentList.at(i));
		prev_center = (rcSegment.x + rcSegment.br().x) / 2;
	}

	PICSortTopSegment(TempList);

	nTempSize = TempList.size();
	for (j=0; j<nTempSize; j++)
		SortList.push_back(TempList.at(j));

	SegmentList.clear();
	for (i=0; i<nSize; i++)
		SegmentList.push_back(SortList.at(i));
}

void SetAutoHorzSequence(SEG_GROUP &SegGroup, Size sizeChar)
{
	SEG_GROUP	SortGroup, TempGroup;
	Rect		rcSegment;
	int			center, prev_center,
				nSize, nTempSize,
				i, j;

	PICSortTopSegment(SegGroup);

	nSize = SegGroup.size();
	if (nSize == 0)
		return;

	center = (SegGroup.at(0).y + SegGroup.at(0).br().y) / 2;
	prev_center = center;
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		if ((rcSegment.y+rcSegment.br().y)/2-center >= sizeChar.height
			|| (rcSegment.y+rcSegment.br().y)/2-prev_center > sizeChar.height/2)
		{
			PICSortLeftSegment(TempGroup);

			nTempSize = TempGroup.size();
			for (j=0; j<nTempSize; j++)
				SortGroup.push_back(TempGroup.at(j));
			TempGroup.clear();

			center = (rcSegment.y + rcSegment.br().y) / 2;
		}

		TempGroup.push_back(rcSegment);
		prev_center = (rcSegment.y + rcSegment.br().y) / 2;
	}

	PICSortLeftSegment(TempGroup);

	nTempSize = TempGroup.size();
	for (j=0; j<nTempSize; j++)
		SortGroup.push_back(TempGroup.at(j));

	SegGroup.clear();
	for (i=0; i<nSize; i++)
		SegGroup.push_back(SortGroup.at(i));
}

void SetAutoHorzSequence(RES_SEG_GROUP &SegmentList, Size sizeChar)
{
	RES_SEG_GROUP	SortList, TempList;
	Rect			rcSegment;
	int				center, prev_center,
					nSize, nTempSize,
					i, j;

	PICSortTopSegment(SegmentList);

	nSize = SegmentList.size();
	if (nSize == 0)
		return;

	center = (SegmentList.at(0).rChar.y + SegmentList.at(0).rChar.br().y) / 2;
	prev_center = center;
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegmentList.at(i).rChar;
		if ((rcSegment.y+rcSegment.br().y)/2-center >= sizeChar.height
			|| (rcSegment.y+rcSegment.br().y)/2-prev_center > sizeChar.height/2)
		{
			PICSortLeftSegment(TempList);

			nTempSize = TempList.size();
			for (j=0; j<nTempSize; j++)
				SortList.push_back(TempList.at(j));
			TempList.clear();

			center = (rcSegment.y + rcSegment.br().y) / 2;
		}

		TempList.push_back(SegmentList.at(i));
		prev_center = (rcSegment.y + rcSegment.br().y) / 2;
	}

	PICSortLeftSegment(TempList);

	nTempSize = TempList.size();
	for (j=0; j<nTempSize; j++)
		SortList.push_back(TempList.at(j));

	SegmentList.clear();
	for (i=0; i<nSize; i++)
		SegmentList.push_back(SortList.at(i));
}

void SetAutoSequence(int nDirection, SEG_GROUP &SegGroup, Size sizeChar)
{
	if (nDirection == 0) // vertical
	{
		SetAutoVertSequence(SegGroup, sizeChar);
	}
	else
	{
		SetAutoHorzSequence(SegGroup, sizeChar);
	}
}

void SetAutoSequence(int nDirection, RES_SEG_GROUP &SegmentList, Size sizeChar)
{
	if (nDirection == 0)
		SetAutoVertSequence(SegmentList, sizeChar);
	else
		SetAutoHorzSequence(SegmentList, sizeChar);

	int	nSize = SegmentList.size();

	for (int i = 0; i < nSize; i++)
		SegmentList.at(i).dwSequence = i + 1;
}

int GetSeparationX(const Mat &image, const Rect &rect, BOOL bWide)
{
	const int	m = 1;

	Mat		imgSegment;
	int		width, height,
			x, y, dx,
			XMinHisL, XMinHisR,
			offset,
			nHistogram,
			nMinHisL = 255, nMinHisR = 255,
			nMaxHisL, nMaxHisR = 0, nMaxHisLTmp = 0;

	if (rect.width == 0 || rect.height == 0)
		return rect.br().x;

	image(rect).copyTo(imgSegment);
	if (imgSegment.empty())
	{
		return rect.br().x;
	}

	width = rect.width;
	height = rect.height;
	for (x = m; x < width-m; x++)
	{
		offset = x - m;
		nHistogram = 0;

		for (y = 0; y < height; y++)
		{
			for (dx = -m; dx <= m; dx++)
			{
				if (imgSegment.data[offset] == 0) // 검은 화소이면
					nHistogram++;
				offset++;
			}

			offset = offset + width - 2 * m - 1;
		}

		nHistogram = nHistogram * 255 / (height * (2 * m + 1));

		if (x < width*3/8)
		{
			if (nMinHisL >= nHistogram)
				nMinHisL = nHistogram, XMinHisL = x, nMaxHisL = nMaxHisLTmp;
			if (nMaxHisLTmp < nHistogram)
				nMaxHisLTmp = nHistogram;
		}

		if (x > width*5/8)
		{
			if (nMinHisR > nHistogram)
				nMinHisR = nHistogram, XMinHisR = x, nMaxHisR = nMinHisR;
			if (nMaxHisR < nHistogram)
				nMaxHisR = nHistogram;
		}
	}

	int	nMinHisLevel = bWide ? 45 : 10,
		nMaxHisLevel = bWide ? 100 : 80;

	if (nMinHisL > nMinHisLevel && nMinHisR > nMinHisLevel)
		return rect.br().x;

	if (nMinHisL == nMinHisR)
	{
		// 톡 셨속탔 틥 될 걱좝뺏 삣캥가 툭딩.(HJH)
		return -(rect.x+XMinHisL);
	}

	if (nMinHisL < nMinHisR && nMinHisL <= nMinHisLevel && nMaxHisL <= nMaxHisLevel)
		return -(rect.x+XMinHisL);

	if (nMinHisR <= nMinHisLevel && nMaxHisR <= nMaxHisLevel)
		return rect.x+XMinHisR;

	return rect.br().x;
}

int GetSeparationY(const Mat &image, int nDirection, BOOL bHandWrite, const Rect &rect)
{
	const int	m = 1;

	Mat		imgSegment;
	int		width, height,
			x, y, dx,
			YMinHis = rect.height, XMinHis = rect.width,
			offset,
			nHistogram,
			nMinHis = 255;

	if (rect.width == 0 || rect.height == 0)
		return nDirection==0? rect.br().y:rect.br().x;

	image(rect).copyTo(imgSegment);
	if (imgSegment.empty())
	{
		return nDirection==0? rect.br().y:rect.br().x;
	}

	if (nDirection == 0) // 세로문서이면(HJH)
	{
		width = rect.width;
		height = rect.height;
		for (y = m; y < height-m; y++)
		{
			offset = (y - m) * width;
			nHistogram = 0;
			for (x=0; x<(2*m+1)*width; x++)
			{
				if (imgSegment.data[offset] == 0) // 검은 화소이면
					nHistogram++;
				offset++;
			}

			nHistogram = nHistogram * 255 / (width * (2 * m + 1));

			if (y > height/8 && y < height*7/8 && nMinHis > nHistogram)
				nMinHis = nHistogram, YMinHis = y;
		}
	}
	else // 가로문서이면(HJH)
	{
		width = rect.width;
		height = rect.height;
		for (x=m; x<width-m; x++)
		{
			offset = x - m;
			nHistogram = 0;
			for (y=0; y<height; y++)
			{
				for (dx=-m; dx<=m; dx++)
				{
					if (imgSegment.data[offset] == 0) // 검은 화소이면
						nHistogram++;
					offset++;
				}
				offset = offset + width - 2 * m - 1;
			}

			nHistogram = nHistogram * height * 255 / ((height * (2 * m + 1)) * width);

			//if (x > width/8 && x < width*7/8 && nMinHis > nHistogram && nHistogram < 75)
			if (x > height*2/5 && x < height*3/5 && nMinHis > nHistogram && nHistogram < 75)
				nMinHis = nHistogram, XMinHis = x;
		}
	}

	if (nDirection == 0 && (!bHandWrite && nMinHis > 20))
	{
		YMinHis = rect.height;
	}

	return nDirection == 0 ? rect.y+YMinHis : rect.x+XMinHis;
}

BOOL CalcVertColGroup(BOOL bHandWrite, SEG_GROUP &SegGroup, COL_GROUP &ColGroup)
{
	// 세그먼트의 기준 너비 및 높이 얻기(HJH)
	Size	sizeChar = CalcCharSize(bHandWrite, SegGroup);

	sizeChar.width = sizeChar.width * 120 / 100;
	sizeChar.height = sizeChar.height * 133 / 100;

	// 행정보의 초기구성(HJH)
	Rect	rcSegment, rcColumn;
	int		nSize, nColumnSize,
			nUnionWidth, nDistance, nIntersectWidth,
			i;

	PICSortLeftSegment(SegGroup);

	nSize = SegGroup.size();
	for (i = 0; i < nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		if (rcSegment.width > sizeChar.width || rcSegment.width < sizeChar.width/3)
			continue;

		nColumnSize = ColGroup.size();
		if (nColumnSize == 0)
			ColGroup.push_back(rcSegment);
		else
		{
			rcColumn = ColGroup.at(nColumnSize-1);

			if (bHandWrite)
			{
				nDistance = max(rcSegment.y, rcColumn.y) - min(rcSegment.br().y, rcColumn.br().y);
				nUnionWidth = max(rcSegment.br().x, rcColumn.br().x) - min(rcSegment.x, rcColumn.x);
				if (nDistance > sizeChar.height*2 || nUnionWidth > sizeChar.width)
					ColGroup.push_back(rcSegment);
				else
				{
					rcColumn = UnionRect(rcColumn, rcSegment);
					ColGroup.at(nColumnSize-1) = rcColumn;
				}
			}
			else
			{
				nIntersectWidth = min(rcSegment.br().x, rcColumn.br().x) - max(rcSegment.x, rcColumn.x);
				nUnionWidth = max(rcSegment.br().x, rcColumn.br().x) - min(rcSegment.x, rcColumn.x);
				if (nIntersectWidth < sizeChar.width*2/10 && nUnionWidth > sizeChar.width*13/12)
					ColGroup.push_back(rcSegment);
				else
				{
					rcColumn = UnionRect(rcColumn, rcSegment);
					ColGroup.at(nColumnSize-1) = rcColumn;
				}
			}
		}
	}

	// 문자행사이 겹침면적에 기초한 문자행 합치기(HJH)
	Rect	rcNextColumn;
	int		nSquare, nNextSquare, nIntersectSquare,
			j;

	//SetAutoSequence(0, ColGroup, sizeChar);
	PICSortLeftSegment(ColGroup);

	nColumnSize = ColGroup.size();
	for (i=0; i<nColumnSize-1; i++)
	{
		rcColumn = ColGroup.at(i);
		nSquare = rcColumn.width * rcColumn.height;

		for (j=i+1; j<nColumnSize; j++)
		{
			rcNextColumn = ColGroup.at(j);
			nNextSquare = rcNextColumn.width * rcNextColumn.height;

			nIntersectSquare = min(rcColumn.br().x, rcNextColumn.br().x) - max(rcColumn.x, rcNextColumn.x);
			if (nIntersectSquare > 0)
				nIntersectSquare *= (min(rcColumn.br().y, rcNextColumn.br().y) - max(rcColumn.y, rcNextColumn.y))*10/7;
			else
				nIntersectSquare = 0;
			if (nIntersectSquare < 0)
				nIntersectSquare = 0;

			nUnionWidth = max(rcColumn.br().x, rcNextColumn.br().x) - min(rcColumn.x, rcNextColumn.x);

			if (nUnionWidth < sizeChar.width*12/10 && 
				(nSquare <= nIntersectSquare || nNextSquare <= nIntersectSquare))
			{
				rcColumn = UnionRect(rcColumn, rcNextColumn);
				ColGroup.at(i) = rcColumn;
				ColGroup.erase(ColGroup.begin()+j);
				nColumnSize--;
				j--;
			}
		}
	}

	// 우아래로 린접한 행의 너비조건에 의한 합치기(HJH)
	nColumnSize = ColGroup.size();
	PICSortLeftSegment(ColGroup);

	for (i=0; i<nColumnSize-1; i++)
	{
		rcColumn = ColGroup.at(i);

		for (j=i+1; j<nColumnSize; j++)
		{
			rcNextColumn = ColGroup.at(j);

			nDistance = max(rcColumn.y, rcNextColumn.y) - min(rcColumn.br().y, rcNextColumn.br().y);
			nUnionWidth = max(rcColumn.br().x, rcNextColumn.br().x) - min(rcColumn.x, rcNextColumn.x);

			if (rcNextColumn.height > sizeChar.height && 
				nUnionWidth < sizeChar.width*12/10 && 
				rcColumn.width >= rcNextColumn.width/2 && rcColumn.width/2 <= rcNextColumn.width)
			{
				rcColumn = UnionRect(rcColumn, rcNextColumn);
				ColGroup.at(i) = rcColumn;
				ColGroup.erase(ColGroup.begin()+j);
				nColumnSize--;
				j--;
			}
		}
	}

	// 정확치 못한 행세그먼트의 제거(HJH)
	nColumnSize = ColGroup.size();
	for (i=nColumnSize-1; i>=0; i--)
	{
		rcColumn = ColGroup.at(i);

		if (rcColumn.height < sizeChar.height*5/3 || rcColumn.width < sizeChar.width/3)
			ColGroup.erase(ColGroup.begin()+i);
	}

	nColumnSize = ColGroup.size();
	//if (nColumnSize != 0)
	//	SetAutoSequence(0, ColGroup, sizeChar);
	PICSortLeftSegment(ColGroup);
	for (i=nColumnSize-1; i>=0; i--)
	{
		rcColumn = ColGroup.at(i);

		if (i < nColumnSize-1)
			nIntersectWidth = min(rcColumn.br().x, ColGroup.at(i+1).br().x) - max(rcColumn.x, ColGroup.at(i+1).x);
		else
			nIntersectWidth = 0;
		if (nIntersectWidth < 0)
			nIntersectWidth = 0;
		if (i > 0)
			nIntersectWidth += min(rcColumn.br().x, ColGroup.at(i-1).br().x) - max(rcColumn.x, ColGroup.at(i-1).x);

		if (nIntersectWidth > rcColumn.width*8/10)
		{
			ColGroup.erase(ColGroup.begin()+i);
			nColumnSize--;
		}
	}

	// 인쇄체인 경우 갈라진 문자행의 합치기(HJH)
	if (!bHandWrite)
	{
		nColumnSize = ColGroup.size();
		if (nColumnSize != 0)
			SetAutoSequence(0, ColGroup, sizeChar);

		for (i=0; i<nColumnSize-1; i++)
		{
			rcColumn = ColGroup.at(i);

			for (j=i+1; j<nColumnSize; j++)
			{
				rcNextColumn = ColGroup.at(j);

				nIntersectWidth = min(rcColumn.br().x, rcNextColumn.br().x) - max(rcColumn.x, rcNextColumn.x);

				if (nIntersectWidth > rcColumn.width*4/10 || nIntersectWidth > rcNextColumn.width*4/10)
				{
					rcColumn = UnionRect(rcColumn, rcNextColumn);
					ColGroup.at(i) = rcColumn;
					ColGroup.erase(ColGroup.begin()+j);
					nColumnSize--;
					j--;
				}
			}
		}
	}

	return TRUE;
}

BOOL CalcHorzColGroup(const Mat &image, int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, COL_GROUP &ColGroup)
{
	// 세그먼트의 기준 너비 및 높이 얻기(HJH)
	Size	sizeChar = CalcCharSize(bHandWrite, SegGroup);

	sizeChar.width = sizeChar.width * 133 / 100;
	sizeChar.height = sizeChar.height * 120 / 100;

	// 행정보의 초기구성(HJH)
	Rect	rcSegment, rcColumn;
	int		nSize, nColumnSize,
			nUnionHeight, nDistance, nIntersectHeight,
			i;
	BOOL	bDot = FALSE;

	//SetAutoSequence(1, SegGroup, sizeChar);
	PICSortTopSegment(SegGroup);

	nSize = SegGroup.size();
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		if (rcSegment.height > sizeChar.height || rcSegment.height < sizeChar.height/6)
			continue;

		nColumnSize = ColGroup.size();
		if (nColumnSize == 0)
			ColGroup.push_back(rcSegment);
		else
		{
			rcColumn = ColGroup.at(nColumnSize-1);

			if (bHandWrite)
			{
				nDistance = max(rcSegment.x, rcColumn.x) - min(rcSegment.br().x, rcColumn.br().x);
				nUnionHeight = max(rcSegment.br().y, rcColumn.br().y) - min(rcSegment.y, rcColumn.y);
				if (nDistance > sizeChar.width*2 || nUnionHeight > sizeChar.height)
					ColGroup.push_back(rcSegment);
				else
				{
					rcColumn = UnionRect(rcColumn, rcSegment);
					ColGroup.at(nColumnSize-1) = rcColumn;
				}
			}
			else
			{
				nIntersectHeight = min(rcSegment.br().y, rcColumn.br().y) - max(rcSegment.y, rcColumn.y);
				nUnionHeight = max(rcSegment.br().y, rcColumn.br().y) - min(rcSegment.y, rcColumn.y);
				if (nIntersectHeight < sizeChar.height*2/10 && nUnionHeight > sizeChar.height*7/6)
				{
					ColGroup.push_back(rcSegment);
				}
				else
				{
					rcColumn = UnionRect(rcColumn, rcSegment);
					ColGroup.at(nColumnSize-1) = rcColumn;
				}
			}
		}
	}

	if (ColGroup.size() > 8)
	{
		bDot = TRUE;
	}

	// 문자행사이 겹침면적 및 문자행의 너비대 높이비률에 기초한 문자행 합치기(HJH)
	Rect	rcNextColumn;
	int		nSquare, nNextSquare,
			nIntersectWidth,
			nUnionWidth,
			j;
	BOOL	bContinue = TRUE;

	//SetAutoSequence(1, ColGroup, sizeChar);
	PICSortTopSegment(ColGroup);

	while (bContinue)
	{
		bContinue = FALSE;

		nColumnSize = ColGroup.size();
		for (i = 0; i < nColumnSize-1; i++)
		{
			rcColumn = ColGroup.at(i);
			nSquare = rcColumn.width * rcColumn.height;

			for (j = i+1; j < nColumnSize; j++)
			{
				rcNextColumn = ColGroup.at(j);
				nNextSquare = rcNextColumn.width * rcNextColumn.height;

				nIntersectWidth = min(rcColumn.br().x, rcNextColumn.br().x) - max(rcColumn.x, rcNextColumn.x);
				nIntersectHeight = min(rcColumn.br().y, rcNextColumn.br().y) - max(rcColumn.y, rcNextColumn.y);
				nUnionWidth = max(rcColumn.br().x, rcNextColumn.br().x) - min(rcColumn.x, rcNextColumn.x);
				nUnionHeight = max(rcColumn.br().y, rcNextColumn.br().y) - min(rcColumn.y, rcNextColumn.y);

				if ((nIntersectWidth >= 0 && nIntersectHeight >= -(max(rcColumn.height, rcNextColumn.height)+5)/10)
					|| (rcColumn.width > rcColumn.height*25 && rcNextColumn.width > rcNextColumn.height*25 && nUnionWidth > nUnionHeight*25))
				{
					rcColumn = UnionRect(rcColumn, rcNextColumn);
					ColGroup.at(i) = rcColumn;
					ColGroup.erase(ColGroup.begin()+j);
					nColumnSize--;
					j--;

					bContinue = TRUE;
				}
			}
		}
	}

	// 좌우로 린접한 행의 너비조건에 의한 합치기(HJH)
	nColumnSize = ColGroup.size();
	//if (nColumnSize != 0)
	//	SetAutoSequence(1, ColGroup, sizeChar);
	PICSortTopSegment(ColGroup);

	for (i=0; i<nColumnSize-1; i++)
	{
		rcColumn = ColGroup.at(i);

		for (j=i+1; j<nColumnSize; j++)
		{
			rcNextColumn = ColGroup.at(j);

			nUnionHeight = max(rcColumn.br().y, rcNextColumn.br().y) - min(rcColumn.y, rcNextColumn.y);

			if (rcNextColumn.width > sizeChar.width && 
				nUnionHeight < sizeChar.height*12/10 && 
				rcColumn.height >= rcNextColumn.height/2 && rcColumn.height/2 <= rcNextColumn.height)
			{
				rcColumn = UnionRect(rcColumn, rcNextColumn);
				ColGroup.at(i) = rcColumn;
				ColGroup.erase(ColGroup.begin()+j);
				nColumnSize--;
				j--;
			}
		}
	}

	// 정확치 못한 행세그먼트의 제거(HJH)
	nColumnSize = ColGroup.size();
	for (i=nColumnSize-1; i>=0; i--)
	{
		rcColumn = ColGroup.at(i);

		if (rcColumn.width < sizeChar.width*5/3 || rcColumn.height < sizeChar.height/3)
			ColGroup.erase(ColGroup.begin()+i);
	}

	nColumnSize = ColGroup.size();
	//if (nColumnSize != 0)
	//	SetAutoSequence(1, ColGroup, sizeChar);
	PICSortTopSegment(ColGroup);
	for (i=nColumnSize-1; i>=0; i--)
	{
		rcColumn = ColGroup.at(i);

		if (i < nColumnSize-1)
			nIntersectHeight = min(rcColumn.br().y, ColGroup.at(i+1).br().y) - max(rcColumn.y, ColGroup.at(i+1).y);
		else
			nIntersectHeight = 0;
		if (nIntersectHeight < 0)
			nIntersectHeight = 0;
		if (i > 0)
			nIntersectHeight += min(rcColumn.br().y, ColGroup.at(i-1).br().y) - max(rcColumn.y, ColGroup.at(i-1).y);

		if (nIntersectHeight > rcColumn.height*8/10)
		{
			ColGroup.erase(ColGroup.begin()+i);
			nColumnSize--;
		}
	}

	// 인쇄체인 경우 갈라진 문자행의 합치기(HJH)
	if (!bHandWrite) // 인쇄체문서이면(HJH)
	{
		nColumnSize = ColGroup.size();
		if (nColumnSize != 0)
			SetAutoSequence(1, ColGroup, sizeChar);

		for (i=0; i<nColumnSize-1; i++)
		{
			rcColumn = ColGroup.at(i);

			for (j=i+1; j<nColumnSize; j++)
			{
				rcNextColumn = ColGroup.at(j);

				nIntersectHeight = min(rcColumn.br().y, rcNextColumn.br().y) - max(rcColumn.y, rcNextColumn.y);

				if (nIntersectHeight > rcColumn.width*4/10 || nIntersectHeight > rcNextColumn.width*4/10)
				{
					rcColumn = UnionRect(rcColumn, rcNextColumn);
					ColGroup.at(i) = rcColumn;
					ColGroup.erase(ColGroup.begin()+j);
					nColumnSize--;
					j--;
				}
			}
		}
	}

	if (!bDot && ColGroup.size() == 1)
	{
		int		y = GetSeparationY(image, 1-nDirection, bHandWrite, ColGroup[0]);
		Rect	rcNewColumn;

		if (y > ColGroup[0].y+ColGroup[0].height/3 && y < ColGroup[0].y+ColGroup[0].height*2/3)
		{
			rcNewColumn.x = ColGroup[0].x;
			rcNewColumn.y = y;
			rcNewColumn.width = ColGroup[0].width;
			rcNewColumn.height = ColGroup[0].br().y - y;

			ColGroup[0].height = y - ColGroup[0].y;

			ColGroup.push_back(rcNewColumn);
		}
	}

	// 무효한 행의 제거
	nColumnSize = ColGroup.size();
	for (i = nColumnSize-1; i >= 0; i--)
	{
		rcColumn = ColGroup.at(i);

		if (rcColumn.height < image.rows/5)
		{
			ColGroup.erase(ColGroup.begin()+i);
		}
	}

	return TRUE;
}

BOOL CalcColGroup(const Mat &image, int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, COL_GROUP &ColGroup)
{
	if (nDirection == 0) // vertical
	{
		if (!CalcVertColGroup(bHandWrite, SegGroup, ColGroup))
			return FALSE;
	}
	else
	{
		if (!CalcHorzColGroup(image, nDirection, bHandWrite, SegGroup, ColGroup))
			return FALSE;
	}

	return TRUE;
}

BOOL CalcColSegGroup(BOOL bHandWrite, SEG_GROUP &SegGroup, const COL_GROUP &ColGroup, COL_SEG_GROUP &ColSegGroup)
{
	Rect	rcSegment, rcColumn;
	int		nSize, nColumnSize,
			nSquare, nIntersectSquare,
			nMaxIntersect, nMaxIntersectNum,
			i, j;
	BOOL	bFirst = TRUE;

	nSize = SegGroup.size();
	for (i=0; i<nSize; i++)
	{
		rcSegment = SegGroup.at(i);
		nSquare = rcSegment.width * rcSegment.height;

		nMaxIntersect = 0;
		nMaxIntersectNum = 0;

		nColumnSize = ColGroup.size();
		for (j = 0; j < nColumnSize; j++)
		{
			rcColumn = ColGroup.at(j);

			if (bFirst)
			{
				SEG_GROUP	TempGroup;

				ColSegGroup.push_back(make_pair(rcColumn, TempGroup));
			}

			nIntersectSquare = min(rcSegment.br().x, rcColumn.br().x) - max(rcSegment.x, rcColumn.x);
			if (nIntersectSquare > 0)
				nIntersectSquare *= (min(rcSegment.br().y, rcColumn.br().y) - max(rcSegment.y, rcColumn.y));
			else
				nIntersectSquare = 0;
			if (nIntersectSquare < 0)
				nIntersectSquare = 0;

			if (nMaxIntersect < nIntersectSquare)
			{
				nMaxIntersect = nIntersectSquare;
				nMaxIntersectNum = j;
			}
		}

		if (nMaxIntersect > nSquare*(bHandWrite? 65:80)/100)
		{
			ColSegGroup.at(nMaxIntersectNum).second.push_back(rcSegment);
			SegGroup.erase(SegGroup.begin()+i);

			nSize--;
			i--;
		}

		bFirst = FALSE;
	}

	return TRUE;
}

#if 0
int SegmentationByHistogram(const Mat &image, int nDirection, RES_SEG_GROUP &SegmentList)	// Only vertical document
{
	COL_GROUP	ColGroup;
	SEG_GROUP	SegGroup,
				ArticleSymbolGroup;
	Size		sizeChar;

	if (nDirection == 1)
	{
		return 0;
	}

	CalcColGroupByHistogram(image, nDirection, ColGroup, ArticleSymbolGroup);
	ColumnSegmentationByHistogram(ColGroup, SegGroup);

	sizeChar = CalcCharSize(SegGroup);
	SetAutoSequence(m_nDocumentType, SegGroup, sizeChar);

	ConstructResultSegGroup(SegGroup, ArticleSymbolGroup, SegmentList);

	return SegmentList.size();
}
#endif

BOOL SecondMergeSegment(int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, Size sizeChar)
{
	BOOL	bMerge = TRUE, bRet = FALSE;
	int		nDistance;
	Rect	rcSegment, rcNextSegment, rcTmp;
	int		nSize,
			i, j;

	if (nDirection == 0) // vertical
	{
		if (bHandWrite)
		{
			sizeChar.width = sizeChar.width * 110 / 100;
			sizeChar.height = sizeChar.height * 133 / 100;
		}
		else
		{
			sizeChar.width = sizeChar.width * 170 / 100;
			sizeChar.height = sizeChar.height * 135 / 100;
		}

		// 가로방향으로 놓인 세그먼트들의 통합(HJH)
		int	dh;

		SetAutoSequence(nDirection, SegGroup, sizeChar);

		if (bHandWrite)
		{
			nSize = SegGroup.size();
			for (i=0; i<nSize-1; i++)
			{
				rcSegment = SegGroup.at(i);
				for (j=i+1; j<nSize; j++)
				{
					rcNextSegment = SegGroup.at(j);

					dh = sizeChar.height / 10;
					if ((rcSegment.y-dh <= rcNextSegment.y && rcSegment.br().y+dh >= rcNextSegment.br().y)
						|| (rcNextSegment.y-dh <= rcSegment.y && rcNextSegment.br().y+dh >= rcSegment.br().y))
					{
						rcTmp = UnionRect(rcSegment, rcNextSegment);
						if (rcTmp.width > sizeChar.width)
							continue;
						SegGroup.at(i) = rcSegment = rcTmp;
						SegGroup.erase(SegGroup.begin()+j);
						nSize--;
						j--;
						bRet = TRUE;
					}
				}
			}
		}
		else // 인쇄체문서이면(HJH)
		{
			nSize = SegGroup.size();
			for (i=0; i<nSize-1; i++)
			{
				rcSegment = SegGroup.at(i);
				for (j=i+1; j<nSize; j++)
				{
					rcNextSegment = SegGroup.at(j);

					if (min(rcSegment.br().y, rcNextSegment.br().y)-max(rcSegment.y, rcNextSegment.y) > 0)
					{
						nDistance = max(rcSegment.x, rcNextSegment.x) - min(rcSegment.br().x, rcNextSegment.br().x);
						rcTmp = UnionRect(rcSegment, rcNextSegment);
						if ((rcTmp.width > sizeChar.width && nDistance > rcTmp.width/10)
							|| rcTmp.width > sizeChar.width*115/100)
							continue;
						SegGroup.at(i) = rcSegment = rcTmp;
						SegGroup.erase(SegGroup.begin()+j);
						nSize--;
						j--;
						bRet = TRUE;
					}
				}
			}
		}
	}
	else // horizontal
	{
		if (bHandWrite)
		{
			sizeChar.width = sizeChar.width * 133 / 100;
			sizeChar.height = sizeChar.height * 110 / 100;
		}
		else
		{
			sizeChar.width = sizeChar.width * 133 / 100;
			sizeChar.height = sizeChar.height * 135 / 100;
		}

		// 세로방향으로 놓인 세그먼트들의 통합(HJH)
		int	dw;

		SortHeightSegment(SegGroup);

		if (bHandWrite)
		{
			nSize = SegGroup.size();
			for (i=0; i<nSize-1; i++)
			{
				rcSegment = SegGroup.at(i);
				for (j=i+1; j<nSize; j++)
				{
					rcNextSegment = SegGroup.at(j);

					dw = rcSegment.width / 10;
					if (rcSegment.x-dw <= rcNextSegment.x && rcSegment.br().x+dw >= rcNextSegment.br().x && 
						min(rcSegment.height, rcNextSegment.height)>3*(max(rcSegment.y, rcNextSegment.y)-min(rcSegment.br().y, rcNextSegment.br().y)))
					{
						rcTmp = UnionRect(rcSegment, rcNextSegment);
						if (rcTmp.width > sizeChar.width || rcTmp.height > sizeChar.height)
							continue;
						SegGroup.at(i) = rcSegment = rcTmp;
						SegGroup.erase(SegGroup.begin()+j);
						nSize--;
						j--;
						bRet = TRUE;
					}
				}
			}
		}
		else
		{
			nSize = SegGroup.size();
			for (i=0; i<nSize-1; i++)
			{
				rcSegment = SegGroup.at(i);
				for (j=i+1; j<nSize; j++)
				{
					rcNextSegment = SegGroup.at(j);

					if (min(rcSegment.br().x, rcNextSegment.br().x)-max(rcSegment.x, rcNextSegment.x) > 0)
					{
						nDistance = max(rcSegment.y, rcNextSegment.y) - min(rcSegment.br().y, rcNextSegment.br().y);
						rcTmp = UnionRect(rcSegment, rcNextSegment);
						if ((rcTmp.height > sizeChar.height && nDistance > rcTmp.height/10)
							|| rcTmp.height > sizeChar.height*115/100)
							continue;
						SegGroup.at(i) = rcSegment = rcTmp;
						SegGroup.erase(SegGroup.begin()+j);
						nSize--;
						j--;
						bRet = TRUE;
					}
				}
			}
		}
	}

	return bRet;
}

BOOL ThirdMergeSegment(int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, Size sizeChar)
{
	// 세그먼트의 기준 너비 및 높이 얻기(HJH)
	if (nDirection == 0) // 세로문서이면(HJH)
	{
		if (bHandWrite) // 필기체문서이면(HJH)
		{
			sizeChar.width = sizeChar.width * 140 / 100;
			sizeChar.height = sizeChar.height * 125 / 100;
		}
		else // 인쇄체문서이면(HJH)
		{
			sizeChar.width = sizeChar.width * 140 / 100;
			sizeChar.height = sizeChar.height * 122 / 100;
		}
	}
	else // 가로문서이면(HJH)
	{
		if (bHandWrite) // 필기체문서이면(HJH)
		{
			sizeChar.width = sizeChar.width * 120 / 100;
			sizeChar.height = sizeChar.height * 140 / 100;
		}
		else // 인쇄체문서이면(HJH)
		{
			sizeChar.width = sizeChar.width * 140/*110*/ / 100;
			sizeChar.height = sizeChar.height * 140 / 100;
		}
	}

	// 합쳐서 한개의 글자로 될수 있는 세그먼트들의 통합
	BOOL	bMerge = TRUE, bRet = FALSE;
	int		nPrev, nNext;
	Rect	rcSegment, rcPrevSegment, rcNextSegment, rcTmp;
	int		nSize,
			nIntersectSquare, nUnionSquare,
			nDistance,
			nUnionWidth, nUnionHeight,
			nPrevDist, nNextDist,
			//nPrevLen, nNextLen,
			i;

	bMerge = TRUE;
	while (bMerge)
	{
		bMerge = FALSE;

		// 세그먼트를 문서방향으로 정렬(HJH)
		SetAutoSequence(nDirection, SegGroup, sizeChar);

		// 합치기(HJH)
		nSize = SegGroup.size();
		for (i=0; i<nSize; i++)
		{
			rcSegment = SegGroup.at(i);
			nPrev = nNext = -1;

			// 앞의 세그먼트와 합칠수 있는가를 검사(HJH)
			if (i == 0)
				nPrev = 0;
			else
			{
				rcPrevSegment = SegGroup.at(i-1);

				nIntersectSquare = min(rcSegment.br().x, rcPrevSegment.br().x) - max(rcSegment.x, rcPrevSegment.x);
				if (nIntersectSquare > 0)
					nIntersectSquare *= 10*(min(rcSegment.br().y, rcPrevSegment.br().y) - max(rcSegment.y, rcPrevSegment.y))/3;
				else
					nIntersectSquare = 0;
				if (nIntersectSquare < 0)
					nIntersectSquare = 0;

				if (nIntersectSquare == 0 && bHandWrite)
				{
					if (sizeChar.height/6 < max(rcSegment.y, rcPrevSegment.y)-min(rcSegment.br().y, rcPrevSegment.br().y)
						|| sizeChar.width/6 < max(rcSegment.x, rcPrevSegment.x)-min(rcSegment.br().x, rcPrevSegment.br().x))
						nPrev = 0;
				}

				if (nPrev == -1)
				{
					nUnionWidth = max(rcSegment.br().x, rcPrevSegment.br().x) - min(rcSegment.x, rcPrevSegment.x);
					nUnionHeight = max(rcSegment.br().y, rcPrevSegment.br().y) - min(rcSegment.y, rcPrevSegment.y);
					nUnionSquare = nUnionWidth * nUnionHeight;

					if (nUnionWidth == rcSegment.width || nUnionWidth == rcPrevSegment.width)
					{
						if (nUnionHeight <= sizeChar.height)
							nPrev = 1;
						else
							nPrev = 0;
					}
					else
					{
						if (nUnionHeight == rcSegment.height || nUnionHeight == rcPrevSegment.height)
						{
							if (nUnionWidth <= sizeChar.width)
								nPrev = 1;
							else
								nPrev = 0;
						}
						else
						{
							if (nUnionWidth <= sizeChar.width && nUnionHeight <= sizeChar.height)
								nPrev = 1;
							else
								nPrev = 0;
						}
					}
				}
			}

			// 뒤의 세그먼트와 합칠수 있는가를 검사(HJH)
			if (i == nSize-1)
				nNext = 0;
			else
			{
				rcNextSegment = SegGroup.at(i+1);

				nIntersectSquare = min(rcSegment.br().x, rcNextSegment.br().x) - max(rcSegment.x, rcNextSegment.x);
				if (nIntersectSquare > 0)
					nIntersectSquare *= 10*(min(rcSegment.br().y, rcNextSegment.br().y) - max(rcSegment.y, rcNextSegment.y))/3;
				else
					nIntersectSquare = 0;
				if (nIntersectSquare < 0)
					nIntersectSquare = 0;

				if (nIntersectSquare == 0 && bHandWrite)
				{
					if (sizeChar.height/6 < max(rcSegment.y, rcNextSegment.y)-min(rcSegment.br().y, rcNextSegment.br().y)
						|| sizeChar.width/6 < max(rcSegment.x, rcNextSegment.x)-min(rcSegment.br().x, rcNextSegment.br().x))
						nNext = 0;
				}

				if (nNext == -1)
				{
					nUnionWidth = max(rcSegment.br().x, rcNextSegment.br().x) - min(rcSegment.x, rcNextSegment.x);
					nUnionHeight = max(rcSegment.br().y, rcNextSegment.br().y) - min(rcSegment.y, rcNextSegment.y);
					nUnionSquare = nUnionWidth * nUnionHeight;

					if (nUnionWidth == rcSegment.width || nUnionWidth == rcNextSegment.width)
					{
						if (nUnionHeight <= sizeChar.height)
							nNext = 1;
						else
							nNext = 0;
					}
					else
					{
						if (nUnionHeight == rcSegment.height || nUnionHeight == rcNextSegment.height)
						{
							if (nUnionWidth <= sizeChar.width)
								nNext = 1;
							else
								nNext = 0;
						}
						else
							if (nUnionWidth <= sizeChar.width && nUnionHeight <= sizeChar.height)
								nNext = 1;
							else
								nNext = 0;
					}
				}
			}

			if (nPrev == 1)
			{
				if (nNext == 1)
				{
					// 우아래세그먼트들중 적당한것을 선택(HJH)
					if (!bHandWrite) // 인쇄체문서이면(HJH)
					{
						if (nDirection == 0) // 세로문서이면(HJH)
						{
							nPrevDist = rcSegment.y - rcPrevSegment.br().y;
							nNextDist = rcNextSegment.y - rcSegment.br().y;
						}
						else // 가로문서이면(HJH)
						{
							nPrevDist = rcSegment.x - rcPrevSegment.br().x;
							nNextDist = rcNextSegment.x - rcSegment.br().x;
						}

						if (nPrevDist < nNextDist)
							nNext = 0;
					}
					else // 필기체문서이면(HJH)
					{
						if (nDirection == 0) // 세로문서이면(HJH)
						{
							nPrevDist = rcSegment.y - rcPrevSegment.br().y;
							nNextDist = rcNextSegment.y - rcSegment.br().y;
						}
						else // 가로문서이면(HJH)
						{
							nPrevDist = rcSegment.x - rcPrevSegment.br().x;
							nNextDist = rcNextSegment.x - rcSegment.br().x;
						}

						if (nPrevDist < nNextDist)
							nNext = 0;
					}
				}
				if (nNext == 0)
				{
					// 웃세그먼트와 합치기(HJH)
					nDistance = max(rcSegment.x, rcPrevSegment.x) - min(rcSegment.br().x, rcPrevSegment.br().x);
					if (nDistance*18 <= sizeChar.width)
					{
						rcTmp = UnionRect(rcSegment, rcPrevSegment);
						SegGroup.at(i-1) = rcTmp;
						SegGroup.erase(SegGroup.begin()+i);
						bMerge = TRUE;
						bRet = TRUE;
						break;
					}
				}
			}
		}
	}

	return bRet;
}

BOOL FinalMergeSegment(BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	BOOL	bRet = FALSE;
	int		nSize;
	Rect	rcSegment, rcNextSegment;
	int		nSquare, nNextSquare, nIntersectSquare,
			i, j;

	// 졋굉뱌뚱롶쇄톡 곺훰벽탱탸 땟텔뺄컴 땄뺴
	nSize = SegGroup.size();
	for (i=0; i<nSize-1; i++)
	{
		rcSegment = SegGroup.at(i);
		nSquare = rcSegment.width * rcSegment.height;
		for (j=i+1; j<nSize; j++)
		{
			rcNextSegment = SegGroup.at(j);
			nNextSquare = rcNextSegment.width * rcNextSegment.height;

			nIntersectSquare = min(rcSegment.br().x, rcNextSegment.br().x) - max(rcSegment.x, rcNextSegment.x);
			if (nIntersectSquare > 0)
				nIntersectSquare *= (min(rcSegment.br().y, rcNextSegment.br().y) - max(rcSegment.y, rcNextSegment.y));
			else
				nIntersectSquare = 0;
			if (nIntersectSquare < 0)
				nIntersectSquare = 0;

			if (bHandWrite)
			{
				if (nIntersectSquare*2 > nSquare ||
					nIntersectSquare*2 > nNextSquare)
				{
					rcSegment = UnionRect(rcSegment, rcNextSegment);
					SegGroup.at(i) = rcSegment;
					SegGroup.erase(SegGroup.begin()+j);
					nSize--;
					j--;
					bRet = TRUE;
				}
			}
			else
			{
				if (nIntersectSquare*2 > nSquare ||
					nIntersectSquare*2 > nNextSquare)
				{
					rcSegment = UnionRect(rcSegment, rcNextSegment);
					SegGroup.at(i) = rcSegment;
					SegGroup.erase(SegGroup.begin()+j);
					nSize--;
					j--;
					bRet = TRUE;
				}
			}
		}
	}

	return bRet;
}

void RemoveInvalidSegment(BOOL bHandWrite, RES_SEG_GROUP &SegmentList)
{
    Rect	rcSegment;
    int		nSize, i;

#if 0
    Size	sizeChar = CalcCharSize(bHandWrite, SegmentList);

    nSize = SegmentList.size();
    for (i=nSize-1; i>=0; i--)
    {
        rcSegment = SegmentList.at(i).rChar;

        if (rcSegment.width < sizeChar.width/4 && rcSegment.height < sizeChar.height/4)
            SegmentList.erase(SegmentList.begin()+i);
    }
#else
    Size	sizeChar(0, 0);

    nSize = SegmentList.size();
	if (!nSize)
	{
		return;
	}

    for (i = 0; i < nSize; i++)
    {
        rcSegment = SegmentList.at(i).rChar;
        sizeChar.width += rcSegment.width;
        sizeChar.height += rcSegment.height;
    }

    sizeChar.width /= nSize;
    sizeChar.height /= nSize;

    for (i = nSize-1; i >= 0; i--)
    {
        rcSegment = SegmentList.at(i).rChar;

        if (rcSegment.height*3 < sizeChar.height*2
            || rcSegment.width*rcSegment.height*6 < sizeChar.width*sizeChar.height)
        {
            SegmentList.erase(SegmentList.begin()+i);
        }
    }
#endif
}

void RemoveInvalidSegment2(SEG_GROUP &SegGroup)
{
	// 세그먼트의 기준 너비 및 높이 얻기(HJH)
	int	nSize,
		width, height,
		nCurrWidth = 0, nCurrHeight = 0,
		nCurrCountW = 0, nCurrCountH = 0,
		nMaxNumW = 0, nMaxCountW = 0,
		nMaxNumH = 0, nMaxCountH = 0,
		i;

	nSize = SegGroup.size();
	SortWidthSegment(SegGroup);

	for (i=nSize/2; i<nSize; i++)
	{
		width = SegGroup.at(i).width / 2 * 2;
		if (width != nCurrWidth)
		{
			if (nMaxCountW <= nCurrCountW)
			{
				nMaxCountW = nCurrCountW;
				nMaxNumW = nCurrWidth;
			}
			nCurrWidth = width;
			nCurrCountW = 0;
		}
		nCurrCountW++;
	}
	if (nMaxNumW == 0)
		nMaxNumW = nCurrWidth;

	SortHeightSegment(SegGroup);

	for (i=nSize/64; i<nSize/4+1; i++)
	{
		height = SegGroup.at(i).height / 2 * 2;
		if (height != nCurrHeight)
		{
			if (nMaxCountH <= nCurrCountH)
			{
				nMaxCountH = nCurrCountH;
				nMaxNumH = nCurrHeight;
			}
			nCurrHeight = height;
			nCurrCountH = 0;
		}
		nCurrCountH++;
	}
	if (nMaxNumH == 0)
		nMaxNumH = nCurrHeight;

	nMaxNumW = nMaxNumW * 125 / 100;
	nMaxNumH = nMaxNumH * 125 / 100;

	Rect	rect;

	nSize = SegGroup.size();
	for (i=nSize-1; i>=0; i--)
	{
		rect = SegGroup.at(i);
		if (rect.width < nMaxNumW/14 || rect.height < nMaxNumH/14)
			SegGroup.erase(SegGroup.begin()+i);
	}
}

void ColumnSegmentation(int nDirection, BOOL bHandWrite, COL_SEG_GROUP &ColSegGroup)
{
	int		nColumnSize,
			i;
	Size	sizeChar;
	BOOL	bContinue = TRUE;

	nColumnSize = ColSegGroup.size();

	while (bContinue && nColumnSize)
	{
		bContinue = FALSE;
		sizeChar = CalcCharSize(bHandWrite, ColSegGroup);
		for (i = 0; i < nColumnSize; i++)
		{
			if (SecondMergeSegment(nDirection, bHandWrite, ColSegGroup.at(i).second, sizeChar))
			{
				bContinue = TRUE;
			}
		}
	}

	bContinue = TRUE;
	while (bContinue && nColumnSize)
	{
		bContinue = FALSE;
		sizeChar = CalcCharSize(bHandWrite, ColSegGroup);
		for (i = 0; i < nColumnSize; i++)
		{
			if (ThirdMergeSegment(nDirection, bHandWrite, ColSegGroup.at(i).second, sizeChar))
			{
				bContinue = TRUE;
			}
		}
	}

	for (i = 0; i < nColumnSize; i++)
	{
		RemoveInvalidSegment2(ColSegGroup.at(i).second);
		PICSortLeftSegment(ColSegGroup.at(i).second);
	}
}

void AdjustSegmentSizeByHeight(const Mat &image, int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	Rect	rcSegment;
	Size	sizeChar;
	int		nSize,
			nHeight, nWidth,
			nHeightWithPrev, nWidthWithPrev, nDistance,
			nMaxValidHeight1, nMaxValidHeight2, nMaxValidWidth1, nMaxValidWidth2,
			y, x,
			i;
	BOOL	bSplit;

	sizeChar = CalcCharSize(bHandWrite, SegGroup);
	SetAutoSequence(nDirection, SegGroup, sizeChar);

	if (nDirection == 0) // 세로문서이면(HJH)
	{
		if (bHandWrite) // 필기체문서이면(HJH)
		{
			nMaxValidHeight1 = sizeChar.height * 130 / 100;
			nMaxValidHeight2 = sizeChar.height * 125 / 100;
		}
		else // 인쇄체문서이면(HJH)
		{
			nMaxValidHeight1 = sizeChar.height * 110 / 100;
			nMaxValidHeight2 = sizeChar.height * 90 / 100;
		}

		nSize = SegGroup.size();
		for (i=0; i<nSize; i++)
		{
			rcSegment = SegGroup.at(i);
			bSplit = FALSE;

			nHeight = rcSegment.height;
			nWidth = rcSegment.width;
			if (nHeight > nMaxValidHeight1) // 높이가 크고 가로세로비가 맞지 않는 세그먼트이면
			{
				// 세로방향으로 여러 세그먼트가 합쳐진것으로 판정하고 가르기 진행
				y = GetSeparationY(image, nDirection, bHandWrite, rcSegment);
				if (y != rcSegment.br().y)
				{
					Rect	rcNewSegment;

					rcNewSegment = rcSegment;
					rcNewSegment.height -= y - rcNewSegment.y;
					rcNewSegment.y = y;

					rcSegment.height = y - rcSegment.y;

					FixSegmentSize(image, rcSegment);
					SegGroup.at(i) = rcSegment;
					FixSegmentSize(image, rcNewSegment);
					SegGroup.insert(SegGroup.begin()+i+1, rcNewSegment);

					nSize++;
					bSplit = TRUE;
				}
			}

			if (i == 0)
				nHeightWithPrev = INT_MAX;
			else
			{
				if (SegGroup.at(i-1).br().x-rcSegment.br().x < sizeChar.width/2)
				{
					nHeightWithPrev = rcSegment.br().y - SegGroup.at(i-1).y;
					nDistance = rcSegment.y-SegGroup.at(i-1).br().y;
				}
				else
					nHeightWithPrev = INT_MAX;
			}
			if (nHeightWithPrev <= nMaxValidHeight2 && nHeightWithPrev > 0) // 앞의 세그먼트와 합친 경우의 높이가 작으면
			{
				// 앞의 세그먼트와 합치기
				SegGroup.at(i-1) = UnionRect(rcSegment, SegGroup.at(i-1));
				SegGroup.erase(SegGroup.begin()+i);
				i--;
				nSize--;
				continue;
			}
			else
				if (bSplit == TRUE)
					i--;
		}
	}
	else // 가로문서이면(HJH)
	{
		if (bHandWrite) // 필기체문서이면(HJH)
		{
			nMaxValidWidth1 = sizeChar.width * 130 / 100;
			nMaxValidWidth2 = sizeChar.width * 125 / 100;
		}
		else // 인쇄체문서이면(HJH)
		{
			nMaxValidWidth1 = sizeChar.width * 110 / 100;
			nMaxValidWidth2 = sizeChar.width * 110 / 100;
		}

		nSize = SegGroup.size();
		for (i=0; i<nSize; i++)
		{
			rcSegment = SegGroup.at(i);
			bSplit = FALSE;

			nWidth = rcSegment.width;
			nHeight = rcSegment.height;
			if (nWidth > nMaxValidWidth1/* && 
				!IsValidRatio(rcSegment)*/) // 너비가 크고 가로세로비가 맞지 않는 세그먼트이면
			{
				// 가로방향으로 여러 세그먼트가 합쳐진것으로 판정하고 가르기 진행
				x = GetSeparationY(image, nDirection, bHandWrite, rcSegment);
				if (x != rcSegment.br().x)
				{
					Rect	rcNewSegment;

					rcNewSegment = rcSegment;
					rcNewSegment.width -= x - rcNewSegment.x;
					rcNewSegment.x = x;

					rcSegment.width = x - rcSegment.x;

					FixSegmentSize(image, rcSegment);
					SegGroup.at(i) = rcSegment;
					FixSegmentSize(image, rcNewSegment);
					SegGroup.insert(SegGroup.begin()+i+1, rcNewSegment);

					nSize++;
					bSplit = TRUE;
				}
			}

			if (i == 0)
				nWidthWithPrev = INT_MAX;
			else
			{
				if (rcSegment.br().y-SegGroup.at(i-1).br().y < sizeChar.height/2)
					nWidthWithPrev = rcSegment.br().x - SegGroup.at(i-1).x;
				else
					nWidthWithPrev = INT_MAX;
			}
			if (nWidthWithPrev <= nMaxValidWidth2 && nWidthWithPrev > 0) // 앞의 세그먼트와 합친 경우의 너비가 작으면
			{
				// 앞의 세그먼트와 합치기
				SegGroup.at(i-1) = UnionRect(rcSegment, SegGroup.at(i-1));
				SegGroup.erase(SegGroup.begin()+i);
				i--;
				nSize--;
				continue;
			}
			else
				if (bSplit == TRUE)
					i--;
		}
	}
}

void AdjustSegmentSize(const Mat &image, int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup)
{
	AdjustSegmentSizeByHeight(image, nDirection, bHandWrite, SegGroup);
}

void SplitWideSegment(const Mat &image, int nDirection, BOOL bHandWrite, COL_SEG_GROUP &ColSegGroup)
{
	Size	sizeChar;
	//int		nRatio = 0;
	int		nColumnSize, nSize,
			n = 0;

	sizeChar.width = sizeChar.height = 0;

	nColumnSize = ColSegGroup.size();
	for (int i = 0; i < nColumnSize; i++)
	{
		nSize = ColSegGroup[i].second.size();
		for (int j = 0; j < nSize; j++)
		{
			int	nWidth = ColSegGroup[i].second[j].width,
				nHeight = ColSegGroup[i].second[j].height;

			if (nWidth * 10 <= nHeight * 9 && nWidth * 5 >= nHeight * 2)
			{
				sizeChar.width += ColSegGroup[i].second[j].width;
				sizeChar.height += ColSegGroup[i].second[j].height;
				//nRatio += (nWidth * 100 + nHeight / 2) / nHeight;
				n++;
			}
		}
	}

	if (!n)
	{
		return;
	}

	sizeChar.width /= n;
	sizeChar.height /= n;
	//nRatio /= n;

	for (int i = 0; i < nColumnSize; i++)
	{
		nSize = ColSegGroup[i].second.size();
		for (int j = 0; j < nSize; j++)
		{
			Rect	rcSegment = ColSegGroup[i].second[j];
			int		nWidth = rcSegment.width,
					nHeight = rcSegment.height,
					nSplit;

			nSplit = (nWidth + sizeChar.width / 2) / sizeChar.width;
			//nSplit = ((nWidth * 100 + nHeight / 2) / nHeight + nRatio / 2) / nRatio;
#if 0
			for (int k = 1; k < nSplit; k++)
			{
				Rect	rcNewSegment;

				rcNewSegment.x = rcSegment.x + (nWidth * k / nSplit);
				rcNewSegment.y = rcSegment.y;
				rcNewSegment.width = nWidth / nSplit;
				rcNewSegment.height = nHeight;
				FixSegmentSize(image, rcNewSegment);

				ColSegGroup[i].second.insert(ColSegGroup[i].second.begin()+j+k, rcNewSegment);
			}

			if (nSplit)
			{
				ColSegGroup[i].second[j].width = nWidth / nSplit;
				j += nSplit - 1;
				nSize += nSplit - 1;
			}
#else
			if (nSplit > 1)
			{
				int		x = GetSeparationY(image, nDirection, bHandWrite, rcSegment);
				Rect	rcNewSegment;

				//if (x > rcSegment.x+rcSegment.width/8 && x < rcSegment.x+rcSegment.width*7/8)
				if (x > rcSegment.x+rcSegment.height*2/5 && x < rcSegment.x+rcSegment.height*3/5
					&& x > rcSegment.x && x < rcSegment.x+rcSegment.width-1)
				{
					rcNewSegment.x = x;
					rcNewSegment.y = rcSegment.y;
					rcNewSegment.width = rcSegment.br().x - x;
					rcNewSegment.height = nHeight;
					FixSegmentSize(image, rcNewSegment);

					ColSegGroup[i].second.insert(ColSegGroup[i].second.begin()+j+1, rcNewSegment);

					ColSegGroup[i].second[j].width = x - rcSegment.x;
					j--;
					nSize++;
				}
			}
#endif
		}
	}

	for (int i = 0; i < nColumnSize; i++)
	{
		nSize = ColSegGroup[i].second.size();
		for (int j = 0; j < nSize; j++)
		{
			Rect	rcSegment = ColSegGroup[i].second[j];
			int		nWidth = rcSegment.width,
					nHeight = rcSegment.height;

			if (nWidth > nHeight)
			{
				int		x = rcSegment.x + nWidth / 2;
				Rect	rcNewSegment;

				rcNewSegment.x = x;
				rcNewSegment.y = rcSegment.y;
				rcNewSegment.width = rcSegment.br().x - x;
				rcNewSegment.height = nHeight;
				FixSegmentSize(image, rcNewSegment);

				ColSegGroup[i].second.insert(ColSegGroup[i].second.begin()+j+1, rcNewSegment);

				ColSegGroup[i].second[j].width = x - rcSegment.x;
				j--;
				nSize++;
			}
		}
	}
}

void ConstructResultSegGroup(int nDirection, BOOL bHandWrite, SEG_GROUP &SegGroup, RES_SEG_GROUP &SegmentList)
{
	SEG_CHAR_INFO	SegmentCharInfo;
	int				nSize, i;

	SegmentList.clear();

	nSize = SegGroup.size();
	for (i = 0; i < nSize; i++)
	{
		SegmentCharInfo.rChar = SegGroup.at(i);
		SegmentCharInfo.nMultiFrameIndex = 0;
		SegmentCharInfo.dwSequence = SegmentList.size() + 1;
		SegmentCharInfo.nCharFlag = 1;
		SegmentCharInfo.nCharType = 0;
		SegmentCharInfo.nCharCode = 0;
		SegmentCharInfo.bShow = FALSE;
		SegmentCharInfo.bWrongIndex = FALSE;
		SegmentCharInfo.dwSpaceWordCount = 0;
		SegmentCharInfo.dwTotalBlackPixel = 0;

		SegmentList.push_back(SegmentCharInfo);
	}
}

int PICFrameSegmentation(const Mat &image, int nDirection, BOOL bHandWrite, PSEGMENT_RESULT pSegRes)
{
	SEG_GROUP		SegGroup, DblSegGroup,
					InitSegGroup,
					ArticleSymbolGroup;
	COL_GROUP		ColGroup, DblColGroup,
					ColumnGroup;
	COL_SEG_GROUP	ColSegGroup, DblColSegGroup;
	Size			sizeChar;
	int				nSize, nColumnSize,
					i, j;
	RES_SEG_GROUP	SegmentList;

	pSegRes->segments.clear();

	// 세그먼트집합 구성(HJH)
	TracePixelSegment(image, nDirection, bHandWrite, SegGroup, 0);
	RemoveInvalidSegment1(image, bHandWrite, SegGroup);

	// 1차세그먼트화 진행(HJH)
	while (FirstMergeSegment(bHandWrite, SegGroup));
#if 0
	Mat	temp;
	cvtColor(image, temp, CV_GRAY2BGR);
	for (i = 0; i < (int)SegGroup.size(); i++)
	{
		rectangle(temp, SegGroup[i].tl(), SegGroup[i].br(), Scalar(255, 0, 0, 0));
	}
	for (i = 0; i < (int)ColGroup.size(); i++)
	{
		Point	p1 = ColGroup[i].tl(),
				p2 = ColGroup[i].br();

		p1.x = max(0, p1.x-1);
		p1.y = max(0, p1.y-1);
		p2.x = min(image.cols-1, p2.x+1);
		p2.y = min(image.rows-1, p2.y+1);
		rectangle(temp, p1, p2, Scalar(0, 0, 255, 0));
	}
	imshow("aaa", temp);waitKey();
#endif

	// Segment lines
	if (bHandWrite)
	{
#if 0
		CalcColGroupByHistogram(image, nDirection, SegGroup, ColGroup);
#endif
	}
	else
	{
		CalcColGroup(image, nDirection, bHandWrite, SegGroup, ColGroup);
	}

#if 0
	nColumnSize = ColGroup.size();
	for (i = 0; i < nColumnSize; i++)
	{
		Rect	rcColumn = ColGroup.at(i);

		rcColumn.y = 0;
		rcColumn.height = image.rows;
		ColumnGroup.push_back(rcColumn);
	}

	if (bHandWrite) // 필기체이면서 틀선이 없으면(HJH)
	{
		// 행사이련결을 끊기(HJH)
		Rect	rcErase;

		nColumnSize = ColumnGroup.size();
		for (i = 1; i < nColumnSize; i++)
		{
			int	x = (ColumnGroup.at(i).x + ColumnGroup.at(i-1).br().x) / 2;
			int	offset = x;

			for (int y = 0; y < image.rows; y++)
			{
				image.data[offset] = 255;
				offset += image.cols;
			}
		}

		// 초기세그먼트화 다시 진행(HJH)
		SegGroup.clear();

		TracePixelSegment(image, bHandWrite, nDirection, SegGroup, 0);
		RemoveInvalidSegment1(image, bHandWrite, SegGroup);

		while (FirstMergeSegment(bHandWrite, SegGroup));
	}
#endif

	CalcColSegGroup(bHandWrite, SegGroup, ColGroup, ColSegGroup);

	// 문자행단위로 세그먼트화 진행(HJH)
	ColumnSegmentation(nDirection, bHandWrite, ColSegGroup);

	SplitWideSegment(image, nDirection, bHandWrite, ColSegGroup);
#if 0
	Mat	temp;
	cvtColor(image, temp, CV_GRAY2BGR);
	for (i = 0; i < (int)ColSegGroup.size(); i++)
	{
		for (j = 0; j < (int)ColSegGroup[i].second.size(); j++)
		{
			rectangle(temp, ColSegGroup[i].second[j].tl(), ColSegGroup[i].second[j].br(), Scalar(255, 0, 0, 0));
		}
	}
	for (i = 0; i < (int)ColGroup.size(); i++)
	{
		Point	p1 = ColGroup[i].tl(),
				p2 = ColGroup[i].br();

		p1.x = max(0, p1.x-1);
		p1.y = max(0, p1.y-1);
		p2.x = min(image.cols-1, p2.x+1);
		p2.y = min(image.rows-1, p2.y+1);
		rectangle(temp, p1, p2, Scalar(0, 0, 255, 0));
	}
	imshow("aaa", temp);waitKey();
#endif

#if 0
	// 어느 문자행에도 속하지 않은 세그먼트들에 대한 처리 진행(HJH)
	if (!bHandWrite)
	{
		nSize = SegGroup.size();
		nColumnSize = ColSegGroup.size();
		if (nSize != 0)
		{
			if (nColumnSize)
				sizeChar = CalcCharSize(bHandWrite, ColSegGroup);
			else
				sizeChar = CalcCharSize(bHandWrite, SegGroup);
			while (SecondMergeSegment(nDirection, bHandWrite, SegGroup, sizeChar));
			while (ThirdMergeSegment(nDirection, bHandWrite, SegGroup, sizeChar));
			RemoveInvalidSegment2(SegGroup);
		}
	}
	else
		SegGroup.clear();
#endif

#if 0
	// 세그먼트집합들의 통합(HJH)
	nColumnSize = ColSegGroup.size();
	for (i=0; i<nColumnSize; i++)
	{
		nSize = ColSegGroup.at(i).second.size();
		for (j=0; j<nSize; j++)
			SegGroup.push_back(ColSegGroup.at(i).second.at(j));
	}

	AdjustSegmentSize(image, nDirection, bHandWrite, SegGroup);

	// 세그먼트들의 최종합치기(HJH)
	while (FinalMergeSegment(bHandWrite, SegGroup));
#else
	// 세그먼트집합들의 통합(HJH)
	SegGroup.clear();

	nColumnSize = ColSegGroup.size();
	for (i=0; i<nColumnSize; i++)
	{
		nSize = ColSegGroup.at(i).second.size();
		for (j=0; j<nSize; j++)
			SegGroup.push_back(ColSegGroup.at(i).second.at(j));
	}
#endif

	// 세그먼트집합의 구성(HJH)
	ConstructResultSegGroup(nDirection, bHandWrite, SegGroup, SegmentList);

#if 1
	// 무효한 세그먼트들의 제거(HJH)
	RemoveInvalidSegment(bHandWrite, SegmentList);
#endif

	nSize = SegmentList.size();
	for (int i = 0; i < nSize; i++)
	{
		SegmentList.at(i).dwSequence = i + 1;
		pSegRes->segments.push_back(SegmentList.at(i).rChar);
	}

	cvtColor(image, pSegRes->imgSegments, CV_GRAY2BGR);

	Mat	imgSegments(pSegRes->imgSegments.rows, pSegRes->imgSegments.cols, CV_8UC3);

	imgSegments.setTo(255);

	for (i = 0; i < (int)ColGroup.size(); i++)
	{
		Point	p1 = ColGroup[i].tl(),
				p2 = ColGroup[i].br();

		p1.x = max(0, p1.x);
		p1.y = max(0, p1.y);
		p2.x = min(image.cols, p2.x) - 1;
		p2.y = min(image.rows, p2.y) - 1;
		rectangle(imgSegments, p1, p2, Scalar(0, 0, 255, 0));
	}

	for (i = 0; i < (int)SegmentList.size(); i++)
	{
		char	szNum[1000];
		Point	p1 = SegmentList[i].rChar.tl(),
				p2 = SegmentList[i].rChar.br();

		p1.x = max(0, p1.x);
		p1.y = max(0, p1.y);
		p2.x = min(image.cols, p2.x) - 1;
		p2.y = min(image.rows, p2.y) - 1;
		rectangle(imgSegments, p1, p2, Scalar(255, 0, 0, 0));

        //_itoa_s(i+1, szNum, 10);
        sprintf(szNum,"%d",i+1);//kjy-6.17
		putText(pSegRes->imgSegments,
			szNum, Point(SegmentList[i].rChar.x, SegmentList[i].rChar.y+12),
			FONT_HERSHEY_TRIPLEX, 0.5, Scalar(0, 64, 255, 0));
	}

	bitwise_and(imgSegments, pSegRes->imgSegments, pSegRes->imgSegments);
	//imgSegments.copyTo(pSegRes->imgSegments);

	return SegmentList.size();
}

int Segmentation(const Mat &image, PSEGMENT_RESULT pSegRes)
{
	if (image.empty())
	{
		return 0;
	}

	CleanNoise(image, pSegRes->imgNR);
	if (pSegRes->imgNR.cols%4)
	{
		pSegRes->imgNR(Rect(0, 0, pSegRes->imgNR.cols/4*4, pSegRes->imgNR.rows)).copyTo(pSegRes->imgNR);
	}

	Rect	rect(0, 0, pSegRes->imgNR.cols, pSegRes->imgNR.rows);

	FixSegmentSize(pSegRes->imgNR, rect);

	ProcCurveCorrection(pSegRes->imgNR);

	Mat	kernel1(3, 1, CV_8UC1);

	kernel1.setTo(1);
	erode(pSegRes->imgNR, pSegRes->imgNR, kernel1);

	if (rect.height < 64)
	{
		Mat	kernel2(3, 3, CV_8UC1);

		kernel2.setTo(1);

		resize(pSegRes->imgNR, pSegRes->imgNR, Size(pSegRes->imgNR.cols*2, pSegRes->imgNR.rows*2), 0.0, 0.0, INTER_NEAREST);
		erode(pSegRes->imgNR, pSegRes->imgNR, kernel2);
	}

	// Segmentation
	//int	x = GetSeparationX(pSegRes->imgNR, Rect(0, 0, pSegRes->imgNR.cols, pSegRes->imgNR.rows), TRUE);
	//pSegRes->imgNR(Rect(-x, 0, (pSegRes->imgNR.cols+x)/4*4, pSegRes->imgNR.rows)).copyTo(pSegRes->imgNR);
	return PICFrameSegmentation(pSegRes->imgNR, 1, FALSE, pSegRes);
}

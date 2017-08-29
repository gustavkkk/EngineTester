#include "stdafx.h"
#include "OCR.h"
#include "math.h"
#include <stdio.h>
#include <malloc.h>
#ifdef MSVC_OCR
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include "opencv2/highgui/highgui.hpp"

#define NORMALSIZE 64
#define FEATURESIZE 768
#define CANDNUM 10
#define DEGREE_CONST 0.05
//#define OCRLEARNING

typedef struct tagNEIGHBOURCOUNT
{
	BYTE Direct_1;
	BYTE Direct_2;
	BYTE Direct_3;
	BYTE Direct_4;
}NEIGHBOURCOUNT;

int g_nRecogCharNum = 64;
int g_nKindNum = 5;

extern BYTE *g_pLockSymDic;
extern BYTE *g_pDicData[DIC_COUNT];

WORD RecogChar[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, //0~9
					0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, //A~Z
					0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 
					0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
					0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, //a~z
					0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 
					0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
					0x3A, 0x2F}; //:, /

char AV_RecogChar[] = {-1, -1, -1, '3', '4', '5', '6', '7', '8', '9', //0~9
					'A', -1, 'C', -1, -1, -1, -1, 'H', -1, 'J', //A~Z
					'K', 'L', -1, 'N', 'O', 'P', -1, -1, -1, 'T', 
					'U', -1, -1, 'X', 'Y', 'Z',
					-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //a~z
					-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
					-1, -1, -1, -1, -1, -1,
					-1, -1}; //:, /

#ifdef MSVC_OCR
BOOL _LearningChar(WORD wCode, BYTE **ppDicData, float *pFeature, int nKind) 
{
	// TODO: Add extra validation here
	
	float fTemp;
	int i, CharIndex;

	for( i = 0; i < g_nRecogCharNum; i++ )
	{
		if( RecogChar[i] == wCode )
		{
			CharIndex = i;
			break;
		}
	}

	if( i == g_nRecogCharNum )
	{
		//AfxMessageBox("This Character cann't find");
		MessageBox(NULL,L"This Character cann't find", L"error", MB_OK);
  
		return FALSE;
	}

	BYTE *pTempDicData = ppDicData[nKind] + (2 * FEATURESIZE + 1) * 4 * CharIndex;
	
	int nLearningCount = *((int*)(pTempDicData));
	float *pDicFeature = (float*)(pTempDicData + 4);

	for( i = 0; i < FEATURESIZE; i++ )
	{
		//pDicFeature[i] = (nLearningCount * pDicFeature[i] + 2 * pFeature[i]) / (float)(nLearningCount + 2);
		pDicFeature[i] = (nLearningCount * pDicFeature[i] + pFeature[i]) / (float)(nLearningCount + 1);
		fTemp = pDicFeature[i] - pFeature[i];
		fTemp *= fTemp;
		fTemp *= fTemp;
		//pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + (float)(2 * fTemp)) / (float)(nLearningCount + 2);
		pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + fTemp) / (float)(nLearningCount + 1);
	}
	
	nLearningCount += 1;
	*((int*)(pTempDicData)) = nLearningCount;

	
	int fh, nSize;
	TCHAR	ExePath[MAX_PATH];
	TCHAR	DicPath[MAX_PATH];
	TCHAR	tTemp[MAX_PATH];

	unsigned char *pTemp;
		
	GetModuleFileName(NULL, ExePath, MAX_PATH);
	ExePath[_tcsrchr(ExePath, '\\') - ExePath] = 0;

	wcscpy(tTemp, ExePath);
	wcscat(tTemp, L"\\DIC\\sz_ocr_" );
	wsprintf( DicPath, L"%s%d", tTemp, nKind);
	lstrcatW( DicPath, L".dic" );

	fh = _wopen(DicPath, _O_RDONLY | _O_BINARY, _S_IREAD);
	if( fh != -1 )
	{
		nSize = _filelength(fh);
		pTemp = (unsigned char*)malloc(nSize * sizeof(unsigned char));
		_read(fh, pTemp, nSize);
		_close(fh);

		wcscpy(tTemp, DicPath);
		wcscat(tTemp, L".bak" );
		
		fh = _wopen(tTemp , _O_CREAT | _O_WRONLY | _O_BINARY, _S_IWRITE);
		if( fh != -1 )
		{
			_write(fh, pTemp, nSize);
			_close(fh);
		}
	}

	fh = _wopen(DicPath, _O_CREAT | _O_WRONLY | _O_BINARY, _S_IWRITE);
	if( fh != -1 )
	{
		_write(fh, ppDicData[nKind], nSize);
		_close(fh);
	}
	else
	{
		MessageBox(NULL, L"Can't Open File", L"error", MB_OK);
		return FALSE;
	}
	
	return TRUE;
}

#if 0
void _LinerNormalization(BYTE *pCharImgData, int nHeight, int nWidth, int nBytesPerLine, BYTE *pNormalData)
{
	int x, y;
	int i, j;

	BYTE Mask[] = { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };
	
	double x_ratio = (double)(nWidth - 1) / (double)NORMALSIZE; 
	double y_ratio = (double)(nHeight - 1) / (double)NORMALSIZE;
	
	DWORD n;
	int MulTable_X[NORMALSIZE];
	int MulTable_Y[NORMALSIZE];
	
	for( i = 0; i < NORMALSIZE; i++)
	{
		MulTable_X[i] = (int)(x_ratio * (double)i + 0.5);
		MulTable_Y[i] = (int)(y_ratio * (double)i + 0.5);
	}

	for( i = 0, n = NORMALSIZE + 2; i < NORMALSIZE; i++, n += (NORMALSIZE + 2))
	{
		if( i < NORMALSIZE - 1)
			y = min(nHeight - 2, MulTable_Y[i]);
		else
			y = nHeight - 2;
		for( j = 0; j < NORMALSIZE; j++)
		{
			if( j < NORMALSIZE - 1)
				x = min(nWidth - 2, MulTable_X[j]);
			else
				x = nWidth - 2;
			if(pCharImgData[nBytesPerLine * y + (x >> 3)] & Mask[ x & 0x07 ])
				pNormalData[ n + j + 1 ] = 1;
		}
	}
}
#endif

void _LinerNormalization(BYTE *pCharImgData, int nHeight, int nWidth, BYTE *pNormalData)
{
	int x, y;
	int i, j;

	double x_ratio = (double)(nWidth - 1) / (double)NORMALSIZE; 
	double y_ratio = (double)(nHeight - 1) / (double)NORMALSIZE;
	
	DWORD n;
	int MulTable_X[NORMALSIZE];
	int MulTable_Y[NORMALSIZE];
	
	for( i = 0; i < NORMALSIZE; i++)
	{
		MulTable_X[i] = (int)(x_ratio * (double)i + 0.5);
		MulTable_Y[i] = (int)(y_ratio * (double)i + 0.5);
	}

	for( i = 0, n = NORMALSIZE + 2; i < NORMALSIZE; i++, n += (NORMALSIZE + 2))
	{
		if( i < NORMALSIZE - 1)
			y = min(nHeight - 2, MulTable_Y[i]);
		else
			y = nHeight - 2;
		for( j = 0; j < NORMALSIZE; j++)
		{
			if( j < NORMALSIZE - 1)
				x = min(nWidth - 2, MulTable_X[j]);
			else
				x = nWidth - 2;
			if(pCharImgData[nWidth * y + x] == 0)
				pNormalData[ n + j + 1 ] = 1;
		}
	}
}

void _Smoothing( BYTE *pNormalData )
{
	int i, j;
	DWORD n;
	int sum;
	
	BYTE bTemp[(NORMALSIZE + 2) * (NORMALSIZE + 2)];
	memcpy(bTemp, pNormalData, (NORMALSIZE + 2) * (NORMALSIZE + 2));
	
	for( i = 0, n = NORMALSIZE + 2; i < NORMALSIZE; i++, n += (NORMALSIZE + 2) )
	{
		for( j = 1; j < NORMALSIZE + 1; j++ )
		{
			sum = bTemp[n - (NORMALSIZE + 2) + j - 1] + bTemp[n - (NORMALSIZE + 2) + j] + bTemp[n - (NORMALSIZE + 2) + j + 1] +
				  bTemp[n + j - 1]                    + bTemp[n + j]                    + bTemp[n + j + 1]      +
				  bTemp[n + (NORMALSIZE + 2) + j - 1] + bTemp[n + 66 + j] + bTemp[n + (NORMALSIZE + 2) + j + 1];
			
			if( pNormalData[n + j] )
			{
				if( sum < 3 )
					pNormalData[n + j] = 0;
			}
			else if ( sum >= 4)
				pNormalData[n + j] = 1;
		}
	}
}

void _GetContourNeighbour(BYTE *ia, int cxDIB,int cyDIB, BYTE *pNeighbourCount)
{
	DWORD m;
	int i,j,ds,nc;
	
	static char ip[] = { 0,-1,-1,-1,0,1,1,1 };
	static char jp[] = { 1,1,0,-1,-1,-1,0,1 };
	static char direct[] = { 0,1,2,3,0,1,2,3 };
	int ii,jj,iw,jw,nb,n;
	
	POINT PrePoint;
	for( i = 1, m = cxDIB + 2; i < cyDIB + 1; i++, m += (cxDIB + 2))
	{
		for( j = 1; j < cxDIB + 1; j++){
			if(ia[ m + j ] == 1){
				nc = ia[ m + j + 1 ] & ia[ m - cxDIB - 2 + j] & ia[ m + j - 1] & ia[ m + cxDIB + 2 + j ];
				if(nc == 0){
					if(ia[ m + cxDIB + 2 + j ] == 0)
						ds = 6;
					else if(ia[ m + j - 1 ] == 0)
						ds = 4;
					else if(ia[ m - cxDIB - 2 + j ] == 0)
						ds = 2;
					else if(ia[ m + j + 1 ] == 0)
						ds = 0;
					ii = iw = i; jj = jw = j;
					n = ( ds + 4 ) & 7;
					ds = -1;
					nb = -1;
					nc = ia[m - cxDIB - 2 + j - 1] + ia[m - cxDIB - 2 + j] + ia[m - cxDIB - 2 + j + 1] +
						 ia[m + j - 1] + ia[m + j + 1] +
						 ia[m + cxDIB + 2 + j - 1] + ia[m + cxDIB + 2 + j] + ia[m + cxDIB + 2 + j + 1];
					
					if(nc > 0){
						do{
							ii = iw;
							jj = jw;
							ia[iw * (cxDIB + 2) + jw] = 3;
							n = ( n + 4 ) & 7;
							do{
								n = ( n + 1 ) & 7;
								iw = ii + ip[n];
								jw = jj + jp[n];
							}while(ia[iw * (cxDIB + 2) + jw] == 0);
							
							if( nb >= 0 )
							{
								pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + direct[n]]++;
								if( PrePoint.x == jj )
									pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 2]++;
								else if( PrePoint.y == ii )
									pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 0]++;
								else
								{
									if( (PrePoint.x - jj + PrePoint.y - ii) == 0 )
										pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 1]++;
									else
										pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 3]++;
								}
							}
							PrePoint.x = jj;
							PrePoint.y = ii;
							if(ds < 0)
							{
								if(nb < 0) nb = n;
								else ds = nb;
							}
						}while((ii!=i)||(jj!=j)||(n!=ds));
					}
					else
						ia[m + j] = 2;
				}
			}
		}
	}

	for( i = 0; i < (cyDIB + 2) * (cxDIB + 2); i++)
	{
		if(ia[i] == 3)
			ia[i] = 1;
	}
}

void _GetPeripheral(BYTE* pNormalData, BYTE* pPeripheral)
{
	int i,j;
	DWORD n = NORMALSIZE + 2;

	for( i = 1; i < NORMALSIZE + 1; i++, n += (NORMALSIZE + 2) )
	{
		for( j = 1; j < NORMALSIZE + 1; j++)
		{
			if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j ] == 1 )
				pPeripheral[ (i - 1) * NORMALSIZE + j - 1 ] += 1;
			if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j ] == 1 )
				pPeripheral[ (i - 1) * NORMALSIZE + j - 1 ] += 1;
			
			if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j + 1 ] == 1 )
				pPeripheral[ (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
			if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j - 1 ] == 1 )
				pPeripheral[ (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
			
			if( pNormalData[ n + j ] - pNormalData[ n + j - 1 ] == 1 )
				pPeripheral[ 2 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
			if( pNormalData[ n + j ] - pNormalData[ n + j + 1 ] == 1 )
				pPeripheral[ 2 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
			
			if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j - 1 ] == 1 )
				pPeripheral[ 3 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
			if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j + 1 ] == 1 )
				pPeripheral[ 3 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
		}
	}
}

void _GetFeature_Of_EndClassification( BYTE *pNormalData, float *pFeature )
{
	int depth;
	int i, j, k, m, total;
	int c1, c2, c3, c4;
	int start, end;
	double f1, f2, f3, f4;
	DWORD n, nn;
	
	BYTE *pPeripheral = (BYTE*)malloc(NORMALSIZE * NORMALSIZE * 4);
	memset(pPeripheral, 0, NORMALSIZE * NORMALSIZE * 4);

	
	NEIGHBOURCOUNT *pNeighbourCount = (NEIGHBOURCOUNT*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2) * sizeof(NEIGHBOURCOUNT));
	memset(pNeighbourCount, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2) * sizeof(NEIGHBOURCOUNT));
	
	_GetPeripheral(pNormalData, pPeripheral);
	_GetContourNeighbour((BYTE*)pNormalData, NORMALSIZE, NORMALSIZE, (BYTE*)pNeighbourCount);
	
	for( depth = 0, nn = 0; depth < 3; depth++, nn += (NORMALSIZE * 4) )
	{
		for( int direct = 0; direct < 8; direct++ )
		{
			f1 = f2 = f3 = f4 = 0.0;
			
			start = direct * 8;
			end = start + 8;
			for( i = start; i < end; i++)
			{
				c1 = c2 = c3 = c4 = 0;
				for( j = 0, n = 0; j < NORMALSIZE; j++, n += NORMALSIZE)
				{
					if(pPeripheral[n + i])
					{
						for( k = 0; k < 3; k++ )
						{
							for( m = 0; m < 3; m++ )
							{
								c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_1;
								c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_2;
								c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_3;
								c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_4;
							}
						}
						
						total = c1 + c2 + c3 + c4;

						if(total != 0)
						{
							f1 += (((double)c1) / total);
							f2 += (((double)c2) / total);
							f3 += (((double)c3) / total);
							f4 += (((double)c4) / total);
						}

						pPeripheral[n + i]--;

						break;
					}
				}
			}
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			
			pFeature[0 + start + nn] = (float)f1;
			pFeature[1 + start + nn] = (float)f2;
			pFeature[2 + start + nn] = (float)f3;
			pFeature[3 + start + nn] = (float)f4;
			
			f1 = f2 = f3 = f4 = 0.0;
			for( i = start; i < end; i++)
			{
				c1 = c2 = c3 = c4 = 0;
				for( j = (NORMALSIZE - 1), n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0; j--, n -= NORMALSIZE)
				{
					if(pPeripheral[n + i])
					{
						for( k = 0; k < 3; k++ )
						{
							for( m = 0; m < 3; m++ )
							{
								c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_1;
								c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_2;
								c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_3;
								c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_4;
							}
						}
						
						total = c1 + c2 + c3 + c4;

						if(total != 0)
						{
							f1 += (((double)c1) / total);
							f2 += (((double)c2) / total);
							f3 += (((double)c3) / total);
							f4 += (((double)c4) / total);
						}

						pPeripheral[n + i]--;

						break;
					}
				}
			}
			
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			
			pFeature[0 + start + 4 + nn] = (float)f1;
			pFeature[1 + start + 4 + nn]=  (float)f2;
			pFeature[2 + start + 4 + nn] = (float)f3;
			pFeature[3 + start + 4 + nn] = (float)f4;
		
			f1 = f2 = f3 = f4 = 0.0;
			for( i = start, n = NORMALSIZE * start; i < end; i++, n += NORMALSIZE)
			{
				c1 = c2 = c3 = c4 = 0;
				for( j = 0; j < NORMALSIZE; j++ )
				{
					if(pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)])
					{
						for( k = 0; k < 3; k++ )
						{
							for( m = 0; m < 3; m++ )
							{
								c1 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_1;
								c2 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_2;
								c3 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_3;
								c4 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_4;
							}
						}

						total = c1 + c2 + c3 + c4;
						
						if(total != 0)
						{
							f1 += (((double)c1) / total);
							f2 += (((double)c2) / total);
							f3 += (((double)c3) / total);
							f4 += (((double)c4) / total);
						}

						pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)]--;

						break;
					}
				}
			}
			
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + NORMALSIZE + nn] = (float)f1;
			pFeature[1 + start + NORMALSIZE + nn]=  (float)f2;
			pFeature[2 + start + NORMALSIZE + nn] = (float)f3;
			pFeature[3 + start + NORMALSIZE + nn] = (float)f4;
			
			f1 = f2 = f3 = f4 = 0.0;
			for( i = start, n = NORMALSIZE * start; i < end; i++, n += NORMALSIZE)
			{
				c1 = c2 = c3 = c4 = 0;
				for( j = (NORMALSIZE - 1); j >= 0; j-- )
				{
					if(pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)])
					{
						for( k = 0; k < 3; k++ )
						{
							for( m = 0; m < 3; m++ )
							{
								c1 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_1;
								c2 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_2;
								c3 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_3;
								c4 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_4;
							}
						}

						total = c1 + c2 + c3 + c4;
						
						if(total != 0)
						{
							f1 += (((double)c1) / total);
							f2 += (((double)c2) / total);
							f3 += (((double)c3) / total);
							f4 += (((double)c4) / total);
						}

						pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)]--;

						break;
					}
				}
			}
			
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + NORMALSIZE + 4 + nn] = (float)f1;
			pFeature[1 + start + NORMALSIZE + 4 + nn]=  (float)f2;
			pFeature[2 + start + NORMALSIZE + 4 + nn] = (float)f3;
			pFeature[3 + start + NORMALSIZE + 4 + nn] = (float)f4;
			
			int l;
			int start1,end1;
			
			f1 = f2 = f3 = f4 = 0.0;
			if( (NORMALSIZE - 1) - 2 * start > 0)
			{
				for( i = (NORMALSIZE - 1) - 2 * start; i > (NORMALSIZE - 1) - 2 * end; i--)
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = i, k = 0, n = i * NORMALSIZE; j < NORMALSIZE && k < NORMALSIZE; j++, k++, n += NORMALSIZE)
					{
						if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			else
			{
				start1 = 2 * start - (NORMALSIZE - 1);
				end1 = start1 + 16;
				for( i = start1; i < end1; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = 0, k = i, n = 0; j < NORMALSIZE && k < NORMALSIZE; j++, k++, n += NORMALSIZE )
					{
						if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + 128 + nn] = (float)f1;
			pFeature[1 + start + 128 + nn]=  (float)f2;
			pFeature[2 + start + 128 + nn] = (float)f3;
			pFeature[3 + start + 128 + nn] = (float)f4;
			
			f1 = f2 = f3 = f4 = 0.0;
			if( 2 * start < NORMALSIZE )
			{
				for( i = 2 * start; i < 2 * start + 16; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = (NORMALSIZE - 1), k = i, n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0 && k >= 0; j--, k--, n -= NORMALSIZE)
					{
						if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			else
			{
				start1 = 128 - 2*(start+1);
				end1 = start1 - 16;
				for( i = start1; i > end1; i-- )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = i, k = (NORMALSIZE - 1), n = i * NORMALSIZE; j >= 0 && k >= 0; j--, k--, n -= NORMALSIZE )
					{
						if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + 128 + 4 + nn] = (float)f1;
			pFeature[1 + start + 128 + 4 + nn]=  (float)f2;
			pFeature[2 + start + 128 + 4 + nn] = (float)f3;
			pFeature[3 + start + 128 + 4 + nn] = (float)f4;

			f1 = f2 = f3 = f4 = 0.0;
			if( 2 * start < NORMALSIZE )
			{
				for( i = 2 * start; i < 2 * start + 16; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = i, k = 0, n = i * NORMALSIZE; j >= 0 && k < NORMALSIZE; j--, k++, n -= NORMALSIZE )
					{
						if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			else
			{
				start1 = 2 * start - (NORMALSIZE - 1);
				end1 = start1 + 16;
				for( i = start1; i < end1; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = (NORMALSIZE - 1), k = i, n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0 && k < NORMALSIZE; j--, k++, n -= NORMALSIZE )
					{
						if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + 192 + nn] = (float)f1;
			pFeature[1 + start + 192 + nn]=  (float)f2;
			pFeature[2 + start + 192 + nn] = (float)f3;
			pFeature[3 + start + 192 + nn] = (float)f4;

			f1 = f2 = f3 = f4 = 0.0;
			if(2 * start < NORMALSIZE )
			{
				for( i = 2 * start; i < 2 * start + 16; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = 0, k = i, n = 0; j < NORMALSIZE && k >= 0; j++, k--, n += NORMALSIZE )
					{
						if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			else
			{
				start1 = 2 * start - (NORMALSIZE - 1);
				end1 = start1 + 16;
				for( i = start1; i < end1; i++ )
				{
					c1 = c2 = c3 = c4 = 0;
					for( j = i, k = (NORMALSIZE - 1), n = i * NORMALSIZE; j < NORMALSIZE && k >= 0; j++, k--, n += NORMALSIZE )
					{
						if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
						{
							for( l = 0; l < 3; l++ )
							{
								for( m = 0; m < 3; m++ )
								{
									c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
									c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
									c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
									c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
								}
							}

							total = c1 + c2 + c3 + c4;
							
							if(total != 0)
							{
								f1 += (((double)c1) / total);
								f2 += (((double)c2) / total);
								f3 += (((double)c3) / total);
								f4 += (((double)c4) / total);
							}

							pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

							break;
						}
					}
				}
			}
			f1 *= 0.125;
			f2 *= 0.125;
			f3 *= 0.125;
			f4 *= 0.125;
			pFeature[0 + start + 192 + 4 + nn] = (float)f1;
			pFeature[1 + start + 192 + 4 + nn]=  (float)f2;
			pFeature[2 + start + 192 + 4 + nn] = (float)f3;
			pFeature[3 + start + 192 + 4 + nn] = (float)f4;
		}
	}
	
	free(pPeripheral);
	free(pNeighbourCount);
}

void _EndClassification(float *pFeature, BYTE **ppDicData, WORD *pCode, RECOG_DEGREE *pRecog_Degree)
{
	int i, j, k;
	int nLearningCount;
	double r0, r1, r2;
	float *pDicFeature;
	RECOG_DEGREE *pTempDegree1;
	RECOG_DEGREE *pTempDegree2;
	int *pTempIndex;
	
	pTempDegree1 = (RECOG_DEGREE*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));
	memset(pTempDegree1, 0, g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));
	
	pTempDegree2 = (RECOG_DEGREE*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));
	memset(pTempDegree2, 0, g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));

	pTempIndex = (int*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(int));
	memset(pTempIndex, 0, g_nKindNum * g_nRecogCharNum * sizeof(int));

	for( k = 0; k < g_nKindNum; k++ )
	{
		for( i = 0; i < g_nRecogCharNum; i++ )
		{
			if( AV_RecogChar[i] < 0 )
				continue;

			r0 = 0.0;
			r1 = 0.0;
			r2 = 0.0;

			pTempIndex[i + k * g_nRecogCharNum] = i;
			pTempDegree1[i + k * g_nRecogCharNum].nKind = k;
			pTempDegree2[i + k * g_nRecogCharNum].nKind = k;
		
			pDicFeature = (float*)(ppDicData[k] + (2 * FEATURESIZE + 1) * 4 * i + 4);
			nLearningCount = *((int*)(pDicFeature-1));

			if(nLearningCount != 0)
			{
				for( j = 0; j < FEATURESIZE; j++ )
				{
					if( pDicFeature[j] < 0 )
						//AfxMessageBox("Minus1");
						MessageBox(NULL,L"Minus1", L"error", MB_OK);
					if( pFeature[j] < 0 )
						//AfxMessageBox("Minus2");
						MessageBox(NULL,L"Minus2", L"error", MB_OK);
			
					pTempDegree1[i + k * g_nRecogCharNum].fDegree += (float)((pDicFeature[j] - pFeature[j]) * (pDicFeature[j] - pFeature[j]) / ( DEGREE_CONST + pDicFeature[j + FEATURESIZE]));
			
					r0 += (pDicFeature[j] * pFeature[j]);
					r1 += (pDicFeature[j] * pDicFeature[j]);
					r2 += (pFeature[j] * pFeature[j]);
				}

				if( r1 == 0.0 || r2 == 0.0 )
					pTempDegree2[i + k * g_nRecogCharNum].fDegree = 0.0;
				else
					pTempDegree2[i + k * g_nRecogCharNum].fDegree = (float)(r0 / (sqrt(r1) *sqrt(r2)));
			}
			else
			{
				pTempDegree1[i + k * g_nRecogCharNum].fDegree = 1000000.0;
				pTempDegree2[i + k * g_nRecogCharNum].fDegree = 0.0;
			}
		}
	}

	for( i = 0; i < g_nKindNum * g_nRecogCharNum; i++ )
	{
		for( j = i + 1; j < g_nKindNum * g_nRecogCharNum; j++ )
		{
			if( pTempDegree2[i].fDegree < pTempDegree2[j].fDegree )
			//if( pTempDegree1[i].fDegree > pTempDegree1[j].fDegree )
			{
				float fTemp = pTempDegree1[i].fDegree;
				int nTemp = pTempDegree1[i].nKind;

				pTempDegree1[i].fDegree = pTempDegree1[j].fDegree;
				pTempDegree1[i].nKind = pTempDegree1[j].nKind;
				pTempDegree1[j].fDegree = fTemp;
				pTempDegree1[j].nKind = nTemp;
				
				fTemp = pTempDegree2[i].fDegree;
				nTemp = pTempDegree2[i].nKind;
				pTempDegree2[i].fDegree = pTempDegree2[j].fDegree;
				pTempDegree2[i].nKind = pTempDegree2[j].nKind;
				pTempDegree2[j].fDegree = fTemp;
				pTempDegree2[j].nKind = nTemp;
				
				nTemp = pTempIndex[i];
				pTempIndex[i] = pTempIndex[j];
				pTempIndex[j] = nTemp;
			}
		}
	}

	for(i = 0; i < CANDNUM; i++)
		pCode[i] = RecogChar[pTempIndex[i]];
	
	memcpy(pRecog_Degree, pTempDegree2, CANDNUM * sizeof(RECOG_DEGREE));
	
	free(pTempDegree1);
	free(pTempDegree2);
	free(pTempIndex);
}

void _GetHistogram_H(BYTE* pCharImagData, int nHeight, int nWidth, int *pHist)
{
	int i, j, n;

	for( i = 0; i < nWidth; i++)
	{
		for(j = 0, n = 0; j < nHeight; j++, n += nWidth)
		{
			if(pCharImagData[i + n] == 0)
				pHist[i]++;
		}
	}
}

void _GetHistogram_V(BYTE* pCharImgData, int nHeight, int nWidth, int *pHist)
{
	int i, j, n;

	for( i = 0, n = 0; i < nHeight; i++, n += nWidth)
	{
		for(j = 0; j < nWidth; j++)
		{
			if(pCharImgData[n + j] == 0)
				pHist[i]++;
		}
	}
}

BOOL _CheckSemicolon(BYTE* pCharImgData, int nHeight, int nWidth)
{
	BOOL blRet = FALSE;

	int *pHist = (int*)malloc(nHeight * sizeof(int));
	memset(pHist, 0, nHeight * sizeof(int));

	_GetHistogram_V(pCharImgData, nHeight, nWidth, pHist);
	
	BOOL flag = FALSE;
	int i, nCount;

	int nCheck = (int)(nWidth * 0.9);

	for( i = 0; i < nHeight; i++)
	{
		if(pHist[i] == 0)
		{
			if(flag == FALSE)
			{
				flag = TRUE;
				nCount = 1;
			}
			else
				nCount++;
		}
		else
		{
			if(flag == TRUE)
			{
				flag = FALSE;
				if(nCount >= nCheck)
				{
					blRet = TRUE;
					break;
				}
			}
		}
	}

	free(pHist);

	return blRet;
}

void _Check_Split_Char(BYTE* pCharImgData, int nHeight, int nWidth, RECT *pRect)
{
	int *pHist = (int*)malloc(nWidth * sizeof(int));
	memset(pHist, 0, nWidth * sizeof(int));

	_GetHistogram_H(pCharImgData, nHeight, nWidth, pHist);

	int i;
	BOOL blRet = FALSE;
	BOOL blFlag = FALSE;
	
	for( i = 0; i < nWidth; i++)
	{
		if(pHist[i] < 2)
		{
			if(blFlag == FALSE)
			{
				pRect->right = i;
				blFlag = TRUE;
			}
		}
		else
		{
			if(blFlag == TRUE)
			{
				pRect->left = i;
				blRet = TRUE;
				break;
			}
		}
	}

	if( blRet == FALSE )
	{
		int split_x;
		int temp = nHeight + 1;
		int left = max(0, (nWidth / 2 - 5));
		int right = min(nWidth, (nWidth / 2 + 5));

		for( i = left; i < right; i++)
		{
			if( pHist[i] < temp )
			{
				temp = pHist[i];
				split_x = i;
			}
		}
		pRect->right = max(0, split_x - 1);
		pRect->left = min(nWidth, split_x + 1);
	}

	free(pHist);
}

void _GetTopBottom(int *pHist, int nHeight, RECT *pRect)
{
	int i;

	pRect->top = 0;
	pRect->bottom = nHeight - 1;

	for( i = 0; i < nHeight; i++)
	{
		if(pHist[i] != 0)
		{
			pRect->top = i;
			break;
		}
	}
	
	for(i = nHeight - 1; i > 0; i--)
	{
		if(pHist[i] != 0)
		{
			pRect->bottom = i;
			break;
		}
	}
}

#ifdef OCRLEARNING
static int sCount = 0;
#endif

int RecognitionChar(BYTE **ppDicData, BYTE *pCharImgData, int nHeight, int nWidth, vector<WORD> &codes, vector<float> &degrees)
{
	int nRet;
	BYTE *pNormalData;
	float *pFeature;
	
	WORD wCode_0[CANDNUM];
	//WORD wCode_1[CANDNUM];
	//WORD wCode_2[CANDNUM];
	
	RECOG_DEGREE Recog_Degree_0[CANDNUM];
	//RECOG_DEGREE Recog_Degree_1[CANDNUM];
	//RECOG_DEGREE Recog_Degree_2[CANDNUM];

	pNormalData = (BYTE*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
	memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

	pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

	_LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

	_Smoothing(pNormalData);
#ifdef OCR_TEST
	Mat test(nHeight,nWidth,CV_8UC1);
	test.data = pCharImgData;
	imshow("test",test);
	waitKey(1000);
#endif
#if 0
	Mat m(66, 66, CV_8UC1);
	for(int i = 0; i < 66 * 66; i++)
		m.data[i] = pNormalData[i] == 0 ? 255 : 0;
	//memcpy(m.data, pNormalData, 66*66);
	imshow("aaa", m);waitKey(0);
#endif

	_GetFeature_Of_EndClassification( pNormalData, pFeature );
	
	_EndClassification(pFeature, ppDicData, wCode_0, Recog_Degree_0);

	float ratio1 = (float)nHeight / nWidth;
	float ratio2 = (float)nWidth / nHeight;

	if((wCode_0[0] == 0x4f || wCode_0[0] == 0x6f) && ratio2 < 0.8)
		wCode_0[0] = 0x30;
	if(wCode_0[0] == 0x31 || wCode_0[0] == 0x49 || wCode_0[0] == 0x6C)
	{
		if( (nHeight < 13 && nWidth < 13) ||
			(ratio1 < 1.5 && Recog_Degree_0[0].fDegree < 0.8))
			wCode_0[0] = 0x2e;
	}
	
	if(wCode_0[0] == 0x3A && Recog_Degree_0[0].fDegree < 0.9)
	{
		if(_CheckSemicolon(pCharImgData, nHeight, nWidth) == FALSE)
			wCode_0[0] = 0x31;
	}

	codes.clear();
	for (int i = 0; i < CANDNUM; i++)
	{
		codes.push_back(wCode_0[i]);
	}

	nRet = CANDNUM;

	BOOL blFlag = TRUE;
	int i, nCount;
	float fTemp = Recog_Degree_0[0].fDegree;
	for( i = 1, nCount = 1; i < CANDNUM; i++)
	{
		if(wCode_0[i] == nRet)
		{
			nCount++;
			fTemp += Recog_Degree_0[i].fDegree;
		}
	}
	
	if(nCount > 3 )
	{
		fTemp /= nCount;
		if( fTemp > 0.75 )
			blFlag = FALSE;
	}

#if 0
	if( blFlag && Recog_Degree_0[0].fDegree < 0.85 && wCode_0[0] != 0x2f )
	{
		if(ratio2 > 0.68)
		{
			RECT rect;
			_Check_Split_Char(pCharImgData, nHeight, nWidth, &rect);
			//{
				int *pHist;
				int j, n0, n1;
				int nHeight_1, nHeight_2 = nHeight;
				int nWidth_1 = rect.right;
				int nWidth_2 = nWidth - rect.left;

				nHeight_1 = nHeight_2 = nHeight;

				pHist = (int*)malloc(nHeight * sizeof(int));

				BYTE *pCharImgData_1 = (BYTE*)malloc(nHeight * nWidth_1);
				BYTE *pCharImgData_2 = (BYTE*)malloc(nHeight * nWidth_2);
				
				for( i = 0, n0 = 0, n1 = 0; i < nHeight; i++, n0 +=  nWidth_1, n1 += nWidth)
				{
					for(j = 0; j < nWidth_1; j++)
					{
						pCharImgData_1[n0 + j] = pCharImgData[n1 + j];
					}
				}
				
				for( i = 0, n0 = 0, n1 = 0; i < nHeight; i++, n0 +=  nWidth_2, n1 += nWidth)
				{
					for(j = 0; j < nWidth_2; j++)
					{
						pCharImgData_2[n0 + j] = pCharImgData[n1 + j + rect.left];
					}
				}

#if 0
				Mat m3(nHeight, nWidth_1, CV_8UC1);
				//for(i = 0; i < 66 * 66; i++)
					//m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
				memcpy(m3.data, pCharImgData_1, nHeight * nWidth_1);
				imshow("aaa3", m3);waitKey(0);
#endif

				memset(pHist, 0, nHeight * sizeof(int));
				_GetHistogram_V(pCharImgData_1, nHeight, nWidth_1, pHist);
				_GetTopBottom(pHist, nHeight, &rect);

				if( rect.top != 0 )
				{
					memcpy(pCharImgData, pCharImgData_1, nHeight * nWidth_1);
					nHeight_1 = rect.bottom - rect.top + 1;
					memcpy(pCharImgData_1, pCharImgData + rect.top * nWidth_1, nHeight_1 * nWidth_1);
				}

				memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));
				_LinerNormalization(pCharImgData_1, nHeight_1, nWidth_1, pNormalData);
				_Smoothing(pNormalData);
				
#if 0
				Mat m1(66, 66, CV_8UC1);
				for(i = 0; i < 66 * 66; i++)
					m1.data[i] = pNormalData[i] == 0 ? 255 : 0;
				//memcpy(m1.data, pCharImgData_1, nHeight_1 * nWidth_1);
				imshow("aaa0", m1);waitKey(0);
#endif

				_GetFeature_Of_EndClassification( pNormalData, pFeature );
				_EndClassification(pFeature, ppDicData, wCode_1, Recog_Degree_1);

				ratio2 = (float)nWidth_1 / nHeight_1;
				if(ratio2 > 1.5 && (wCode_1[0] == 0x31 || wCode_1[0] == 0x49 || wCode_1[0] == 0x6c))
				{
					wCode_1[0] = 0x2D;
				}

#if 0
				Mat m4(nHeight, nWidth_2, CV_8UC1);
				//for(i = 0; i < 66 * 66; i++)
					//m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
				memcpy(m4.data, pCharImgData_2, nHeight * nWidth_2);
				imshow("aaa4", m4);waitKey(0);
#endif

				memset(pHist, 0, nHeight * sizeof(int));
				_GetHistogram_V(pCharImgData_2, nHeight, nWidth_2, pHist);
				_GetTopBottom(pHist, nHeight, &rect);

				if( rect.top != 0 )
				{
					memcpy(pCharImgData, pCharImgData_2, nHeight * nWidth_2);
					nHeight_2 = rect.bottom - rect.top + 1;
					memcpy(pCharImgData_2, pCharImgData + rect.top * nWidth_2, nHeight_2 * nWidth_2);
				}
				
				memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));
				_LinerNormalization(pCharImgData_2, nHeight_2, nWidth_2, pNormalData);
				_Smoothing(pNormalData);
				
#if 0
				Mat m2(66, 66, CV_8UC1);
				for(i = 0; i < 66 * 66; i++)
					m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
				//memcpy(m2.data, pCharImgData_2, nHeight * nWidth_2);
				imshow("aaa1", m2);waitKey(0);
#endif
				_GetFeature_Of_EndClassification( pNormalData, pFeature );
				_EndClassification(pFeature, ppDicData, wCode_2, Recog_Degree_2);

				ratio2 = (float)nWidth_2 / nHeight_2;
				if(ratio2 > 1.5 && (wCode_2[0] == 0x31 || wCode_2[0] == 0x49 || wCode_2[0] == 0x6c))
				{
					wCode_2[0] = 0x2D;
				}

				if((Recog_Degree_1[0].fDegree > 0.8 && Recog_Degree_2[0].fDegree > 0.8) && 
					((Recog_Degree_1[0].fDegree + Recog_Degree_2[0].fDegree) / 2.0 > Recog_Degree_0[0].fDegree))
				{
					wRet = (wCode_2[0] << 8) | wCode_1[0];
				}

				free(pHist);
				free(pCharImgData_1);
				free(pCharImgData_2);
			//}
		}
	}
#endif

	free(pNormalData);
	free(pFeature);

	degrees.clear();
	for (int i = 0; i < CANDNUM; i++)
	{
		degrees.push_back(Recog_Degree_0[i].fDegree);
	}

	return nRet;
}

BOOL CheckLockSymbol_Revised(BYTE **ppDicData, BYTE *pCharImgData, int nHeight, int nWidth)
{
	vector<WORD>	codes;
	vector<float>	degrees;
	int				cnt;

	if (nHeight <= 0 || nWidth <= 0)
		return FALSE;

	cnt = RecognitionChar(ppDicData,pCharImgData,nHeight,nWidth,codes,degrees);

	for (int i = 0; i < cnt; i++)
	{
		if (*(char *)&codes[i] == 'O' && degrees[i] > 0.837)
		{
			return TRUE;
		}
	}

	return FALSE;
}
#define LOCK_SYMBOL_KIND 30

BOOL _LearningLockSymbol(BYTE *pDicData, float *pFeature, int nKind) 
{
	// TODO: Add extra validation here
	
	int i;
	float fTemp;

	BYTE *pTempDicData = pDicData + (2 * FEATURESIZE + 1) * 4 * nKind;
	
	int nLearningCount = *((int*)(pTempDicData));
	float *pDicFeature = (float*)(pTempDicData + 4);

	for( i = 0; i < FEATURESIZE; i++ )
	{
		//pDicFeature[i] = (nLearningCount * pDicFeature[i] + 2 * pFeature[i]) / (float)(nLearningCount + 2);
		pDicFeature[i] = (nLearningCount * pDicFeature[i] + pFeature[i]) / (float)(nLearningCount + 1);
		fTemp = pDicFeature[i] - pFeature[i];
		fTemp *= fTemp;
		fTemp *= fTemp;
		//pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + (float)(2 * fTemp)) / (float)(nLearningCount + 2);
		pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + fTemp) / (float)(nLearningCount + 1);
	}
	
	nLearningCount += 1;
	*((int*)(pTempDicData)) = nLearningCount;

	int fh, nSize;
	TCHAR	ExePath[MAX_PATH];
	TCHAR	DicPath[MAX_PATH];
	unsigned char *pTemp;
		
	GetModuleFileName(NULL, ExePath, MAX_PATH);
	ExePath[_tcsrchr(ExePath, '\\') - ExePath] = 0;

	wcscpy(DicPath, ExePath);
	wcscat(DicPath, L"\\DIC\\sz_sym.dic" );

	fh = _wopen(DicPath, _O_RDONLY | _O_BINARY, _S_IREAD);
	if( fh != -1 )
	{
		nSize = _filelength(fh);
		pTemp = (unsigned char*)malloc(nSize * sizeof(unsigned char));
		_read(fh, pTemp, nSize);
		_close(fh);

		wcscpy(DicPath, ExePath);
		wcscat(DicPath, L"\\DIC\\sz_sym_bak.dic" );
		
		fh = _wopen(DicPath , _O_CREAT | _O_WRONLY | _O_BINARY, _S_IWRITE);
		if( fh != -1 )
		{
			_write(fh, pTemp, nSize);
			_close(fh);
		}
	}

	wcscpy(DicPath, ExePath);
	wcscat(DicPath, L"\\DIC\\sz_sym.dic" );
	fh = _wopen(DicPath, _O_CREAT | _O_WRONLY | _O_BINARY, _S_IWRITE);
	if( fh != -1 )
	{
		_write(fh, pDicData, nSize);
		_close(fh);
	}
	else
	{
		MessageBox(NULL, L"Can't Open File", L"error", MB_OK);
		return FALSE;
	}
	
	free(pTemp);
	return TRUE;
}

void _SymbolClassification(BYTE *pDicData, float *pFeature, float *pDegree)
{
	int i, j;
	int nLearningCount;
	double r0, r1, r2;
	float *pDicFeature;
	float TempDegree1[LOCK_SYMBOL_KIND];
	float TempDegree2[LOCK_SYMBOL_KIND];

	memset(TempDegree1, 0, sizeof(float) * LOCK_SYMBOL_KIND);
	memset(TempDegree2, 0, sizeof(float) * LOCK_SYMBOL_KIND);

	for( i = 0; i < LOCK_SYMBOL_KIND; i++ )
	{
		r0 = 0.0;
		r1 = 0.0;
		r2 = 0.0;

		pDicFeature = (float*)(pDicData + (2 * FEATURESIZE + 1) * 4 * i + 4);
		nLearningCount = *((int*)(pDicFeature-1));

		if(nLearningCount != 0)
		{
			for( j = 0; j < FEATURESIZE; j++ )
			{
				if( pDicFeature[j] < 0 )
					//AfxMessageBox("Minus1");
					MessageBox(NULL,L"Minus1", L"error", MB_OK);
				if( pFeature[j] < 0 )
					//AfxMessageBox("Minus2");
					MessageBox(NULL,L"Minus2", L"error", MB_OK);
			
				TempDegree1[i] += (float)((pDicFeature[j] - pFeature[j]) * (pDicFeature[j] - pFeature[j]) / ( DEGREE_CONST + pDicFeature[j + FEATURESIZE]));
			
				r0 += (pDicFeature[j] * pFeature[j]);
				r1 += (pDicFeature[j] * pDicFeature[j]);
				r2 += (pFeature[j] * pFeature[j]);
			}

			if( r1 == 0.0 || r2 == 0.0 )
				TempDegree2[i] = 0.0;
			else
				TempDegree2[i] = (float)(r0 / (sqrt(r1) *sqrt(r2)));
		}
		else
		{
			TempDegree1[i] = 1000000.0;
			TempDegree2[i] = 0.0;
		}
	}

	memcpy(pDegree, TempDegree2, sizeof(float) * LOCK_SYMBOL_KIND);
}

BOOL CheckLockSymbol(BYTE *pDicData, BYTE *pCharImgData,int nHeight,int nWidth)
{
	BOOL blRet = FALSE;
	///kjy-todo-2015.4.26

	if(!pCharImgData)
		return blRet;

#ifdef OCR_ONCE
	Mat test(nHeight,nWidth,CV_8UC1);
	test.data = pCharImgData;
	imshow("test",test);
	waitKey(1000);
#endif
	///////////////////
	float *pFeature;
	BYTE *pNormalData;
	float Degree[LOCK_SYMBOL_KIND];

	pNormalData = (BYTE*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
	memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

	pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

	_LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

	_Smoothing(pNormalData);

#if 0
	Mat m(66, 66, CV_8UC1);
	for(int i = 0; i < 66 * 66; i++)
		m.data[i] = pNormalData[i] == 0 ? 255 : 0;
	//memcpy(m.data, pNormalData, 66*66);
	imshow("aaa", m);waitKey(0);
#endif


	_GetFeature_Of_EndClassification( pNormalData, pFeature );
	
	_SymbolClassification(pDicData, pFeature, Degree);

	int i;
	for( i = 0; i < LOCK_SYMBOL_KIND; i++ )
	{
		if( Degree[i] > 0.8 )
		{
			blRet = TRUE;
			break;
		}
	}

#if 0
	int nMeanIndex;
	if( blRet )
	{
		nMeanIndex = 66 * 32 + 32;
		if( pNormalData[nMeanIndex] == 1 )
		{
			if( pNormalData[nMeanIndex - 1] != 1 || pNormalData[nMeanIndex + 1] != 1 ||
				pNormalData[nMeanIndex - 66] != 1 || pNormalData[nMeanIndex + 66] != 1 )
				blRet = FALSE;
		}
	}
#endif

	free(pNormalData);
	free(pFeature);

//	if( blRet == FALSE )
//		::MessageBox(NULL, (LPCWSTR)L"Failed CheckSymbol!", (LPCWSTR)L"CheckSymbol", MB_OK);
	return blRet;
}

BOOL LearningChar(BYTE *pCharImgData,int nHeight,int nWidth, WORD wCode, int nKind)
{
	BOOL blRet = TRUE;

	float *pFeature;
	BYTE *pNormalData;

#if 0
	Mat test(nHeight,nWidth,CV_8UC1);
	test.data = pCharImgData;
	imshow("test",test);
	waitKey(0);
#endif

	pNormalData = (BYTE*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
	memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

	pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

	_LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

	_Smoothing(pNormalData);

#if 0
	Mat m(66, 66, CV_8UC1);
	for(int i = 0; i < 66 * 66; i++)
		m.data[i] = pNormalData[i] == 0 ? 255 : 0;
	//memcpy(m.data, pNormalData, 66*66);
	imshow("aaa", m);waitKey(0);
#endif

	_GetFeature_Of_EndClassification( pNormalData, pFeature );

	_LearningChar(wCode, g_pDicData, pFeature, nKind); 

	free(pNormalData);
	free(pFeature);
	
	return blRet;
}

BOOL LearningLockSymbol(BYTE *pCharImgData,int nHeight,int nWidth)
{
	BOOL blRet = TRUE;

	unsigned char *pDicData = g_pLockSymDic;
	
	float *pFeature;
	BYTE *pNormalData;
	float Degree[LOCK_SYMBOL_KIND];

	pNormalData = (BYTE*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
	memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

	pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

	_LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

	_Smoothing(pNormalData);

	_GetFeature_Of_EndClassification( pNormalData, pFeature );
	
	_SymbolClassification(pDicData, pFeature, Degree);

	int i, nIndex;
	int nLearningCount;
	float fMax = -1.0;
	
	for( i = 0; i < LOCK_SYMBOL_KIND; i++ )
	{
		if( Degree[i] == 0.0 )
		{
			nLearningCount = i;
			break;
		}
		else if( Degree[i] > fMax )
		{
			fMax = Degree[i];
			nIndex = i;
		}
	}

	if(fMax < 0.5)
	{
		if( nLearningCount < LOCK_SYMBOL_KIND - 1 )
			nIndex = nLearningCount;
		else
			blRet = FALSE;
	}

	if(blRet)
		_LearningLockSymbol(pDicData, pFeature, nIndex);

	free(pNormalData);
	free(pFeature);

	return blRet;
}
#else

void _LinerNormalization(uchar *pCharImgData, int nHeight, int nWidth, uchar *pNormalData)
{
    int x, y;
    int i, j;

    double x_ratio = (double)(nWidth - 1) / (double)NORMALSIZE;
    double y_ratio = (double)(nHeight - 1) / (double)NORMALSIZE;

    unsigned int n;
    int MulTable_X[NORMALSIZE];
    int MulTable_Y[NORMALSIZE];

    for( i = 0; i < NORMALSIZE; i++)
    {
        MulTable_X[i] = (int)(x_ratio * (double)i + 0.5);
        MulTable_Y[i] = (int)(y_ratio * (double)i + 0.5);
    }

    for( i = 0, n = NORMALSIZE + 2; i < NORMALSIZE; i++, n += (NORMALSIZE + 2))
    {
        if( i < NORMALSIZE - 1)
            y = min(nHeight - 2, MulTable_Y[i]);
        else
            y = nHeight - 2;
        for( j = 0; j < NORMALSIZE; j++)
        {
            if( j < NORMALSIZE - 1)
                x = min(nWidth - 2, MulTable_X[j]);
            else
                x = nWidth - 2;
            if(pCharImgData[nWidth * y + x] == 0)
                pNormalData[ n + j + 1 ] = 1;
        }
    }
}

void _Smoothing( uchar *pNormalData )
{
    int i, j;
    unsigned int n;
    int sum;

    uchar bTemp[(NORMALSIZE + 2) * (NORMALSIZE + 2)];
    memcpy(bTemp, pNormalData, (NORMALSIZE + 2) * (NORMALSIZE + 2));

    for( i = 0, n = NORMALSIZE + 2; i < NORMALSIZE; i++, n += (NORMALSIZE + 2) )
    {
        for( j = 1; j < NORMALSIZE + 1; j++ )
        {
            sum = bTemp[n - (NORMALSIZE + 2) + j - 1] + bTemp[n - (NORMALSIZE + 2) + j] + bTemp[n - (NORMALSIZE + 2) + j + 1] +
                  bTemp[n + j - 1]                    + bTemp[n + j]                    + bTemp[n + j + 1]      +
                  bTemp[n + (NORMALSIZE + 2) + j - 1] + bTemp[n + 66 + j] + bTemp[n + (NORMALSIZE + 2) + j + 1];

            if( pNormalData[n + j] )
            {
                if( sum < 3 )
                    pNormalData[n + j] = 0;
            }
            else if ( sum >= 4)
                pNormalData[n + j] = 1;
        }
    }
}

void _GetContourNeighbour(uchar *ia, int cxDIB,int cyDIB, uchar *pNeighbourCount)
{
    unsigned int m;
    int i,j,ds,nc;

    static char ip[] = { 0,-1,-1,-1,0,1,1,1 };
    static char jp[] = { 1,1,0,-1,-1,-1,0,1 };
    static char direct[] = { 0,1,2,3,0,1,2,3 };
    int ii,jj,iw,jw,nb,n;

    POINT PrePoint;
    for( i = 1, m = cxDIB + 2; i < cyDIB + 1; i++, m += (cxDIB + 2))
    {
        for( j = 1; j < cxDIB + 1; j++){
            if(ia[ m + j ] == 1){
                nc = ia[ m + j + 1 ] & ia[ m - cxDIB - 2 + j] & ia[ m + j - 1] & ia[ m + cxDIB + 2 + j ];
                if(nc == 0){
                    if(ia[ m + cxDIB + 2 + j ] == 0)
                        ds = 6;
                    else if(ia[ m + j - 1 ] == 0)
                        ds = 4;
                    else if(ia[ m - cxDIB - 2 + j ] == 0)
                        ds = 2;
                    else if(ia[ m + j + 1 ] == 0)
                        ds = 0;
                    ii = iw = i; jj = jw = j;
                    n = ( ds + 4 ) & 7;
                    ds = -1;
                    nb = -1;
                    nc = ia[m - cxDIB - 2 + j - 1] + ia[m - cxDIB - 2 + j] + ia[m - cxDIB - 2 + j + 1] +
                         ia[m + j - 1] + ia[m + j + 1] +
                         ia[m + cxDIB + 2 + j - 1] + ia[m + cxDIB + 2 + j] + ia[m + cxDIB + 2 + j + 1];

                    if(nc > 0){
                        do{
                            ii = iw;
                            jj = jw;
                            ia[iw * (cxDIB + 2) + jw] = 3;
                            n = ( n + 4 ) & 7;
                            do{
                                n = ( n + 1 ) & 7;
                                iw = ii + ip[n];
                                jw = jj + jp[n];
                            }while(ia[iw * (cxDIB + 2) + jw] == 0);

                            if( nb >= 0 )
                            {
                                pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + direct[n]]++;
                                if( PrePoint.x == jj )
                                    pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 2]++;
                                else if( PrePoint.y == ii )
                                    pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 0]++;
                                else
                                {
                                    if( (PrePoint.x - jj + PrePoint.y - ii) == 0 )
                                        pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 1]++;
                                    else
                                        pNeighbourCount[4*ii * (cxDIB + 2) + 4*jj + 3]++;
                                }
                            }
                            PrePoint.x = jj;
                            PrePoint.y = ii;
                            if(ds < 0)
                            {
                                if(nb < 0) nb = n;
                                else ds = nb;
                            }
                        }while((ii!=i)||(jj!=j)||(n!=ds));
                    }
                    else
                        ia[m + j] = 2;
                }
            }
        }
    }

    for( i = 0; i < (cyDIB + 2) * (cxDIB + 2); i++)
    {
        if(ia[i] == 3)
            ia[i] = 1;
    }
}

void _GetPeripheral(uchar* pNormalData, uchar* pPeripheral)
{
    int i,j;
    unsigned int n = NORMALSIZE + 2;

    for( i = 1; i < NORMALSIZE + 1; i++, n += (NORMALSIZE + 2) )
    {
        for( j = 1; j < NORMALSIZE + 1; j++)
        {
            if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j ] == 1 )
                pPeripheral[ (i - 1) * NORMALSIZE + j - 1 ] += 1;
            if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j ] == 1 )
                pPeripheral[ (i - 1) * NORMALSIZE + j - 1 ] += 1;

            if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j + 1 ] == 1 )
                pPeripheral[ (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
            if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j - 1 ] == 1 )
                pPeripheral[ (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;

            if( pNormalData[ n + j ] - pNormalData[ n + j - 1 ] == 1 )
                pPeripheral[ 2 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
            if( pNormalData[ n + j ] - pNormalData[ n + j + 1 ] == 1 )
                pPeripheral[ 2 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;

            if( pNormalData[ n + j ] - pNormalData[ n - (NORMALSIZE + 2) + j - 1 ] == 1 )
                pPeripheral[ 3 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
            if( pNormalData[ n + j ] - pNormalData[ n + (NORMALSIZE + 2) + j + 1 ] == 1 )
                pPeripheral[ 3 * (NORMALSIZE * NORMALSIZE) + (i - 1) * NORMALSIZE + j - 1 ] += 1;
        }
    }
}

void _GetFeature_Of_EndClassification( uchar *pNormalData, float *pFeature )
{
    int depth;
    int i, j, k, m, total;
    int c1, c2, c3, c4;
    int start, end;
    double f1, f2, f3, f4;
    unsigned int n, nn;

    uchar *pPeripheral = (uchar*)malloc(NORMALSIZE * NORMALSIZE * 4);
    memset(pPeripheral, 0, NORMALSIZE * NORMALSIZE * 4);


    NEIGHBOURCOUNT *pNeighbourCount = (NEIGHBOURCOUNT*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2) * sizeof(NEIGHBOURCOUNT));
    memset(pNeighbourCount, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2) * sizeof(NEIGHBOURCOUNT));

    _GetPeripheral(pNormalData, pPeripheral);
    _GetContourNeighbour((uchar*)pNormalData, NORMALSIZE, NORMALSIZE, (uchar*)pNeighbourCount);

    for( depth = 0, nn = 0; depth < 3; depth++, nn += (NORMALSIZE * 4) )
    {
        for( int direct = 0; direct < 8; direct++ )
        {
            f1 = f2 = f3 = f4 = 0.0;

            start = direct * 8;
            end = start + 8;
            for( i = start; i < end; i++)
            {
                c1 = c2 = c3 = c4 = 0;
                for( j = 0, n = 0; j < NORMALSIZE; j++, n += NORMALSIZE)
                {
                    if(pPeripheral[n + i])
                    {
                        for( k = 0; k < 3; k++ )
                        {
                            for( m = 0; m < 3; m++ )
                            {
                                c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_1;
                                c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_2;
                                c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_3;
                                c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_4;
                            }
                        }

                        total = c1 + c2 + c3 + c4;

                        if(total != 0)
                        {
                            f1 += (((double)c1) / total);
                            f2 += (((double)c2) / total);
                            f3 += (((double)c3) / total);
                            f4 += (((double)c4) / total);
                        }

                        pPeripheral[n + i]--;

                        break;
                    }
                }
            }
            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;

            pFeature[0 + start + nn] = (float)f1;
            pFeature[1 + start + nn] = (float)f2;
            pFeature[2 + start + nn] = (float)f3;
            pFeature[3 + start + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            for( i = start; i < end; i++)
            {
                c1 = c2 = c3 = c4 = 0;
                for( j = (NORMALSIZE - 1), n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0; j--, n -= NORMALSIZE)
                {
                    if(pPeripheral[n + i])
                    {
                        for( k = 0; k < 3; k++ )
                        {
                            for( m = 0; m < 3; m++ )
                            {
                                c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_1;
                                c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_2;
                                c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_3;
                                c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + k) + i + m ].Direct_4;
                            }
                        }

                        total = c1 + c2 + c3 + c4;

                        if(total != 0)
                        {
                            f1 += (((double)c1) / total);
                            f2 += (((double)c2) / total);
                            f3 += (((double)c3) / total);
                            f4 += (((double)c4) / total);
                        }

                        pPeripheral[n + i]--;

                        break;
                    }
                }
            }

            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;

            pFeature[0 + start + 4 + nn] = (float)f1;
            pFeature[1 + start + 4 + nn]=  (float)f2;
            pFeature[2 + start + 4 + nn] = (float)f3;
            pFeature[3 + start + 4 + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            for( i = start, n = NORMALSIZE * start; i < end; i++, n += NORMALSIZE)
            {
                c1 = c2 = c3 = c4 = 0;
                for( j = 0; j < NORMALSIZE; j++ )
                {
                    if(pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)])
                    {
                        for( k = 0; k < 3; k++ )
                        {
                            for( m = 0; m < 3; m++ )
                            {
                                c1 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_1;
                                c2 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_2;
                                c3 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_3;
                                c4 += pNeighbourCount[ (NORMALSIZE + 2)*(i+k) + j + m ].Direct_4;
                            }
                        }

                        total = c1 + c2 + c3 + c4;

                        if(total != 0)
                        {
                            f1 += (((double)c1) / total);
                            f2 += (((double)c2) / total);
                            f3 += (((double)c3) / total);
                            f4 += (((double)c4) / total);
                        }

                        pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)]--;

                        break;
                    }
                }
            }

            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + NORMALSIZE + nn] = (float)f1;
            pFeature[1 + start + NORMALSIZE + nn]=  (float)f2;
            pFeature[2 + start + NORMALSIZE + nn] = (float)f3;
            pFeature[3 + start + NORMALSIZE + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            for( i = start, n = NORMALSIZE * start; i < end; i++, n += NORMALSIZE)
            {
                c1 = c2 = c3 = c4 = 0;
                for( j = (NORMALSIZE - 1); j >= 0; j-- )
                {
                    if(pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)])
                    {
                        for( k = 0; k < 3; k++ )
                        {
                            for( m = 0; m < 3; m++ )
                            {
                                c1 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_1;
                                c2 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_2;
                                c3 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_3;
                                c4 += pNeighbourCount[ (NORMALSIZE + 2) * (i + k) + j + m ].Direct_4;
                            }
                        }

                        total = c1 + c2 + c3 + c4;

                        if(total != 0)
                        {
                            f1 += (((double)c1) / total);
                            f2 += (((double)c2) / total);
                            f3 += (((double)c3) / total);
                            f4 += (((double)c4) / total);
                        }

                        pPeripheral[n + j + (2 * NORMALSIZE * NORMALSIZE)]--;

                        break;
                    }
                }
            }

            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + NORMALSIZE + 4 + nn] = (float)f1;
            pFeature[1 + start + NORMALSIZE + 4 + nn]=  (float)f2;
            pFeature[2 + start + NORMALSIZE + 4 + nn] = (float)f3;
            pFeature[3 + start + NORMALSIZE + 4 + nn] = (float)f4;

            int l;
            int start1,end1;

            f1 = f2 = f3 = f4 = 0.0;
            if( (NORMALSIZE - 1) - 2 * start > 0)
            {
                for( i = (NORMALSIZE - 1) - 2 * start; i > (NORMALSIZE - 1) - 2 * end; i--)
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = i, k = 0, n = i * NORMALSIZE; j < NORMALSIZE && k < NORMALSIZE; j++, k++, n += NORMALSIZE)
                    {
                        if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            else
            {
                start1 = 2 * start - (NORMALSIZE - 1);
                end1 = start1 + 16;
                for( i = start1; i < end1; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = 0, k = i, n = 0; j < NORMALSIZE && k < NORMALSIZE; j++, k++, n += NORMALSIZE )
                    {
                        if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j+l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }

            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + 128 + nn] = (float)f1;
            pFeature[1 + start + 128 + nn]=  (float)f2;
            pFeature[2 + start + 128 + nn] = (float)f3;
            pFeature[3 + start + 128 + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            if( 2 * start < NORMALSIZE )
            {
                for( i = 2 * start; i < 2 * start + 16; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = (NORMALSIZE - 1), k = i, n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0 && k >= 0; j--, k--, n -= NORMALSIZE)
                    {
                        if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            else
            {
                start1 = 128 - 2*(start+1);
                end1 = start1 - 16;
                for( i = start1; i > end1; i-- )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = i, k = (NORMALSIZE - 1), n = i * NORMALSIZE; j >= 0 && k >= 0; j--, k--, n -= NORMALSIZE )
                    {
                        if(pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (3 * NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + 128 + 4 + nn] = (float)f1;
            pFeature[1 + start + 128 + 4 + nn]=  (float)f2;
            pFeature[2 + start + 128 + 4 + nn] = (float)f3;
            pFeature[3 + start + 128 + 4 + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            if( 2 * start < NORMALSIZE )
            {
                for( i = 2 * start; i < 2 * start + 16; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = i, k = 0, n = i * NORMALSIZE; j >= 0 && k < NORMALSIZE; j--, k++, n -= NORMALSIZE )
                    {
                        if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            else
            {
                start1 = 2 * start - (NORMALSIZE - 1);
                end1 = start1 + 16;
                for( i = start1; i < end1; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = (NORMALSIZE - 1), k = i, n = (NORMALSIZE - 1) * NORMALSIZE; j >= 0 && k < NORMALSIZE; j--, k++, n -= NORMALSIZE )
                    {
                        if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + 192 + nn] = (float)f1;
            pFeature[1 + start + 192 + nn]=  (float)f2;
            pFeature[2 + start + 192 + nn] = (float)f3;
            pFeature[3 + start + 192 + nn] = (float)f4;

            f1 = f2 = f3 = f4 = 0.0;
            if(2 * start < NORMALSIZE )
            {
                for( i = 2 * start; i < 2 * start + 16; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = 0, k = i, n = 0; j < NORMALSIZE && k >= 0; j++, k--, n += NORMALSIZE )
                    {
                        if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            else
            {
                start1 = 2 * start - (NORMALSIZE - 1);
                end1 = start1 + 16;
                for( i = start1; i < end1; i++ )
                {
                    c1 = c2 = c3 = c4 = 0;
                    for( j = i, k = (NORMALSIZE - 1), n = i * NORMALSIZE; j < NORMALSIZE && k >= 0; j++, k--, n += NORMALSIZE )
                    {
                        if(pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ])
                        {
                            for( l = 0; l < 3; l++ )
                            {
                                for( m = 0; m < 3; m++ )
                                {
                                    c1 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_1;
                                    c2 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_2;
                                    c3 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_3;
                                    c4 += pNeighbourCount[ (NORMALSIZE + 2) * (j + l) + k + m ].Direct_4;
                                }
                            }

                            total = c1 + c2 + c3 + c4;

                            if(total != 0)
                            {
                                f1 += (((double)c1) / total);
                                f2 += (((double)c2) / total);
                                f3 += (((double)c3) / total);
                                f4 += (((double)c4) / total);
                            }

                            pPeripheral[ (NORMALSIZE * NORMALSIZE) + n + k ]--;

                            break;
                        }
                    }
                }
            }
            f1 *= 0.125;
            f2 *= 0.125;
            f3 *= 0.125;
            f4 *= 0.125;
            pFeature[0 + start + 192 + 4 + nn] = (float)f1;
            pFeature[1 + start + 192 + 4 + nn]=  (float)f2;
            pFeature[2 + start + 192 + 4 + nn] = (float)f3;
            pFeature[3 + start + 192 + 4 + nn] = (float)f4;
        }
    }

    free(pPeripheral);
    free(pNeighbourCount);
}

void _EndClassification(float *pFeature, BYTE **ppDicData, WORD *pCode, RECOG_DEGREE *pRecog_Degree)
{
    int i, j, k;
    int nLearningCount;
    double r0, r1, r2;
    float *pDicFeature;
    RECOG_DEGREE *pTempDegree1;
    RECOG_DEGREE *pTempDegree2;
    int *pTempIndex;

    pTempDegree1 = (RECOG_DEGREE*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));
    memset(pTempDegree1, 0, g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));

    pTempDegree2 = (RECOG_DEGREE*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));
    memset(pTempDegree2, 0, g_nKindNum * g_nRecogCharNum * sizeof(RECOG_DEGREE));

    pTempIndex = (int*)malloc(g_nKindNum * g_nRecogCharNum * sizeof(int));
    memset(pTempIndex, 0, g_nKindNum * g_nRecogCharNum * sizeof(int));

    for( k = 0; k < g_nKindNum; k++ )
    {
        for( i = 0; i < g_nRecogCharNum; i++ )
        {
            if( AV_RecogChar[i] < 0 )
                continue;

            r0 = 0.0;
            r1 = 0.0;
            r2 = 0.0;

            pTempIndex[i + k * g_nRecogCharNum] = i;
            pTempDegree1[i + k * g_nRecogCharNum].nKind = k;
            pTempDegree2[i + k * g_nRecogCharNum].nKind = k;

            pDicFeature = (float*)(ppDicData[k] + (2 * FEATURESIZE + 1) * 4 * i + 4);
            nLearningCount = *((int*)(pDicFeature-1));

            if(nLearningCount != 0)
            {
                for( j = 0; j < FEATURESIZE; j++ )
                {
#if 0
                    if( pDicFeature[j] < 0 )
                        //AfxMessageBox("Minus1");
                        MessageBox(NULL,L"Minus1", L"error", MB_OK);
                    if( pFeature[j] < 0 )
                        //AfxMessageBox("Minus2");
                        MessageBox(NULL,L"Minus2", L"error", MB_OK);
#endif
                    pTempDegree1[i + k * g_nRecogCharNum].fDegree += (float)((pDicFeature[j] - pFeature[j]) * (pDicFeature[j] - pFeature[j]) / ( DEGREE_CONST + pDicFeature[j + FEATURESIZE]));

                    r0 += (pDicFeature[j] * pFeature[j]);
                    r1 += (pDicFeature[j] * pDicFeature[j]);
                    r2 += (pFeature[j] * pFeature[j]);
                }

                if( r1 == 0.0 || r2 == 0.0 )
                    pTempDegree2[i + k * g_nRecogCharNum].fDegree = 0.0;
                else
                    pTempDegree2[i + k * g_nRecogCharNum].fDegree = (float)(r0 / (sqrt(r1) *sqrt(r2)));
            }
            else
            {
                pTempDegree1[i + k * g_nRecogCharNum].fDegree = 1000000.0;
                pTempDegree2[i + k * g_nRecogCharNum].fDegree = 0.0;
            }
        }
    }

    for( i = 0; i < g_nKindNum * g_nRecogCharNum; i++ )
    {
        for( j = i + 1; j < g_nKindNum * g_nRecogCharNum; j++ )
        {
            if( pTempDegree2[i].fDegree < pTempDegree2[j].fDegree )
            //if( pTempDegree1[i].fDegree > pTempDegree1[j].fDegree )
            {
                float fTemp = pTempDegree1[i].fDegree;
                int nTemp = pTempDegree1[i].nKind;

                pTempDegree1[i].fDegree = pTempDegree1[j].fDegree;
                pTempDegree1[i].nKind = pTempDegree1[j].nKind;
                pTempDegree1[j].fDegree = fTemp;
                pTempDegree1[j].nKind = nTemp;

                fTemp = pTempDegree2[i].fDegree;
                nTemp = pTempDegree2[i].nKind;
                pTempDegree2[i].fDegree = pTempDegree2[j].fDegree;
                pTempDegree2[i].nKind = pTempDegree2[j].nKind;
                pTempDegree2[j].fDegree = fTemp;
                pTempDegree2[j].nKind = nTemp;

                nTemp = pTempIndex[i];
                pTempIndex[i] = pTempIndex[j];
                pTempIndex[j] = nTemp;
            }
        }
    }

    for(i = 0; i < CANDNUM; i++)
        pCode[i] = RecogChar[pTempIndex[i]];

    memcpy(pRecog_Degree, pTempDegree2, CANDNUM * sizeof(RECOG_DEGREE));

    free(pTempDegree1);
    free(pTempDegree2);
    free(pTempIndex);
}

void _GetHistogram_H(uchar* pCharImagData, int nHeight, int nWidth, int *pHist)
{
    int i, j, n;

    for( i = 0; i < nWidth; i++)
    {
        for(j = 0, n = 0; j < nHeight; j++, n += nWidth)
        {
            if(pCharImagData[i + n] == 0)
                pHist[i]++;
        }
    }
}

void _GetHistogram_V(uchar* pCharImgData, int nHeight, int nWidth, int *pHist)
{
    int i, j, n;

    for( i = 0, n = 0; i < nHeight; i++, n += nWidth)
    {
        for(j = 0; j < nWidth; j++)
        {
            if(pCharImgData[n + j] == 0)
                pHist[i]++;
        }
    }
}

bool _CheckSemicolon(uchar* pCharImgData, int nHeight, int nWidth)
{
    bool blRet = FALSE;

    int *pHist = (int*)malloc(nHeight * sizeof(int));
    memset(pHist, 0, nHeight * sizeof(int));

    _GetHistogram_V(pCharImgData, nHeight, nWidth, pHist);

    bool flag = FALSE;
    int i, nCount;

    int nCheck = (int)(nWidth * 0.9);

    for( i = 0; i < nHeight; i++)
    {
        if(pHist[i] == 0)
        {
            if(flag == FALSE)
            {
                flag = TRUE;
                nCount = 1;
            }
            else
                nCount++;
        }
        else
        {
            if(flag == TRUE)
            {
                flag = FALSE;
                if(nCount >= nCheck)
                {
                    blRet = TRUE;
                    break;
                }
            }
        }
    }

    free(pHist);

    return blRet;
}

void _Check_Split_Char(uchar* pCharImgData, int nHeight, int nWidth, RECT *pRect)
{
    int *pHist = (int*)malloc(nWidth * sizeof(int));
    memset(pHist, 0, nWidth * sizeof(int));

    _GetHistogram_H(pCharImgData, nHeight, nWidth, pHist);

    int i;
    bool blRet = FALSE;
    bool blFlag = FALSE;

    for( i = 0; i < nWidth; i++)
    {
        if(pHist[i] < 2)
        {
            if(blFlag == FALSE)
            {
                pRect->right = i;
                blFlag = TRUE;
            }
        }
        else
        {
            if(blFlag == TRUE)
            {
                pRect->left = i;
                blRet = TRUE;
                break;
            }
        }
    }

    if( blRet == FALSE )
    {
        int split_x;
        int temp = nHeight + 1;
        int left = max(0, (nWidth / 2 - 5));
        int right = min(nWidth, (nWidth / 2 + 5));

        for( i = left; i < right; i++)
        {
            if( pHist[i] < temp )
            {
                temp = pHist[i];
                split_x = i;
            }
        }
        pRect->right = max(0, split_x - 1);
        pRect->left = min(nWidth, split_x + 1);
    }

    free(pHist);
}

void _GetTopBottom(int *pHist, int nHeight, RECT *pRect)
{
    int i;

    pRect->top = 0;
    pRect->bottom = nHeight - 1;

    for( i = 0; i < nHeight; i++)
    {
        if(pHist[i] != 0)
        {
            pRect->top = i;
            break;
        }
    }

    for(i = nHeight - 1; i > 0; i--)
    {
        if(pHist[i] != 0)
        {
            pRect->bottom = i;
            break;
        }
    }
}

#ifdef OCRLEARNING
static int sCount = 0;
#endif

WORD RecognitionChar(uchar **ppDicData, uchar *pCharImgData, int nHeight, int nWidth, float *pfDegree)
{
    WORD wRet;
    uchar *pNormalData;
    float *pFeature;

    WORD wCode_0[10];
    WORD wCode_1[10];
    WORD wCode_2[10];

    RECOG_DEGREE Recog_Degree_0[10];
    RECOG_DEGREE Recog_Degree_1[10];
    RECOG_DEGREE Recog_Degree_2[10];

    pNormalData = (uchar*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
    memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

    pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

    _LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

    _Smoothing(pNormalData);
#ifdef OCR_TEST
    Mat test(nHeight,nWidth,CV_8UC1);
    test.data = pCharImgData;
    imshow("test",test);
    waitKey(1000);
#endif
#if 0
    Mat m(66, 66, CV_8UC1);
    for(int i = 0; i < 66 * 66; i++)
        m.data[i] = pNormalData[i] == 0 ? 255 : 0;
    //memcpy(m.data, pNormalData, 66*66);
    imshow("aaa", m);waitKey(0);
#endif

    _GetFeature_Of_EndClassification( pNormalData, pFeature );

    _EndClassification(pFeature, ppDicData, wCode_0, Recog_Degree_0);

    float ratio1 = (float)nHeight / nWidth;
    float ratio2 = (float)nWidth / nHeight;

    if((wCode_0[0] == 0x4f || wCode_0[0] == 0x6f) && ratio2 < 0.8)
        wCode_0[0] = 0x30;
    if(wCode_0[0] == 0x31 || wCode_0[0] == 0x49 || wCode_0[0] == 0x6C)
    {
        if( (nHeight < 13 && nWidth < 13) ||
            (ratio1 < 1.5 && Recog_Degree_0[0].fDegree < 0.8))
            wCode_0[0] = 0x2e;
    }

#ifdef OCRLEARNING
    //char LearningCode[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefhijklmnorstuvwxzygpq/:";
    //char LearningCode[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefhijklmnorstuvwxzygpq:/";
    char LearningCode[] = "012345";
    uchar flag[]         = {0,0,1,0,0,1,0,0,0};
    //if(flag[sCount])
    //	_Learning(LearningCode[sCount], ppDicData, pFeature, 0);
    //if(sCount == 2)
        _Learning(LearningCode[sCount], ppDicData, pFeature, 0);


    sCount++;

    if(sCount == 17)
        sCount = sCount;
#endif

    if(wCode_0[0] == 0x3A && Recog_Degree_0[0].fDegree < 0.9)
    {
        if(_CheckSemicolon(pCharImgData, nHeight, nWidth) == FALSE)
            wCode_0[0] = 0x31;
    }

    wRet = wCode_0[0];

    bool blFlag = TRUE;
    int i, nCount;
    float fTemp = Recog_Degree_0[0].fDegree;
    for( i = 1, nCount = 1; i < CANDNUM; i++)
    {
        if(wCode_0[i] == wRet)
        {
            nCount++;
            fTemp += Recog_Degree_0[i].fDegree;
        }
    }

    if(nCount > 3 )
    {
        fTemp /= nCount;
        if( fTemp > 0.75 )
            blFlag = FALSE;
    }

    if( blFlag && Recog_Degree_0[0].fDegree < 0.85 && wCode_0[0] != 0x2f )
    {
        if(ratio2 > 0.68)
        {
            RECT rect;
            _Check_Split_Char(pCharImgData, nHeight, nWidth, &rect);
            //{
                int *pHist;
                int j, n0, n1;
                int nHeight_1, nHeight_2 = nHeight;
                int nWidth_1 = rect.right;
                int nWidth_2 = nWidth - rect.left;

                nHeight_1 = nHeight_2 = nHeight;

                pHist = (int*)malloc(nHeight * sizeof(int));

                uchar *pCharImgData_1 = (uchar*)malloc(nHeight * nWidth_1);
                uchar *pCharImgData_2 = (uchar*)malloc(nHeight * nWidth_2);

                for( i = 0, n0 = 0, n1 = 0; i < nHeight; i++, n0 +=  nWidth_1, n1 += nWidth)
                {
                    for(j = 0; j < nWidth_1; j++)
                    {
                        pCharImgData_1[n0 + j] = pCharImgData[n1 + j];
                    }
                }

                for( i = 0, n0 = 0, n1 = 0; i < nHeight; i++, n0 +=  nWidth_2, n1 += nWidth)
                {
                    for(j = 0; j < nWidth_2; j++)
                    {
                        pCharImgData_2[n0 + j] = pCharImgData[n1 + j + rect.left];
                    }
                }

#if 0
                Mat m3(nHeight, nWidth_1, CV_8UC1);
                //for(i = 0; i < 66 * 66; i++)
                    //m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
                memcpy(m3.data, pCharImgData_1, nHeight * nWidth_1);
                imshow("aaa3", m3);waitKey(0);
#endif

                memset(pHist, 0, nHeight * sizeof(int));
                _GetHistogram_V(pCharImgData_1, nHeight, nWidth_1, pHist);
                _GetTopBottom(pHist, nHeight, &rect);

                if( rect.top != 0 )
                {
                    memcpy(pCharImgData, pCharImgData_1, nHeight * nWidth_1);
                    nHeight_1 = rect.bottom - rect.top + 1;
                    memcpy(pCharImgData_1, pCharImgData + rect.top * nWidth_1, nHeight_1 * nWidth_1);
                }

                memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));
                _LinerNormalization(pCharImgData_1, nHeight_1, nWidth_1, pNormalData);
                _Smoothing(pNormalData);

#if 0
                Mat m1(66, 66, CV_8UC1);
                for(i = 0; i < 66 * 66; i++)
                    m1.data[i] = pNormalData[i] == 0 ? 255 : 0;
                //memcpy(m1.data, pCharImgData_1, nHeight_1 * nWidth_1);
                imshow("aaa0", m1);waitKey(0);
#endif

                _GetFeature_Of_EndClassification( pNormalData, pFeature );
                _EndClassification(pFeature, ppDicData, wCode_1, Recog_Degree_1);

                ratio2 = (float)nWidth_1 / nHeight_1;
                if(ratio2 > 1.5 && (wCode_1[0] == 0x31 || wCode_1[0] == 0x49 || wCode_1[0] == 0x6c))
                {
                    wCode_1[0] = 0x2D;
                }

#if 0
                if(sCount == 1)
                {
                    _Learning(0x30, ppDicData, pFeature, 4);
                }
#endif

#if 0
                Mat m4(nHeight, nWidth_2, CV_8UC1);
                //for(i = 0; i < 66 * 66; i++)
                    //m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
                memcpy(m4.data, pCharImgData_2, nHeight * nWidth_2);
                imshow("aaa4", m4);waitKey(0);
#endif

                memset(pHist, 0, nHeight * sizeof(int));
                _GetHistogram_V(pCharImgData_2, nHeight, nWidth_2, pHist);
                _GetTopBottom(pHist, nHeight, &rect);

                if( rect.top != 0 )
                {
                    memcpy(pCharImgData, pCharImgData_2, nHeight * nWidth_2);
                    nHeight_2 = rect.bottom - rect.top + 1;
                    memcpy(pCharImgData_2, pCharImgData + rect.top * nWidth_2, nHeight_2 * nWidth_2);
                }


                memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));
                _LinerNormalization(pCharImgData_2, nHeight_2, nWidth_2, pNormalData);
                _Smoothing(pNormalData);

#if 0
                Mat m2(66, 66, CV_8UC1);
                for(i = 0; i < 66 * 66; i++)
                    m2.data[i] = pNormalData[i] == 0 ? 255 : 0;
                //memcpy(m2.data, pCharImgData_2, nHeight * nWidth_2);
                imshow("aaa1", m2);waitKey(0);
#endif
                _GetFeature_Of_EndClassification( pNormalData, pFeature );
                _EndClassification(pFeature, ppDicData, wCode_2, Recog_Degree_2);

                ratio2 = (float)nWidth_2 / nHeight_2;
                if(ratio2 > 1.5 && (wCode_2[0] == 0x31 || wCode_2[0] == 0x49 || wCode_2[0] == 0x6c))
                {
                    wCode_2[0] = 0x2D;
                }

#if 0
                if(sCount == 12)
                {
                    _Learning(0x30, ppDicData, pFeature, 4);
                }
#endif

                if((Recog_Degree_1[0].fDegree > 0.8 && Recog_Degree_2[0].fDegree > 0.8) &&
                    ((Recog_Degree_1[0].fDegree + Recog_Degree_2[0].fDegree) / 2.0 > Recog_Degree_0[0].fDegree))
                {
                    wRet = (wCode_2[0] << 8) | wCode_1[0];
                }

                free(pHist);
                free(pCharImgData_1);
                free(pCharImgData_2);

            //}
        }
    }

    free(pNormalData);
    free(pFeature);

    *pfDegree = Recog_Degree_0[0].fDegree;

    return wRet;
}


#define LOCK_SYMBOL_KIND 10
#if 0
bool _LearningLockSymbol(uchar *pDicData, float *pFeature, int nKind)
{
    // TODO: Add extra validation here

    int i;
    float fTemp;

    uchar *pTempDicData = pDicData + (2 * FEATURESIZE + 1) * 4 * nKind;

    int nLearningCount = *((int*)(pTempDicData));
    float *pDicFeature = (float*)(pTempDicData + 4);

    for( i = 0; i < FEATURESIZE; i++ )
    {
        //pDicFeature[i] = (nLearningCount * pDicFeature[i] + 2 * pFeature[i]) / (float)(nLearningCount + 2);
        pDicFeature[i] = (nLearningCount * pDicFeature[i] + pFeature[i]) / (float)(nLearningCount + 1);
        fTemp = pDicFeature[i] - pFeature[i];
        fTemp *= fTemp;
        fTemp *= fTemp;
        //pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + (float)(2 * fTemp)) / (float)(nLearningCount + 2);
        pDicFeature[i + FEATURESIZE] = (nLearningCount * pDicFeature[i + FEATURESIZE] + fTemp) / (float)(nLearningCount + 1);
    }

    nLearningCount += 1;
    *((int*)(pTempDicData)) = nLearningCount;

    unsigned char DicPath[256];
    /*GetCurrentDirectory(256, DicPath);*/
    lstrcpyW(DicPath, L"E:\\DATA2\\data\\project\\shenzhen\\OCR\\SZOCR\\trunk\\Sources\\Server\\EngineTester\\DIC\\sz_sym.dic");

    int fh, nSize;
    if(!_wsopen_s(&fh, DicPath, _O_WRONLY | _O_BINARY, _SH_DENYNO, _S_IWRITE))
    {
        nSize = _filelength(fh);
        _write(fh, pDicData, nSize);
        _close(fh);
    }
    else
    {
        MessageBox(NULL, L"Can't Open File", L"error", MB_OK);
        return FALSE;
    }

    return TRUE;
}
#endif
void _SymbolClassification(uchar *pDicData, float *pFeature, float *pDegree)
{
    int i, j;
    int nLearningCount;
    double r0, r1, r2;
    float *pDicFeature;
    float TempDegree1[LOCK_SYMBOL_KIND];
    float TempDegree2[LOCK_SYMBOL_KIND];

    memset(TempDegree1, 0, sizeof(float) * LOCK_SYMBOL_KIND);
    memset(TempDegree2, 0, sizeof(float) * LOCK_SYMBOL_KIND);

    for( i = 0; i < LOCK_SYMBOL_KIND; i++ )
    {
        r0 = 0.0;
        r1 = 0.0;
        r2 = 0.0;

        pDicFeature = (float*)(pDicData + (2 * FEATURESIZE + 1) * 4 * i + 4);
        nLearningCount = *((int*)(pDicFeature-1));

        if(nLearningCount != 0)
        {
            for( j = 0; j < FEATURESIZE; j++ )
            {
#ifdef MSVC_OCR
                if( pDicFeature[j] < 0 )
                    //AfxMessageBox("Minus1");
                    MessageBox(NULL,L"Minus1", L"error", MB_OK);
                if( pFeature[j] < 0 )
                    //AfxMessageBox("Minus2");
                    MessageBox(NULL,L"Minus2", L"error", MB_OK);
#endif

                TempDegree1[i] += (float)((pDicFeature[j] - pFeature[j]) * (pDicFeature[j] - pFeature[j]) / ( DEGREE_CONST + pDicFeature[j + FEATURESIZE]));

                r0 += (pDicFeature[j] * pFeature[j]);
                r1 += (pDicFeature[j] * pDicFeature[j]);
                r2 += (pFeature[j] * pFeature[j]);
            }

            if( r1 == 0.0 || r2 == 0.0 )
                TempDegree2[i] = 0.0;
            else
                TempDegree2[i] = (float)(r0 / (sqrt(r1) *sqrt(r2)));
        }
        else
        {
            TempDegree1[i] = 1000000.0;
            TempDegree2[i] = 0.0;
        }
    }

    memcpy(pDegree, TempDegree2, sizeof(float) * LOCK_SYMBOL_KIND);

#if 0
    _LearningLockSymbol(pDicData, pFeature, 6);
#endif

}

bool CheckLockSymbol(uchar *pDicData, uchar *pCharImgData,int nHeight,int nWidth)
{
    bool blRet = FALSE;
    ///kjy-todo-2015.4.26

    if(!pCharImgData)
        return blRet;

#ifdef OCR_ONCE
    Mat test(nHeight,nWidth,CV_8UC1);
    test.data = pCharImgData;
    imshow("test",test);
    waitKey(1000);
#endif
    ///////////////////
    float *pFeature;
    uchar *pNormalData;
    float Degree[10];

    pNormalData = (uchar*)malloc((NORMALSIZE + 2) * (NORMALSIZE + 2));
    memset(pNormalData, 0, (NORMALSIZE + 2) * (NORMALSIZE + 2));

    pFeature = (float*)malloc(FEATURESIZE * sizeof(float));

    _LinerNormalization(pCharImgData, nHeight, nWidth, pNormalData);

    _Smoothing(pNormalData);

#if 0
    Mat m(66, 66, CV_8UC1);
    for(int i = 0; i < 66 * 66; i++)
        m.data[i] = pNormalData[i] == 0 ? 255 : 0;
    //memcpy(m.data, pNormalData, 66*66);
    imshow("aaa", m);waitKey(0);
#endif


    _GetFeature_Of_EndClassification( pNormalData, pFeature );

    _SymbolClassification(pDicData, pFeature, Degree);

    int i;
    for( i = 0; i < LOCK_SYMBOL_KIND; i++ )
    {
        if( Degree[i] > 0.75 )
        {
            blRet = TRUE;
            break;
        }
    }

#if 0
    int nMeanIndex;
    if( blRet )
    {
        nMeanIndex = 66 * 32 + 32;
        if( pNormalData[nMeanIndex] == 1 )
        {
            if( pNormalData[nMeanIndex - 1] != 1 || pNormalData[nMeanIndex + 1] != 1 ||
                pNormalData[nMeanIndex - 66] != 1 || pNormalData[nMeanIndex + 66] != 1 )
                blRet = FALSE;
        }
    }
#endif

    free(pNormalData);
    free(pFeature);

    return blRet;
}

#endif


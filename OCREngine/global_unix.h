#pragma once
#define USER_DEFINED_MIN 10000
#define USER_DEFINED_MAX 0
#define USER_DEFINED_STRING_SIZE 40//40//30
#define OCR_PIXEL_MIN 0
#define OCR_PIXEL_MAX 255
typedef unsigned short      WORD;
typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef unsigned long WCHAR;
typedef bool BOOL;
typedef struct t_POINT{
    long x;
    long y;
} POINT,*lpPOINT;

typedef struct tagRECT
{
    long    left;
    long    top;
    long    right;
    long    bottom;
} RECT, *PRECT;

typedef struct group_{
	int size;
	int firstindex;
	int lastindex;
	group_()
	{
		size = firstindex = lastindex = 0;
	}
	group_(int size_,int firstindex_,int lastindex_)
	{
		size = size_;
		firstindex = firstindex_;
		lastindex = lastindex_;
	}
	void setgroup_(int firstindex_,int lastindex_)
	{
		size = lastindex_ - firstindex_;
		firstindex = firstindex_;
		lastindex = lastindex_;
	}
	void setSize(int size_)
	{
		size = size_;
	}
} group_;

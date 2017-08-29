#include "stdafx.h"
#include "Array.h"

void blur_(int* array_,int arraySize)
{
	if(arraySize < 7 || !array_) return;

	int *array_t;
	array_t = new int[arraySize];

	//cvRound((array_[0] + array_[1]) / 2.0);
	array_[0] = array_[1];
	array_[1] = array_[2];
	array_[2] = array_[3];
	array_[arraySize - 3] = array_[arraySize - 4];
	array_[arraySize - 2] = array_[arraySize - 3];
	array_[arraySize - 1] = array_[arraySize - 2];
	for(int i = 3; i < arraySize - 3; i++)
		array_t[i] = cvRound((array_[i - 3] + array_[i - 2] + array_[i - 1] + array_[i] + array_[i + 1] + array_[i + 2] + array_[i + 3]) / 7);
	array_t[0] = array_[1];
	array_t[1] = array_[2];
	array_t[2] = array_[3];
	array_t[arraySize - 1] = array_[arraySize - 2];
	array_t[arraySize - 2] = array_[arraySize - 3];
	array_t[arraySize - 3] = array_[arraySize - 4];

	for(int i = 0; i < arraySize; i++)
		array_[i] = array_t[i];

	delete []array_t;

	return;
}

void detectPeak(int* array_,int arraySize)
{
	if(arraySize == 0 || !array_ || arraySize < 5) return;
	int *array_t;
	array_t = new int[arraySize];
	for(int i = 2; i < arraySize - 2; i++)
		if((array_[i] > array_[i - 2] &&
		   array_[i] > array_[i - 1] &&
		   array_[i] > array_[i + 1] &&
		   array_[i] > array_[i + 2])||
		   (array_[i] < array_[i - 2] &&
		   array_[i] < array_[i - 1] &&
		   array_[i] < array_[i + 1] &&
		   array_[i] < array_[i + 2]))
		   array_t[i] = 1;
		else
		   array_[i] = 0;
	array_t[0] = 0 ;
	array_t[1] = 0 ;
	array_t[arraySize - 2] = 0 ;
	array_t[arraySize - 1] = 0 ;

	for(int i = 0; i < arraySize; i++)
		array_[i] = array_t[i];
	delete []array_t;
	return;
}
int calcMax(int* array_,int arraySize)
{
	int max = 0;
	for(int i = 0; i < arraySize; i++)
		if(array_[i]  > max) max = array_[i];
	return max;
}
int calcMin(int* array_,int arraySize)
{
	int min =  calcMax(array_,arraySize);//2 * arraySize;
	for(int i = 0; i < arraySize; i++)
		if(array_[i] < min) min = array_[i];
	return min;
}
int calcMean(int* array_original,int arraySize)
{
	//return (int)(calcMax(array_original,arraySize) * 0.2 + calcMin(array_original,arraySize) * 0.5);
	//int sum = 0;
	//for(int i = 0; i < arraySize; i++)
	//	sum += array_original[i];
	//return int(sum / arraySize);
	int min = calcMin(array_original,arraySize);
	int max = calcMax(array_original,arraySize);
	min = min < 0 ? 0:min;
	max = max < 0 ? 0:max;
	return (int)(min * 0.9 + max * 0.1);//min * 0.8 + max * 0.2;
}
int calcMedian(int* array_original,int arraySize)
{
	if(!array_original || arraySize <= 1)
		return 0;
	//return (int)(calcMax(array_,arraySize) * 0.2 + calcMin(array_,arraySize) * 0.5);
	int* array_;
	array_ = new int[arraySize];
	for(int k = 0; k < arraySize; k++)
	{
		array_[k] = array_original[k];
	}
	int curindex = arraySize;
	while(curindex > 0)
	{
		int i,j;
		int max = 0;
		for(i = (arraySize - curindex); i < arraySize; i++)
			if(array_[i] > max)
			{
				max = array_[i];
				j = i;
			}
		int temp;
		temp = array_[arraySize - curindex];
		array_[arraySize - curindex] = array_[j];
		array_[j] = temp;
		curindex--;
	}
	int median = array_[int(arraySize /2)];
	delete []array_;
	return median;//0;
}
void lower(int* array_,int arraySize)
{
	//int start = getFirstIndex(array_,arraySize,1),firstindex,firstpeak,tmp;
	//GetPeak_D2U(array_,arraySize,start,tmp,firstindex,firstpeak);
	int mean = calcMax(array_,arraySize) / 3;//calcMean(array_,arraySize);//modified by kojy-0721
	for(int i = 0; i < arraySize; i++)
	{
		if(array_[i] >= mean) 
			array_[i] -= mean;
		else
			array_[i] = 0;
	}
	return;
}
int GetIndexofNextIncrease(int* array_,int arraySize,int start)
{
	//for(int i = start; i < arraySize; i++)
	//{

	//}
	return start;
}
bool isHolePoint(int* array_,int arraySize,int pointindex)
{
	bool left = true,
		 right = true;
	for(int i = pointindex; i < arraySize - 1; i++)
	{
		if(array_[pointindex] < array_[i + 1])
			break;
		if(array_[pointindex] > array_[i + 1])
		{
			right = false;
			break;
		}
	}
	for(int i = pointindex; i >= 1; i--)
	{
		if(array_[pointindex] < array_[i - 1])
			break;
		if(array_[pointindex] > array_[i - 1])
		{
			left = false;
			break;
		}
	}
	if(left && right)
		return true;
	else
		return false;
}
int  GetIndexofHole(int* array_,int arraySize,int start,bool direction_D2U)
{
	//exception handling//
	if(start >= arraySize &&  direction_D2U)
		return arraySize - 1;
	if(start <= 0 &&  !direction_D2U)
		return 0;
	//Normal Action//
	int holeindex;//
	int pole;//
	bool isHole;//
	if(direction_D2U)
	{
		holeindex = start + 1;
		pole = 1;
		isHole = false;
		for(int i = holeindex; i < arraySize; i++)
		{
			isHole = isHolePoint(array_,arraySize,i);
			if(isHole)
				return i;
		}
		return holeindex;
	}
	else{
		holeindex = start - 1;
		pole = 1;
		isHole = false;
		for(int i = holeindex; i > 0; i--)
		{
			isHole = isHolePoint(array_,arraySize,i);
			if(isHole)
				return i;
		}
		return holeindex;
	}
}
void GetPeak_D2U(int* array_,int arraySize,int start,int& end,int& peakindex,int& peakvalue)
{
	peakindex = -1;
	peakvalue = 0;
	int min = calcMin(array_,arraySize);
	end = GetIndexofHole(array_,arraySize,start,true);
	bool increasing = true;
	for(int i = start; i < (arraySize - 2); i++)
	{
		if(array_[i] > peakvalue)
		{
			peakvalue = array_[i];
			peakindex = i;
		}
		if(array_[i] != min && array_[i + 1] == min)
		{
			end = i + 1;
			break;
		}
	}
	return;
}
void getFirstLock(int* array_,int arraySize,int& start,int& end)
{
	start = getFirstIndex(array_,arraySize,1);
	if(array_[0] > 1) start = 0;//kjy-todo-2015.4.26
	int min = calcMin(array_,arraySize);
	int oldmax = int(calcMax(array_,arraySize) * 1.5);
	int upperlimit = (oldmax + start) < arraySize ? (oldmax + start):(arraySize - 1);
	int count = 0;
	//try exception
	if(start > (arraySize - 3))
	{
		end = start;
		return;
	}
	//
#if 0
	/****************************************************************************************************************************************

				*      *         **** 
			  *   *  *  *       *    *
	A---->	**     **    *    **     **
						 ****
	*****************************************************************************************************************************************/
	int peak = 0,peakindex = 0,start_ = start,end_ = start,firstindex,firstpeak;
	GetPeak_D2U(array_,arraySize,start_,end_,firstindex,firstpeak);
	while((end_ - start) * 2.2 < oldmax || peak * 2.2 < firstpeak)
	{
		if(start_ == end_)
			break;
		int i = end_ + 1;
		while(array_[i] == array_[end_] && i < arraySize - 1)
		{
			i++;
			if(i == (arraySize - 1))
			{
				end = upperlimit;
				return;
			}
		}
		start_ = end_;
		GetPeak_D2U(array_,arraySize,i,end_,peakindex,peak);
		
	}
	end = GetIndexofHole(array_,arraySize,peakindex,true);
	return;
#else
	int firstpeak,firstindex,secondpeak,secondindex,thirdpeak,thirdindex,tmp1,tmp2;
	GetPeak_D2U(array_,arraySize,start,tmp1,firstindex,firstpeak);
	GetPeak_D2U(array_,arraySize,tmp1,tmp2,secondindex,secondpeak);
	GetPeak_D2U(array_,arraySize,tmp2,end,thirdindex,thirdpeak);
	//if(secondpeak * 1.9 < firstpeak || )
	if(secondpeak * 1.6 < firstpeak &&
	   secondpeak < thirdpeak && 
	   abs(oldmax + firstindex - secondindex) < abs(oldmax + firstindex - thirdindex))
	{
		end = thirdindex + 2;
		return;
	}
	else
	{
		end = secondindex + 2;
		return;
	}
#if 0
	end = -1;
	//---Up to Down----//
	for(int i = upperlimit; i > start; i--)
		if((array_[i] == min && array_[i - 1] != min)||
		   (array_[i - 1] == min && array_[i] != min))
		{
			end = i;//2--> redundant
			break;
			////}
		}
	//--Down to Up---///
	for(int i = (start + 1); i < upperlimit; i++)
		if(array_[i] != min && array_[i + 1] == min)
		{
			count++;
			if(count == 2 && end != -1)
			{
				end += i + 2;
				end /= 2;
				return;
			}
		}
#endif
#endif
	end = upperlimit;
	return;
}

void GetPeak_U2D(int* array_,int arraySize,int start,int& end,int& peakindex,int& peakvalue)
{
	peakindex = 2;
	peakvalue = 0;
	int min = calcMin(array_,arraySize);
	end = GetIndexofHole(array_,arraySize,start,false);
	for(int i = start; i > 2; i--)
	{
		if(array_[i] > peakvalue)
		{
			peakvalue = array_[i];
			peakindex = i;
		}
		if(array_[i] != min && array_[i - 1] == min)
		{
			end = i - 1;
			break;
		}
	}
	return;
}
void getLastLock(int* array_,int arraySize,int& start,int& end)
{
	end = getLastIndex(array_,arraySize,1);
	end = end == (arraySize - 1) ? end:(end + 1);
	if(array_[arraySize - 1] > 1) end = arraySize - 1;//kjy-todo-2015.4.26
	int min = calcMin(array_,arraySize);
	int oldmax = int(calcMax(array_,arraySize) * 1.5);
	int lowerlimit = (end - oldmax) < 0 ? 0:(end - oldmax);
	int count = 0;
	//try exception
	if(end < 3)
	{
		start = end;
		return;
	}

	int firstpeak,firstindex,secondpeak,secondindex,thirdpeak,thirdindex,tmp1 = 0,tmp2 = 0;
	GetPeak_U2D(array_,arraySize,end,tmp1,firstindex,firstpeak);
	GetPeak_U2D(array_,arraySize,tmp1,tmp2,secondindex,secondpeak);
	GetPeak_U2D(array_,arraySize,tmp2,start,thirdindex,thirdpeak);
	if(secondpeak * 1.6 < firstpeak &&
		secondpeak < thirdpeak &&
		abs(secondindex - lowerlimit) < abs(thirdindex - lowerlimit))
	{
		start = thirdindex - 2;
		return;
	}
	else
	{
		start = secondindex - 2;
		return;
	}
	start = lowerlimit;
	return;
#if OLD_GLL
	end = getLastIndex(array_,arraySize,1);
	end = end == (arraySize - 1) ? end:(end + 1);
	if(array_[arraySize - 1] > 1) end = arraySize - 1;//kjy-todo-2015.4.26
	int min = calcMin(array_,arraySize);
	int oldmax = calcMax(array_,arraySize) * 1.5;
	int lowerlimit = (end - oldmax) < 0 ? 0:(end - oldmax);
	int count = 0;
	//try exception
	if(end < 3)
	{
		start = end;
		return;
	}
	//
#if 1
	int firstpeak,firstindex,secondpeak,secondindex,thirdpeak,thirdindex,tmp1 = 0,tmp2 = 0;
	GetPeak_U2D(array_,arraySize,end,tmp1,firstindex,firstpeak);
	GetPeak_U2D(array_,arraySize,tmp1,tmp2,secondindex,secondpeak);
	GetPeak_U2D(array_,arraySize,tmp2,start,thirdindex,thirdpeak);
	if(secondpeak * 1.6 < firstpeak)
	{
		start = thirdindex - 2;
		return;
	}
	else
	{
		start = secondindex - 2;
		return;
	}
#else 
	start = -1;
	//---Up to Down----/
	for(int i = lowerlimit; i < end; i++)
		if((array_[i - 1] == min && array_[i] != min) ||
		   (array_[i - 1] != min && array_[i] == min))
		{
			start = i;//1--> redundant
			break;
		}
	//---Down to Up-----/
	for(int i = (end - 1); i > lowerlimit; i--)
		if((array_[i] != min && array_[i - 1] == min) ||
			(array_[i] == min && array_[i - 1] != min))
		{
			count++;
			if(count == 2 && start != -1)
			{
				start += i - 2;//1--> redundant
				start /=2;
				return;
			}
		}
#endif
	start = lowerlimit;
	return;
#endif
}
int getFirstIndex(int* array_,int arraySize,int value)
{
	for(int i = 1; i < arraySize; i++)
		if(array_[i - 1] >= value) return i - 1;
		else if(array_[i - 1] < value && array_[i] >= value) return i - 1;
	return 0;
}
int getLastIndex(int* array_,int arraySize,int value)
{
	for(int i = (arraySize - 2); i >= 0; i--)
		if(array_[i + 1] >= value) return i + 1;
		else if(array_[i] >= value && array_[i + 1] < value) return i + 1;
		return arraySize - 1;
}
void calcSize(int* array_,int arraySize,int& firstindex,int& lastindex)
{
	//blur_(array_,arraySize);
	//Gradient_(array_,arraySize);
	int min = calcMin(array_,arraySize);
	min = min < 0 ? 0: min;
	int max = calcMax(array_,arraySize);
	int mean = min;//min * 0.9 + max * 0.1;//calcMean(array_,arraySize);//(int)(float(max - min) / 1.4142);//
	int maxindex = getFirstIndex(array_,arraySize,max);//kjy-todo-2015.3.27
	//int oldfirstindex = getFirstIndex(array_,arraySize,mean - 1),oldlastindex = getLastIndex(array_,arraySize,mean - 1);

	for(int i = maxindex; i < arraySize - 1; i++)
		if((array_[i] >= mean && array_[i + 1] <= mean) || i == arraySize - 2) 
		{
			lastindex = i;
			break;
		}
	for(int j = maxindex; j > 0; j--)
		if((array_[j] >= mean && array_[j - 1] <= mean) || j == 1) 
		{
			firstindex = j;
			break;
		}
	return;
}
int calcGroupCount(int* array_,int arraySize,group_& biggest)
{
	blur_(array_,arraySize);
	//blur_(array_,arraySize);
	//blur_(array_,arraySize);
	//Gradient_(array_,arraySize);
	int min = calcMin(array_,arraySize);

	int groupcount = 0;
	bool flag = false;
	int previousfirstindex = 0;
	int previouslastindex = 0;
	int firstindex = 0;
	int lastindex = 0;
	bool isfirstgroup = true;

	for(int i = 0; i < arraySize; i++)
	{
		if(!flag && array_[i] > min)//처음으로 peak가 확인되는 시점에서
		{
			flag = true;//어떤 그룹을 지나는 전기간 true로 설정되여있다.
			firstindex = i;
			if((firstindex - previouslastindex) < USER_DEFINED_STRING_SIZE) 
				firstindex = previousfirstindex;
			else
				groupcount++;
		}
		if((flag && array_[i] == min) || (flag && i == (arraySize - 1)))//peak가 끝나는 시점에서
		{
			flag = false;
			lastindex = i;
			if(isfirstgroup)
			{
				biggest.setgroup_(firstindex,lastindex);
				isfirstgroup = false;
			}
			previousfirstindex = firstindex;
			previouslastindex = lastindex;
			if(biggest.size < (lastindex - firstindex))
				biggest.setgroup_(firstindex,lastindex);
		}
	}
	return groupcount;
}
int calcGroupSize(int* array_,int arraySize,int firstindex,int lastindex)
{
	if(!array_) return 0;
	if(firstindex < 0) firstindex = 0;
	if(lastindex >= arraySize) lastindex = arraySize;
	int size = 0;
	for(int i = firstindex; i < lastindex; i++)
	{
		size += array_[i];
	}
	return size;
}
int calcGroupCount_(int* array_,int arraySize,group_& biggest)
{
	blur_(array_,arraySize);
	//blur_(array_,arraySize);
	//blur_(array_,arraySize);
	//Gradient_(array_,arraySize);
	int min = calcMin(array_,arraySize);

	int groupcount = 0;
	bool flag = false;
	int previousfirstindex = 0;
	int previouslastindex = 0;
	int firstindex = 0;
	int lastindex = 0;
	bool isfirstgroup = true;

	for(int i = 0; i < arraySize; i++)
	{
		if(!flag && array_[i] > min)//처음으로 peak가 확인되는 시점에서
		{
			flag = true;//어떤 그룹을 지나는 전기간 true로 설정되여있다.
			firstindex = i;
			if((firstindex - previouslastindex) < USER_DEFINED_STRING_SIZE) 
				firstindex = previousfirstindex;
			else
				groupcount++;
		}
		if((flag && array_[i] == min) || (flag && i == (arraySize - 1)))//peak가 끝나는 시점에서
		{
			flag = false;
			lastindex = i;
			if(isfirstgroup)
			{
				biggest.setgroup_(firstindex,lastindex);
				biggest.setSize(calcGroupSize(array_,arraySize,firstindex,lastindex));
				isfirstgroup = false;
			}
			previousfirstindex = firstindex;
			previouslastindex = lastindex;
			if(biggest.size < calcGroupSize(array_,arraySize,firstindex,lastindex))
				biggest.setgroup_(firstindex,lastindex);
		}
	}
	return groupcount;
}
//kjy-todo-2015.3.21-it's for filtering non-string outputs
int calcGroupCount_t(int* array_,int arraySize)
{
	blur_(array_,arraySize);
	blur_(array_,arraySize);
	blur_(array_,arraySize);
	//Gradient_(array_,arraySize);
	int min = calcMin(array_,arraySize);
	int groupcount = 0;
	bool flag = false;
//	int previousfirstindex = 0;
//	int previouslastindex = 0;
//	int firstindex = 0;
//	int lastindex = 0;
//	bool isfirstgroup = true;

	for(int i = 0; i < arraySize - 1; i++)
	{
		if(array_[i] == 0 && array_[i + 1] != 0)
			groupcount++;
	}
	return groupcount;
}


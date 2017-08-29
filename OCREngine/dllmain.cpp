// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "OCR.h"

BYTE *g_pDicData[DIC_COUNT];
BYTE *g_pLockSymDic;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			TCHAR	ExePath[MAX_PATH], DicPath[MAX_PATH];
			TCHAR	temp[MAX_PATH];
			int		i, fh, nSize;

			GetModuleFileName(hModule, ExePath, MAX_PATH);
			ExePath[_tcsrchr(ExePath, '\\')-ExePath] = 0;

			for( i = 0; i < DIC_COUNT; i++)
			{
				g_pDicData[i] = NULL;
				wcscpy(temp, ExePath);
				wcscat( temp, L"\\DIC\\sz_ocr_" );
				wsprintf(DicPath, L"%s%d", temp,i);
				wcscat(DicPath, L".dic");

				fh = _wopen(DicPath, _O_RDONLY | _O_BINARY, _S_IREAD);
				if(fh != -1)
				{
					nSize = _filelength(fh);
					g_pDicData[i] = new BYTE[nSize];
					_read(fh, g_pDicData[i], nSize);
					_close(fh);
				}
			}

			g_pLockSymDic = NULL;

			wcscpy(DicPath, ExePath);
			wcscat(DicPath, L"\\DIC\\sz_sym.dic" );

			fh = _wopen(DicPath , _O_RDONLY | _O_BINARY, _S_IREAD);
			if( fh != -1 )
			{
				nSize = _filelength(fh);
				g_pLockSymDic = new BYTE[nSize];
				_read(fh, g_pLockSymDic, nSize);
				_close(fh);
			}
		}

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		for (int i = 0; i < DIC_COUNT; i++)
		{
			if (g_pDicData[i])
			{
				delete []g_pDicData[i];
			}
		}

		if (g_pLockSymDic)
		{
			delete []g_pLockSymDic;
		}
		break;
	}
	return TRUE;
}


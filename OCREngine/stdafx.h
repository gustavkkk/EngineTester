// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef MSVC_OCR//kjy-todo-2015.6.17

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <tchar.h>
#include "global.h"

#define OCR_PIXEL_MIN 0
#define OCR_PIXEL_MAX 255

#else//kjy-todo-2015.6.17

#include "global.h"

#endif
// TODO: reference additional headers your program requires here

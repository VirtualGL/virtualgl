/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2010, 2014, 2017-2019 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#ifndef __VGLUTIL_H__
#define __VGLUTIL_H__

#ifdef _WIN32
	#include <windows.h>
	#define sleep(t)  Sleep((t) * 1000)
	#define usleep(t)  Sleep((t) / 1000)
#else
	#include <sys/time.h>
	#include <unistd.h>
	#define stricmp  strcasecmp
	#define strnicmp  strncasecmp
#endif
#include "vglinline.h"

#ifdef _MSC_VER
#define snprintf(str, n, format, ...) \
	_snprintf_s(str, n, _TRUNCATE, format, __VA_ARGS__)
#endif

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif

#define IS_POW2(x)  (((x) & (x - 1)) == 0)

#ifdef sgi
#define _SC_NPROCESSORS_CONF  _SC_NPROC_CONF
#endif

static INLINE int NumProcs(void)
{
	#ifdef _WIN32

	DWORD_PTR ProcAff, SysAff, i;  int count = 0;
	if(!GetProcessAffinityMask(GetCurrentProcess(), &ProcAff, &SysAff))
		return 1;
	for(i = 0; i < sizeof(long *) * 8; i++)
		if(ProcAff & (1LL << i)) count++;
	return count;

	#elif defined(__APPLE__)

	return 1;

	#else

	long count = 1;
	if((count = sysconf(_SC_NPROCESSORS_CONF)) != -1) return (int)count;
	else return 1;

	#endif
}

#define BYTESWAP(i) \
	( (((i) & 0xff000000) >> 24) | \
	  (((i) & 0x00ff0000) >>  8) | \
	  (((i) & 0x0000ff00) <<  8) | \
	  (((i) & 0x000000ff) << 24) )

#define BYTESWAP24(i) \
	( (((i) & 0xff0000) >> 16) | \
	   ((i) & 0x00ff00)        | \
	  (((i) & 0x0000ff) << 16) )

#define BYTESWAP16(i) \
	( (((i) & 0xff00) >> 8) | \
	  (((i) & 0x00ff) << 8) )

static INLINE int LittleEndian(void)
{
	unsigned int value = 1;
	unsigned char *ptr = (unsigned char *)(&value);
	if(ptr[0] == 1 && ptr[3] == 0) return 1;
	else return 0;
}

#ifdef _WIN32

static INLINE double GetTime(void)
{
	LARGE_INTEGER frequency, time;
	if(QueryPerformanceFrequency(&frequency) != 0)
	{
		QueryPerformanceCounter(&time);
		return (double)time.QuadPart / (double)frequency.QuadPart;
	}
	else return (double)GetTickCount() * 0.001;
}

#else

static INLINE double GetTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, (struct timezone *)NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

#endif  /* _WIN32 */

#endif  /* __VGLUTIL_H__ */

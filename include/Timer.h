// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2014, 2018-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>
#include "vglinline.h"


namespace util
{
	class Timer
	{
		public:

			Timer(void) : t1(0.0)
			{
				#ifdef _WIN32
				highRes = false;  tick = 0.001;
				LARGE_INTEGER frequency;
				if(QueryPerformanceFrequency(&frequency) != 0)
				{
					tick = (double)1.0 / (double)(frequency.QuadPart);
					highRes = true;
				}
				#endif
			}

			void start(void)
			{
				t1 = time();
			}

			double time(void)
			{
				#ifdef _WIN32

				if(highRes)
				{
					LARGE_INTEGER Time;
					QueryPerformanceCounter(&Time);
					return (double)(Time.QuadPart) * tick;
				}
				else
					return (double)GetTickCount() * tick;

				#else

				struct timeval tv;
				gettimeofday(&tv, (struct timezone *)NULL);
				return (double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.000001;

				#endif
			}

			double elapsed(void)
			{
				return time() - t1;
			}

		private:

			#ifdef _WIN32
			bool highRes;  double tick;
			#endif
			double t1;
	};
}

#endif  // __TIMER_H__

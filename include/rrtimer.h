/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#ifndef __RRTIMER_H__
#define __RRTIMER_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

class rrtimer
{
	public:

		rrtimer(void) : t1(0.0)
		{
			#ifdef _WIN32
			highres=false;  tick=0.001;
			LARGE_INTEGER Frequency;
			if(QueryPerformanceFrequency(&Frequency)!=0)
			{
				tick=(double)1.0/(double)(Frequency.QuadPart);
				highres=true;
			}
			#endif
		}

		void start(void)
		{
			t1=time();
		}

		double time(void)
		{
			#ifdef _WIN32
			if(highres)
			{
				LARGE_INTEGER Time;
				QueryPerformanceCounter(&Time);
				return((double)(Time.QuadPart)*tick);
			}
			else
				return((double)GetTickCount()*tick);
			#else
			struct timeval __tv;
			gettimeofday(&__tv, (struct timezone *)NULL);
			return((double)(__tv.tv_sec)+(double)(__tv.tv_usec)*0.000001);
			#endif
		}

		double elapsed(void)
		{
			return time()-t1;
		}

	private:

		#ifdef _WIN32
		bool highres;  double tick;
		#endif
		double t1;
};

#endif


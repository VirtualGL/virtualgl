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

#ifndef __RRPROFILER_H__
#define __RRPROFILER_H__

#include <stdlib.h>
#include "rrtimer.h"
#include "rrlog.h"

class rrprofiler
{
	public:

	rrprofiler(const char *_name="Profiler", double _interval=2.0) : name(_name),
		interval(_interval), mbytes(0.0), mpixels(0.0), totaltime(0.0), start(0.0),
		frames(0)
	{
		profile=false;  char *ev=NULL;
		if((ev=getenv("RRPROFILE"))!=NULL && !strncmp(ev, "1", 1))
			profile=true;
		if((ev=getenv("VGL_PROFILE"))!=NULL && !strncmp(ev, "1", 1))
			profile=true;
	}

	void setname(const char *_name)
	{
		if(_name) name=_name;
	}

	void startframe(void)
	{
		if(!profile) return;
		start=timer.time();
	}

	void endframe(long pixels, long bytes, double incframes)
	{
		if(!profile) return;
		if(start)
		{
			totaltime+=timer.time()-start;
			if(pixels) mpixels+=(double)pixels/1000000.;
			if(bytes) mbytes+=(double)bytes/1000000.;
			if(incframes) frames+=incframes;
		}
		if(totaltime>interval)
		{
			rrout.print("%s  ", name);
			if(mpixels) rrout.print("- %.2f Mpixels/sec", mpixels/totaltime);
			if(mbytes) rrout.print("- %.2f Mbits/sec", mbytes*8.0/totaltime);
			if(frames) rrout.print("- %.2f frames/sec", frames/totaltime);
			rrout.PRINT("\n");
			totaltime=0.;  mpixels=0.;  frames=0.;  mbytes=0.;
		}
	}

	private:

	const char *name;
	double interval;
	double mbytes, mpixels, totaltime, start, frames;
	bool profile;
	rrtimer timer;
};

#endif

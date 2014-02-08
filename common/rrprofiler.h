/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

#ifndef __RRPROFILER_H__
#define __RRPROFILER_H__

#include <stdlib.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#define strdup _strdup
#endif
#include "Timer.h"
#include "Log.h"

class rrprofiler
{
	public:

	rrprofiler(const char *_name="Profiler", double _interval=2.0) :
		interval(_interval), mbytes(0.0), mpixels(0.0), totaltime(0.0), start(0.0),
		frames(0), lastframe(0.0)
	{
		profile=false;  char *ev=NULL;
		setname(_name);  freestr=false;
		if((ev=getenv("RRPROFILE"))!=NULL && !strncmp(ev, "1", 1))
			profile=true;
		if((ev=getenv("VGL_PROFILE"))!=NULL && !strncmp(ev, "1", 1))
			profile=true;
	}

	~rrprofiler(void)
	{
		if(name && freestr) free(name);
	}

	void setname(char *_name)
	{
		if(_name) {name=strdup(_name);  freestr=true;}
	}

	void setname(const char *_name)
	{
		if(_name) name=(char *)_name;
	}

	void startframe(void)
	{
		if(!profile) return;
		start=timer.time();
	}

	void endframe(long pixels, long bytes, double incframes)
	{
		if(!profile) return;
		double now=timer.time();
		if(start!=0.0)
		{
			totaltime+=now-start;
			if(pixels) mpixels+=(double)pixels/1000000.;
			if(bytes) mbytes+=(double)bytes/1000000.;
			if(incframes!=0.0) frames+=incframes;
		}
		if(lastframe==0.0) lastframe=now;
		if(totaltime>interval || (now-lastframe)>interval)
		{
			char temps[256];  size_t i=0;
			snprintf(&temps[i], 255-i, "%s  ", name);  i=strlen(temps);
			if(mpixels) {snprintf(&temps[i], 255-i, "- %7.2f Mpixels/sec", mpixels/totaltime);  i=strlen(temps);}
			if(frames) {snprintf(&temps[i], 255-i, "- %7.2f fps", frames/totaltime);  i=strlen(temps);}
			if(mbytes) {snprintf(&temps[i], 255-i, "- %7.2f Mbits/sec (%.1f:1)", mbytes*8.0/totaltime, mpixels*3./mbytes);  i=strlen(temps);}
			vglout.PRINT("%s\n", temps);
			totaltime=0.;  mpixels=0.;  frames=0.;  mbytes=0.;  lastframe=now;
		}
	}

	private:

	char *name;
	double interval;
	double mbytes, mpixels, totaltime, start, frames, lastframe;
	bool profile;
	Timer timer;
	bool freestr;
};

#endif

// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2014, 2021 D. R. Commander
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

#include "Profiler.h"
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#define snprintf  _snprintf
#define strdup  _strdup
#endif
#include "Timer.h"
#include "Log.h"

using namespace common;


Profiler::Profiler(const char *name_, double interval_) : interval(interval_),
	mbytes(0.0), mpixels(0.0), totalTime(0.0), start(0.0), frames(0),
	lastFrame(0.0)
{
	profile = false;  char *ev = NULL;
	setName(name_);  freestr = false;
	if((ev = getenv("RRPROFILE")) != NULL && !strncmp(ev, "1", 1))
		profile = true;
	if((ev = getenv("VGL_PROFILE")) != NULL && !strncmp(ev, "1", 1))
		profile = true;
}


Profiler::~Profiler(void)
{
	if(freestr) free(name);
}


void Profiler::setName(char *name_)
{
	if(name_)
	{
		name = strdup(name_);  freestr = true;
	}
}


void Profiler::setName(const char *name_)
{
	if(name_) name = (char *)name_;
}


void Profiler::startFrame(void)
{
	if(!profile) return;
	start = timer.time();
}


void Profiler::endFrame(long pixels, long bytes, double incFrames)
{
	if(!profile) return;
	double now = timer.time();
	if(start != 0.0)
	{
		totalTime += now - start;
		if(pixels) mpixels += (double)pixels / 1000000.;
		if(bytes) mbytes += (double)bytes / 1000000.;
		if(incFrames != 0.0) frames += incFrames;
	}
	if(lastFrame == 0.0) lastFrame = now;
	if(totalTime > interval || (now - lastFrame) > interval)
	{
		char temps[256];  size_t i = 0;
		snprintf(&temps[i], 255 - i, "%s  ", name);  i = strlen(temps);
		if(mpixels)
		{
			snprintf(&temps[i], 255 - i, "- %7.2f Mpixels/sec", mpixels / totalTime);
			i = strlen(temps);
		}
		if(frames)
		{
			snprintf(&temps[i], 255 - i, "- %7.2f fps", frames / totalTime);
			i = strlen(temps);
		}
		if(mbytes)
		{
			snprintf(&temps[i], 255 - i, "- %7.2f Mbits/sec (%.1f:1)",
				mbytes * 8.0 / totalTime, mpixels * 3. / mbytes);
			i = strlen(temps);
		}
		vglout.PRINT("%s\n", temps);
		totalTime = 0.;  mpixels = 0.;  frames = 0.;  mbytes = 0.;
		lastFrame = now;
	}
}

// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2010-2011, 2014, 2019-2021, 2024 D. R. Commander
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

#include "X11Trans.h"
#include "Timer.h"
#include "fakerconfig.h"
#include "vglutil.h"
#include "Log.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

extern "C" void _vgl_disableFaker(void);
extern "C" void _vgl_enableFaker(void);

using namespace util;
using namespace common;
using namespace server;


X11Trans::X11Trans(void) : thread(NULL), deadYet(false)
{
	if(fconfig.sync) nFrames = 1;
	for(int i = 0; i < nFrames; i++) frames[i] = NULL;
	thread = new Thread(this);
	thread->start();
	profBlit.setName("Blit      ");
	profTotal.setName("Total     ");
	if(fconfig.verbose) fbx_printwarnings(vglout.getFile());
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
	// NOTE: Without this line, helgrind reports a data race on the class
	// instance address in the destructor (false positive?)
	ANNOTATE_BENIGN_RACE_SIZED(this, sizeof(X11Trans *), );
	#endif
}


void X11Trans::run(void)
{
	Timer timer, sleepTimer;  double err = 0.;  bool first = true;

	try
	{
		_vgl_disableFaker();

		while(!deadYet)
		{
			FBXFrame *f;  void *ftemp = NULL;

			q.get(&ftemp);  f = (FBXFrame *)ftemp;  if(deadYet) return;
			if(!f) THROW("Queue has been shut down");
			ready.signal();
			profBlit.startFrame();
			f->redraw();
			profBlit.endFrame(f->hdr.width * f->hdr.height, 0, 1);

			profTotal.endFrame(f->hdr.width * f->hdr.height, 0, 1);
			profTotal.startFrame();

			if(fconfig.flushdelay > 0.)
			{
				long usec = (long)(fconfig.flushdelay * 1000000.);
				if(usec > 0) usleep(usec);
			}
			if(fconfig.fps > 0.)
			{
				double elapsed = timer.elapsed();
				if(first) first = false;
				else
				{
					if(elapsed < 1. / fconfig.fps)
					{
						sleepTimer.start();
						long usec = (long)((1. / fconfig.fps - elapsed - err) * 1000000.);
						if(usec > 0) usleep(usec);
						double sleepTime = sleepTimer.elapsed();
						err = sleepTime - (1. / fconfig.fps - elapsed - err);
						if(err < 0.) err = 0.;
					}
				}
				timer.start();
			}

			f->signalComplete();
		}

	}
	catch(std::exception &e)
	{
		if(thread) thread->setError(e);
		ready.signal();
		_vgl_enableFaker();
		throw;
	}
	_vgl_enableFaker();
}


FBXFrame *X11Trans::getFrame(Display *dpy, Window win, int width, int height)
{
	FBXFrame *f = NULL;

	if(thread) thread->checkError();
	{
		CriticalSection::SafeLock l(mutex);

		int index = -1;
		for(int i = 0; i < nFrames; i++)
			if(!frames[i] || (frames[i] && frames[i]->isComplete()))
				index = i;
		if(index < 0) THROW("No free buffers in pool");
		if(!frames[index])
			frames[index] = new FBXFrame(dpy, win, NULL, fconfig.sync);
		f = frames[index];  f->waitUntilComplete();
	}

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.x = hdr.y = 0;
	hdr.width = hdr.framew = width;
	hdr.height = hdr.frameh = height;
	f->init(hdr);
	return f;
}


bool X11Trans::isReady(void)
{
	if(thread) thread->checkError();
	return q.items() <= 0;
}


void X11Trans::synchronize(void)
{
	ready.wait();
}


static void __X11Trans_spoilfct(void *f)
{
	if(f) ((FBXFrame *)f)->signalComplete();
}


void X11Trans::sendFrame(FBXFrame *f, bool sync)
{
	if(thread) thread->checkError();
	if(sync)
	{
		profBlit.startFrame();
		f->redraw();
		f->signalComplete();
		profBlit.endFrame(f->hdr.width * f->hdr.height, 0, 1);
		ready.signal();
	}
	else q.spoil((void *)f, __X11Trans_spoilfct);
}

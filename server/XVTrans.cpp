// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009-2011, 2014, 2019-2021 D. R. Commander
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

#include "XVTrans.h"
#include "vglutil.h"
#include "Timer.h"
#include "fakerconfig.h"
#include "Log.h"
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#endif

using namespace util;
using namespace common;
using namespace server;


XVTrans::XVTrans(void) : thread(NULL), deadYet(false)
{
	for(int i = 0; i < NFRAMES; i++) frames[i] = NULL;
	thread = new Thread(this);
	thread->start();
	profXV.setName("XV        ");
	profTotal.setName("Total     ");
	if(fconfig.verbose) fbxv_printwarnings(vglout.getFile());
	#ifdef USEHELGRIND
	ANNOTATE_BENIGN_RACE_SIZED(&deadYet, sizeof(bool), );
	// NOTE: Without this line, helgrind reports a data race on the class
	// instance address in the destructor (false positive?)
	ANNOTATE_BENIGN_RACE_SIZED(this, sizeof(XVTrans *), );
	#endif
}


void XVTrans::run(void)
{
	Timer timer, sleepTimer;  double err = 0.;  bool first = true;

	try
	{
		while(!deadYet)
		{
			XVFrame *f;  void *ftemp = NULL;

			q.get(&ftemp);  f = (XVFrame *)ftemp;  if(deadYet) return;
			if(!f) throw("Queue has been shut down");
			ready.signal();
			profXV.startFrame();
			f->redraw();
			profXV.endFrame(f->hdr.width * f->hdr.height, 0, 1);

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
		ready.signal();  throw;
	}
}


XVFrame *XVTrans::getFrame(Display *dpy, Window win, int width, int height)
{
	XVFrame *f = NULL;

	if(thread) thread->checkError();
	{
		CriticalSection::SafeLock l(mutex);

		int index = -1;
		for(int i = 0; i < NFRAMES; i++)
			if(!frames[i] || (frames[i] && frames[i]->isComplete()))
				index = i;
		if(index < 0) THROW("No free buffers in pool");
		if(!frames[index])
			frames[index] = new XVFrame(dpy, win);
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


bool XVTrans::isReady(void)
{
	if(thread) thread->checkError();
	return q.items() <= 0;
}


void XVTrans::synchronize(void)
{
	ready.wait();
}


static void __XVTrans_spoilfct(void *f)
{
	if(f) ((XVFrame *)f)->signalComplete();
}


void XVTrans::sendFrame(XVFrame *f, bool sync)
{
	if(thread) thread->checkError();
	if(sync)
	{
		profXV.startFrame();
		f->redraw();
		f->signalComplete();
		profXV.endFrame(f->hdr.width * f->hdr.height, 0, 1);
		ready.signal();
	}
	else q.spoil((void *)f, __XVTrans_spoilfct);
}

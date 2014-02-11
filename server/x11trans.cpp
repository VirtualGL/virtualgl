/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2011, 2014 D. R. Commander
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

#include "x11trans.h"
#include "Timer.h"
#include "fakerconfig.h"
#include "vglutil.h"
#include "Log.h"


x11trans::x11trans(void) : _t(NULL), _deadyet(false)
{
	for(int i=0; i<NFRAMES; i++) _frame[i]=NULL;
	errifnot(_t=new Thread(this));
	_t->start();
	_prof_blit.setName("Blit      ");
	_prof_total.setName("Total     ");
	if(fconfig.verbose) fbx_printwarnings(vglout.getFile());
}


void x11trans::run(void)
{
	Timer t, sleept;  double err=0.;  bool first=true;

	try
	{
 		while(!_deadyet)
		{
			FBXFrame *f;  void *ftemp=NULL;
			_q.get(&ftemp);  f=(FBXFrame *)ftemp;  if(_deadyet) return;
			if(!f) _throw("Queue has been shut down");
			_ready.signal();
			_prof_blit.startFrame();
			f->redraw();
			_prof_blit.endFrame(f->hdr.width*f->hdr.height, 0, 1);

			_prof_total.endFrame(f->hdr.width*f->hdr.height, 0, 1);
			_prof_total.startFrame();

			if(fconfig.flushdelay>0.)
			{
				long usec=(long)(fconfig.flushdelay*1000000.);
				if(usec>0) usleep(usec);
			}
			if(fconfig.fps>0.)
			{
				double elapsed=t.elapsed();
				if(first) first=false;
				else
				{
					if(elapsed<1./fconfig.fps)
					{
						sleept.start();
						long usec=(long)((1./fconfig.fps-elapsed-err)*1000000.);
						if(usec>0) usleep(usec);
						double sleeptime=sleept.elapsed();
						err=sleeptime-(1./fconfig.fps-elapsed-err);  if(err<0.) err=0.;
					}
				}
				t.start();
			}

			f->signalComplete();
		}

	}
	catch(Error &e)
	{
		if(_t) _t->setError(e);
		_ready.signal();  throw;
	}
}


FBXFrame *x11trans::getframe(Display *dpy, Window win, int w, int h)
{
	FBXFrame *f=NULL;
	if(_t) _t->checkError();
	{
		CS::SafeLock l(_mutex);
		int framei=-1;
		for(int i=0; i<NFRAMES; i++)
			if(!_frame[i] || (_frame[i] && _frame[i]->isComplete())) framei=i;
		if(framei<0) _throw("No free buffers in pool");
		if(!_frame[framei]) errifnot(_frame[framei]=new FBXFrame(dpy, win));
		f=_frame[framei];  f->waitUntilComplete();
	}
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	f->init(hdr);
	return f;
}


bool x11trans::ready(void)
{
	if(_t) _t->checkError();
	return(_q.items()<=0);
}


void x11trans::synchronize(void)
{
	_ready.wait();
}


static void __x11trans_spoilfct(void *f)
{
	if(f) ((FBXFrame *)f)->signalComplete();
}


void x11trans::sendframe(FBXFrame *f, bool sync)
{
	if(_t) _t->checkError();
	if(sync) 
	{
		_prof_blit.startFrame();
		f->redraw();
		f->signalComplete();
		_prof_blit.endFrame(f->hdr.width*f->hdr.height, 0, 1);
		_ready.signal();
	}
	else _q.spoil((void *)f, __x11trans_spoilfct);
}

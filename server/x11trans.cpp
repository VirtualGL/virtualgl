/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2011 D. R. Commander
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
#include "rrtimer.h"
#include "fakerconfig.h"
#include "rrutil.h"


x11trans::x11trans(void) : _t(NULL), _deadyet(false)
{
	for(int i=0; i<NFRAMES; i++) _frame[i]=NULL;
	errifnot(_t=new Thread(this));
	_t->start();
	_prof_blit.setname("Blit      ");
	_prof_total.setname("Total     ");
	if(fconfig.verbose) fbx_printwarnings(rrout.getfile());
}


void x11trans::run(void)
{
	rrtimer t, sleept;  double err=0.;  bool first=true;

	try
	{
 		while(!_deadyet)
		{
			rrfb *f=NULL;
			_q.get((void **)&f);  if(_deadyet) return;
			if(!f) _throw("Queue has been shut down");
			_ready.signal();
			_prof_blit.startframe();
			f->redraw();
			_prof_blit.endframe(f->_h.width*f->_h.height, 0, 1);

			_prof_total.endframe(f->_h.width*f->_h.height, 0, 1);
			_prof_total.startframe();

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

			f->complete();
		}

	}
	catch(rrerror &e)
	{
		if(_t) _t->seterror(e);
		_ready.signal();  throw;
	}
}


rrfb *x11trans::getframe(Display *dpy, Window win, int w, int h)
{
	rrfb *f=NULL;
	if(_t) _t->checkerror();
	{
	rrcs::safelock l(_mutex);
	int framei=-1;
	for(int i=0; i<NFRAMES; i++)
		if(!_frame[i] || (_frame[i] && _frame[i]->iscomplete())) framei=i;
	if(framei<0) _throw("No free buffers in pool");
	if(!_frame[framei]) errifnot(_frame[framei]=new rrfb(dpy, win));
	f=_frame[framei];  f->waituntilcomplete();
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
	if(_t) _t->checkerror();
	return(_q.items()<=0);
}


void x11trans::synchronize(void)
{
	_ready.wait();
}


static void __x11trans_spoilfct(void *f)
{
	if(f) ((rrfb *)f)->complete();
}


void x11trans::sendframe(rrfb *f, bool sync)
{
	if(_t) _t->checkerror();
	if(sync) 
	{
		_prof_blit.startframe();
		f->redraw();
		f->complete();
		_prof_blit.endframe(f->_h.width*f->_h.height, 0, 1);
		_ready.signal();
	}
	else _q.spoil((void *)f, __x11trans_spoilfct);
}

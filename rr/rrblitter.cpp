/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include "rrblitter.h"
#include "rrtimer.h"
#include "fakerconfig.h"

extern FakerConfig fconfig;

rrblitter::rrblitter(void) : _t(NULL), _deadyet(false)
{
	for(int i=0; i<NB; i++) _bmp[i]=NULL;
	errifnot(_t=new Thread(this));
	_t->start();
	_prof_blit.setname("Blit      ");
	_prof_total.setname("Total     ");
	_lastb=NULL;
	if(fconfig.verbose) fbx_printwarnings(rrout.getfile());
}

void rrblitter::run(void)
{
	rrtimer t, sleept;  double err=0.;  bool first=true;
//	rrfb *lastb=NULL;

	try {
 
	while(!_deadyet)
	{
		rrfb *b=NULL;
		_q.get((void **)&b);  if(_deadyet) return;
		if(!b) _throw("Queue has been shut down");
		_ready.signal();
		_prof_blit.startframe();
		b->redraw();
		_prof_blit.endframe(b->_h.width*b->_h.height, 0, 1);

		_prof_total.endframe(b->_h.width*b->_h.height, 0, 1);
		_prof_total.startframe();

		if(fconfig.fps.isset())
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

		b->complete();
//		lastb=b;
	}

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);
		_ready.signal();  throw;
	}
}

rrfb *rrblitter::getbitmap(Display *dpy, Window win, int w, int h, bool spoil)
{
	rrfb *b=NULL;
	if(!spoil) _ready.wait();
	if(_t) _t->checkerror();
	{
	rrcs::safelock l(_bmpmutex);
	int bmpi=-1;
	for(int i=0; i<NB; i++)
		if(!_bmp[i] || (_bmp[i] && _bmp[i]->iscomplete())) bmpi=i;
	if(bmpi<0) _throw("No free buffers in pool");
	if(!_bmp[bmpi]) errifnot(_bmp[bmpi]=new rrfb(dpy, win));
	b=_bmp[bmpi];  b->waituntilcomplete();
	}
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	b->init(hdr);
	return b;
}

bool rrblitter::frameready(void)
{
	if(_t) _t->checkerror();
	return(_q.items()<=0);
}

static void __rrblitter_spoilfct(void *b)
{
	if(b) ((rrfb *)b)->complete();
}

void rrblitter::sendframe(rrfb *b, bool sync)
{
	if(_t) _t->checkerror();
	if(sync) 
	{
		_prof_blit.startframe();
		blitdiff(b, _lastb);
		_prof_blit.endframe(b->_h.width*b->_h.height, 0, 1);
		if(_lastb) _lastb->complete();
		_lastb=b;
		_ready.signal();
	}
	else _q.spoil((void *)b, __rrblitter_spoilfct);
}

void rrblitter::blitdiff(rrfb *b, rrfb *lastb)
{
	int i, j;  bool needsync=false;
	bool bu=false;
	if(b->_flags&RRBMP_BOTTOMUP) bu=true;
	int tilesizex=fconfig.tilesize? fconfig.tilesize:b->_h.height;
	int tilesizey=fconfig.tilesize? fconfig.tilesize:b->_h.width;

	for(i=0; i<b->_h.height; i+=tilesizey)
	{
		int h=tilesizey, y=i;
		if(b->_h.height-i<(3*tilesizey/2)) {h=b->_h.height-i;  i+=tilesizey;}
		for(j=0; j<b->_h.width; j+=tilesizex)
		{
			int w=tilesizex, x=j;
			if(b->_h.width-j<(3*tilesizex/2)) {w=b->_h.width-j;  j+=tilesizex;}
			if(b->tileequals(lastb, x, y, w, h)) continue;
			b->drawtile(x, y, w, h);  needsync=true;
		}
	}

	if(needsync) b->sync();
}

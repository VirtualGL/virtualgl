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

#include "rrblitter.h"
#include "rrtimer.h"

#define STRIPH 64

void rrblitter::run(void)
{
//	rrfb *lastb=NULL;

	try {
 
	while(!_deadyet)
	{
		rrfb *b=NULL;
		_q.get((void **)&b);  if(_deadyet) return;
		if(!b) _throw("Queue has been shut down");
		_ready.unlock();
		_prof_blit.startframe();
		b->redraw();
		_prof_blit.endframe(b->h.width*b->h.height, 0, 1);
//		lastb=b;
	}

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);
		_ready.unlock();  throw;
	}
}

rrfb *rrblitter::getbitmap(Display *dpy, Window win, int w, int h)
{
	rrfb *b=NULL;
	_ready.lock();
	if(_t) _t->checkerror();
	_bmpmutex.lock();
	if(!_bmp[_bmpi]) errifnot(_bmp[_bmpi]=new rrfb(dpy, win));
	b=_bmp[_bmpi];  _bmpi=(_bmpi+1)%NB;
	_bmpmutex.unlock();
	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.height=hdr.frameh=h;
	hdr.width=hdr.framew=w;
	hdr.x=hdr.y=0;
	b->init(&hdr);
	return b;
}

bool rrblitter::frameready(void)
{
	if(_t) _t->checkerror();
	return(_q.items()<=0);
}

void rrblitter::sendframe(rrfb *b, bool sync)
{
	if(_t) _t->checkerror();
	if(sync) 
	{
		_prof_blit.startframe();
		blitdiff(b, _lastb);
		_prof_blit.endframe(b->h.width*b->h.height, 0, 1);
		_lastb=b;
		_ready.unlock();
	}
	else _q.add((void *)b);
}

void rrblitter::blitdiff(rrfb *b, rrfb *lastb)
{
	int endline, i;  int startline;  bool needsync=false;
	bool bu=false;
	if(b->flags&RRBMP_BOTTOMUP) bu=true;

	for(i=0; i<b->h.height; i+=STRIPH)
	{
		if(bu)
		{
			startline=b->h.height-i-STRIPH;
			if(startline<0) startline=0;
			endline=startline+min(b->h.height-i, STRIPH);
			if(b->h.height-i<2*STRIPH) {startline=0;  i+=STRIPH;}
		}
		else
		{
			startline=i;
			endline=startline+min(b->h.height-i, STRIPH);
			if(b->h.height-i<2*STRIPH) {endline=b->h.height;  i+=STRIPH;}
		}
		if(b->stripequals(lastb, startline, endline)) continue;
		b->drawstrip(startline, endline);  needsync=true;
	}

	if(needsync) b->sync();
}

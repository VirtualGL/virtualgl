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
rrfb *b=NULL;

	try {
 
	while(!deadyet)
	{
		b=NULL;
		q.get((void **)&b);  if(deadyet) return;
		if(!b) _throw("Queue has been shut down");
		prof_blit.startframe();
		b->redraw();
		prof_blit.endframe(b->h.width*b->h.height, 0, 1);
		b->complete();
//		lastb=b;
	}

	} catch(...) {if(b) b->complete();  throw;}
}

rrfb *rrblitter::getbitmap(Display *dpy, Window win, int w, int h)
{
	rrfb *b=NULL;
	if(t) t->checkerror();
	bmpmutex.lock();
	if(!bmp[bmpi]) errifnot(bmp[bmpi]=new rrfb(dpy, win));
	b=bmp[bmpi];  bmpi=(bmpi+1)%NB;
	bmpmutex.unlock();
	b->waituntilcomplete();
	if(t) t->checkerror();
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
	if(t) t->checkerror();
	return(q.items()<=0);
}

void rrblitter::sendframe(rrfb *bmp, bool sync)
{
	if(t) t->checkerror();
	if(sync) 
	{
		prof_blit.startframe();
		blitdiff(bmp, _lastb);
		prof_blit.endframe(bmp->h.width*bmp->h.height, 0, 1);
		if(_lastb) _lastb->complete();
		_lastb=bmp;
	}
	else q.add((void *)bmp);
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

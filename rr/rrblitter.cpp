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
 
	while(!deadyet)
	{
		rrfb *b=NULL;
		q.get((void **)&b);  if(deadyet) return;
		if(!b) _throw("Queue has been shut down");
		ready.unlock();
		prof_blit.startframe();
		b->redraw();
		prof_blit.endframe(b->h.bmpw*b->h.bmph, 0, 1);
//		lastb=b;
	}

	} catch(...) {ready.unlock();  throw;}
}

rrfb *rrblitter::getbitmap(Display *dpy, Window win, int w, int h)
{
	rrfb *b=NULL;
	ready.lock();
	if(t) t->checkerror();
	bmpmutex.lock();
	if(!bmp[bmpi]) errifnot(bmp[bmpi]=new rrfb(dpy, win));
	b=bmp[bmpi];  bmpi=(bmpi+1)%NB;
	bmpmutex.unlock();
	rrframeheader hdr;
	hdr.bmph=hdr.winh=h;
	hdr.bmpw=hdr.winw=w;
	hdr.bmpx=hdr.bmpy=0;
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
		prof_blit.endframe(bmp->h.bmpw*bmp->h.bmph, 0, 1);
		_lastb=bmp;
		ready.unlock();
	}
	else q.add((void *)bmp);
}

void rrblitter::blitdiff(rrfb *b, rrfb *lastb)
{
	int endline, i;  int startline;  bool needsync=false;
	bool bu=false;
	if(b->flags&RRBMP_BOTTOMUP) bu=true;

	for(i=0; i<b->h.bmph; i+=STRIPH)
	{
		if(bu)
		{
			startline=b->h.bmph-i-STRIPH;
			if(startline<0) startline=0;
			endline=startline+min(b->h.bmph-i, STRIPH);
			if(b->h.bmph-i<2*STRIPH) {startline=0;  i+=STRIPH;}
		}
		else
		{
			startline=i;
			endline=startline+min(b->h.bmph-i, STRIPH);
			if(b->h.bmph-i<2*STRIPH) {endline=b->h.bmph;  i+=STRIPH;}
		}
		if(b->stripequals(lastb, startline, endline)) continue;
		b->drawstrip(startline, endline);  needsync=true;
	}

	if(needsync) b->sync();
}

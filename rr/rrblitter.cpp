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
#include "hputil.h"

#define STRIPH 64

void *rrblitter::run(void *param)
{
	rrblitter *rrb=(rrblitter *)param;

	try {
 
	while(!rrb->deadyet)
	{
		rrbitmap *b=NULL;
		rrb->q.get((void **)&b);  if(rrb->deadyet) break;
		if(!b) _throw("Queue has been shut down");
		pthread_mutex_unlock(&rrb->ready);
		rrb->prof_blit.startframe();
		b->redraw();
		rrb->prof_blit.endframe(b->h.bmpw*b->h.bmph, 0, 1);
	}

	} catch(rrerror &e) {rrb->lasterror=e;  pthread_mutex_unlock(&rrb->ready);}
	return 0;
}

void rrblitter::checkerror(void)
{
	if(lasterror) throw lasterror;
}

rrbitmap *rrblitter::getbitmap(Display *dpy, Window win, int w, int h)
{
	rrbitmap *b=NULL;
	pthread_mutex_lock(&ready);
	checkerror();
	pthread_mutex_lock(&bmpmutex);
	if(!bmp[bmpi]) errifnot(bmp[bmpi]=new rrbitmap(dpy, win));
	b=bmp[bmpi];  bmpi=(bmpi+1)%NB;
	pthread_mutex_unlock(&bmpmutex);
	rrframeheader hdr;
	hdr.bmph=hdr.winh=h;
	hdr.bmpw=hdr.winw=w;
	hdr.bmpx=hdr.bmpy=0;
	b->init(&hdr);
	return b;
}

bool rrblitter::frameready(void)
{
	checkerror();
	return(q.items()<=0);
}

void rrblitter::sendframe(rrbitmap *bmp)
{
	checkerror();
	q.add((void *)bmp);
}

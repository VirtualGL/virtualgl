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

#include "rrcwin.h"
#include "rrerror.h"
#include "rrprofiler.h"

rrcwin::rrcwin(int dpynum, Window window) : jpgi(0), deadyet(false), profile(false),
	t(NULL)
{
	char dpystr[80];
	if(dpynum<0 || dpynum>255 || !window)
		throw(rrerror("rrcwin::rrcwin()", "Invalid argument"));
	sprintf(dpystr, "localhost:%d.0", dpynum);
	this->dpynum=dpynum;  this->window=window;
	char *ev=NULL;
	if((ev=getenv("RRPROFILE"))!=NULL && !strncmp(ev, "1", 1))
		{profile=true;}
	b=new rrfb(dpystr, window);
	errifnot(b);
	errifnot(t=new Thread(this));
	t->start();
}

rrcwin::~rrcwin(void)
{
	deadyet=true;
	q.release();
	frameready.unlock();
	if(t) t->stop();
	delete b;
}

int rrcwin::match(int dpynum, Window window)
{
	return (this->dpynum==dpynum && this->window==window);
}

rrjpeg *rrcwin::getFrame(void)
{
	rrjpeg *j=NULL;
	if(t) t->checkerror();
	frameready.lock();
	if(t) t->checkerror();
	jpgmutex.lock();
	j=&jpg[jpgi];  jpgi=(jpgi+1)%NB;
	jpgmutex.unlock();
	return j;
}

void rrcwin::drawFrame(rrjpeg *f)
{
	if(t) t->checkerror();
	q.add(f);
}


void rrcwin::run(void)
{
	rrprofiler pt("Total"), pb("Blit"), pd("Decompress");

	try {

	while(!deadyet)
	{
		rrjpeg *j=NULL;
		q.get((void **)&j);  if(deadyet) break;
		if(!j) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		frameready.unlock();
		if(j->h.eof)
		{
			pb.startframe();
			b->init(&j->h);
			b->redraw();
			pb.endframe(b->h.winw*b->h.winh, 0, 1);
			pt.endframe(b->h.winw*b->h.winh, 0, 1);
			pt.startframe();
		}
		else
		{
			pd.startframe();
			*b=*j;
			pd.endframe(j->h.bmpw*j->h.bmph, j->h.size, (double)(j->h.bmpw*j->h.bmph)/
				(double)(j->h.winw*j->h.winh));
		}
	}

	} catch(...) {frameready.unlock();  throw;}
}

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

#include "rrprofiler.h"
#include "rrcwin.h"


rrcwin::rrcwin(int dpynum, Window window)
{
	char dpystr[80];
	if(dpynum<0 || dpynum>255 || !window) _throw("Invalid argument to rrcwin constructor");
	sprintf(dpystr, "localhost:%d.0", dpynum);
	this->dpynum=dpynum;  this->window=window;
	tryunix(pthread_mutex_init(&frameready, NULL));
	b=new rrbitmap(dpystr, window);
	errifnot(b);
	deadyet=false;  displaythnd=0;
	tryunix(pthread_create(&displaythnd, NULL, displayer, (void *)this));
}


rrcwin::~rrcwin(void)
{
	deadyet=true;
	q.release();
	pthread_mutex_unlock(&frameready);
	if(displaythnd) {pthread_join(displaythnd, NULL);  displaythnd=0;}
	delete b;
}


int rrcwin::match(int dpynum, Window window)
{
	return (this->dpynum==dpynum && this->window==window);
}


rrjpeg *rrcwin::getFrame(void)
{
	tryunix(pthread_mutex_lock(&frameready));
	checkerror();
	rrjpeg *j=new rrjpeg;
	if(!j) _throw("Could not allocate receive buffer");
	return j;
}


void rrcwin::drawFrame(rrjpeg *f)
{
	checkerror();
	q.add(f);
}


void *rrcwin::displayer(void *param)
{
	rrprofiler pt("Total"), pb("Blit"), pd("Decompress");
	rrcwin *rrw=(rrcwin *)param;

	try
	{
		while(!rrw->deadyet)
		{
			rrjpeg *j=NULL;
			rrw->q.get((void **)&j);  if(rrw->deadyet) break;
			if(!j) _throw("Invalid image received from queue");
			pthread_mutex_unlock(&rrw->frameready);
			if(j->h.eof)
			{
				pb.startframe();
				rrw->b->init(&j->h);
				rrw->b->redraw();
				pb.endframe(rrw->b->h.winw*rrw->b->h.winh, 0, 1);
				pt.endframe(rrw->b->h.winw*rrw->b->h.winh, 0, 1);
				pt.startframe();
			}
			else
			{
				pd.startframe();
				*rrw->b=*j;
				pd.endframe(j->h.bmpw*j->h.bmph, j->h.size, (double)(j->h.bmpw*j->h.bmph)/
					(double)(j->h.winw*j->h.winh));
			}
			delete j;
		}
	}
	catch(rrerror &e)
	{
		if(!rrw->deadyet) rrw->lasterror=e;
		pthread_mutex_unlock(&rrw->frameready);
	}
	return 0;
}

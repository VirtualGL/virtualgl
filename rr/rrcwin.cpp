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
#include "rrtimer.h"

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

void rrcwin::showprofile(rrframeheader *h, int size)
{
	static rrtimer timer;
	static double tstart=0., mpixels=0., displaytime=0., mbits=0., frames=0.;
	if(profile)
	{
		if(tstart)
		{
			displaytime+=timer.time()-tstart;
			if(!h->eof)
			{
				mpixels+=(double)h->bmpw*(double)h->bmph/1000000.;
				mbits+=(double)size*8./1000000.;
				frames+=(double)(h->bmpw*h->bmph)/(double)(h->winw*h->winh);
			}
		}
		if(displaytime>1.)
		{
			rrout.PRINT("%.2f Mpixels/sec - %.2f fps - %.2f Mbps\n", mpixels/displaytime,
				frames/displaytime, mbits/displaytime);
			displaytime=0.;  mpixels=0.;  frames=0;  mbits=0.;
		}
		tstart=timer.time();
	}
}

void rrcwin::run(void)
{
	try {

	while(!deadyet)
	{
		rrjpeg *j=NULL;
		q.get((void **)&j);  if(deadyet) break;
		if(!j) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		frameready.unlock();
		if(j->h.eof)
		{
			b->init(&j->h);
			b->redraw();
		}
		else *b=*j;
		showprofile(&b->h, j->h.size);
	}

	} catch(...) {frameready.unlock();  throw;}
}

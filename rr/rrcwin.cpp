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

//#define RRPROFILE
#include "rrcwin.h"


rrcwin::rrcwin(int dpynum, Window window)
{
	char dpystr[80];
	if(dpynum<0 || dpynum>255 || !window) _throw("Invalid argument to rrcwin constructor");
	sprintf(dpystr, "localhost:%d.0", dpynum);
	this->dpynum=dpynum;  this->window=window;
	tryunix(pthread_mutex_init(&frameready, NULL));
	profile=false;  char *ev=NULL;
	if((ev=getenv("RRPROFILE"))!=NULL && !strncmp(ev, "1", 1)) {profile=true;  hptimer_init();}
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


void rrcwin::showprofile(rrframeheader *h, int size)
{
	static double tstart=0., mpixels=0., displaytime=0., mbits=0., frames=0.;
	if(profile)
	{
		if(tstart)
		{
			displaytime+=hptime()-tstart;
			if(!h->eof)
			{
				mpixels+=(double)h->bmpw*(double)h->bmph/1000000.;
				mbits+=(double)size*8./1000000.;
				frames+=(double)(h->bmpw*h->bmph)/(double)(h->winw*h->winh);
			}
		}
		if(displaytime>1.)
		{
			hpprintf("%.2f Mpixels/sec - %.2f fps - %.2f Mbps\n", mpixels/displaytime,
				frames/displaytime, mbits/displaytime);
			displaytime=0.;  mpixels=0.;  frames=0;  mbits=0.;
		}
		tstart=hptime();
	}
}


void *rrcwin::displayer(void *param)
{
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
				rrw->b->init(&j->h);
				rrw->b->redraw();
			}
			else *rrw->b=*j;
			rrw->showprofile(&rrw->b->h, j->h.size);
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

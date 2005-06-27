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

rrcwin::rrcwin(int dpynum, Window window, int _drawmethod) : drawmethod(_drawmethod),
	b(NULL), jpgi(0), glf(NULL), deadyet(false), t(NULL)
{
	char dpystr[80];
	if(dpynum<0 || dpynum>255 || !window)
		throw(rrerror("rrcwin::rrcwin()", "Invalid argument"));
	sprintf(dpystr, "localhost:%d.0", dpynum);
	this->dpynum=dpynum;  this->window=window;
	b=new rrfb(dpystr, window);
	if(!b) _throw("Could not allocate class instance");
	#ifdef USEGL
	if(drawmethod==RR_DRAWAUTO || drawmethod==RR_DRAWOGL)
	{
		try
		{
			glf=new rrglframe(dpystr, window);
			if(!glf) _throw("Could not allocate class instance");
		}
		catch(rrerror &e)
		{
			rrout.println("OpenGL error-- %s\nUsing X11 drawing instead",
				e.getMessage());
			if(glf) {delete glf;  glf=NULL;}
			drawmethod=RR_DRAWX11;
		}
	}
	if(drawmethod==RR_DRAWOGL)
	{
		delete b;  b=NULL;
	}
	#endif
	errifnot(t=new Thread(this));
	t->start();
}

rrcwin::~rrcwin(void)
{
	deadyet=true;
	q.release();
	if(t) t->stop();
	if(b) delete b;
	#ifdef USEGL
	if(glf) delete glf;
	#endif
	for(int i=0; i<NB; i++) jpg[i].complete();
}

int rrcwin::match(int dpynum, Window window)
{
	return (this->dpynum==dpynum && this->window==window);
}

rrjpeg *rrcwin::getFrame(void)
{
	rrjpeg *j=NULL;
	if(t) t->checkerror();
	jpgmutex.lock();
	j=&jpg[jpgi];  jpgi=(jpgi+1)%NB;
	jpgmutex.unlock();
	j->waituntilcomplete();
	if(t) t->checkerror();
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
	rrjpeg *j=NULL;
	double xtime=0., gltime=0., t1=0.;
	bool dobenchmark=false;
	bool firstframe=(drawmethod==RR_DRAWAUTO)? true:false;

	try {

	while(!deadyet)
	{
		j=NULL;
		q.get((void **)&j);  if(deadyet) break;
		if(!j) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		if(j->h.eof)
		{
			pb.startframe();
			if(dobenchmark) t1=rrtime();
			if(b)
			{
				b->init(&j->h);
				b->redraw();
			}
			if(dobenchmark) xtime+=rrtime()-t1;
			#ifdef USEGL
			if(dobenchmark) t1=rrtime();
			if(glf)
			{
				glf->init(&j->h);
				glf->redraw();
			}
			if(dobenchmark) gltime+=rrtime()-t1;
			if(dobenchmark && (xtime>0.25 || gltime>0.25))
			{
				rrout.print("X11: %.3f ms  OGL: %.3f ms  ", xtime*1000., gltime*1000.);
				if(gltime<0.95*xtime)
				{
					delete b;  b=NULL;  rrout.println("Using OpenGL");
				}
				else
				{
					delete glf;  glf=NULL;  rrout.println("Using X11");
				}
				dobenchmark=false;
			}
			if(firstframe) {dobenchmark=true;  firstframe=false;}
			#endif
			if(b)
			{
				pb.endframe(b->h.framew*b->h.frameh, 0, 1);
				pt.endframe(b->h.framew*b->h.frameh, 0, 1);
			}
			#ifdef USEGL
			if(glf)
			{
				pb.endframe(glf->h.framew*glf->h.frameh, 0, 1);
				pt.endframe(glf->h.framew*glf->h.frameh, 0, 1);
			}
			#endif
			pt.startframe();
		}
		else
		{
			pd.startframe();
			if(dobenchmark) t1=rrtime();
			if(b) *b=*j;
			if(dobenchmark) xtime+=rrtime()-t1;
			#ifdef USEGL
			if(dobenchmark) t1=rrtime();
			if(glf) *glf=*j;
			if(dobenchmark) gltime+=rrtime()-t1;
			#endif
			pd.endframe(j->h.width*j->h.height, j->h.size, (double)(j->h.width*j->h.height)/
				(double)(j->h.framew*j->h.frameh));
		}
		j->complete();
	}

	} catch(rrerror &e)
	{
		if(t) t->seterror(e);  if(j) j->complete();  throw;
	}
}

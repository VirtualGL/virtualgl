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

rrcwin::rrcwin(int dpynum, Window window, int drawmethod, bool stereo) :
	_drawmethod(drawmethod), _b(NULL), _jpgi(0),
	#ifdef USEGL
	_glf(NULL),
	#endif
	_deadyet(false), _t(NULL), _stereo(stereo)
{
	char dpystr[80];
	if(dpynum<0 || dpynum>255 || !window)
		throw(rrerror("rrcwin::rrcwin()", "Invalid argument"));
	#ifdef XDK
	sprintf(dpystr, "LOCALPC:%d.0", dpynum);
	#else
	sprintf(dpystr, ":%d.0", dpynum);
	#endif
	_dpynum=dpynum;  _window=window;
	_b=new rrfb(dpystr, window);
	if(!_b) _throw("Could not allocate class instance");
	#ifdef USEGL
	if(_stereo) _drawmethod=RR_DRAWOGL;
	initgl();
	#else
	_stereo=false;
	#endif
	errifnot(_t=new Thread(this));
	_t->start();
}

rrcwin::~rrcwin(void)
{
	_deadyet=true;
	_q.release();
	if(_t) _t->stop();
	if(_b) delete _b;
	#ifdef USEGL
	if(_glf) delete _glf;
	#endif
	for(int i=0; i<NB; i++) _jpg[i].complete();
	if(_t) {delete _t;  _t=NULL;}
}

#ifdef USEGL
void rrcwin::initgl(void)
{
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "LOCALPC:%d.0", _dpynum);
	#else
	sprintf(dpystr, "localhost:%d.0", _dpynum);
	#endif
	if(_drawmethod==RR_DRAWAUTO || _drawmethod==RR_DRAWOGL)
	{
		try
		{
			_glf=new rrglframe(dpystr, _window);
			if(!_glf) _throw("Could not allocate class instance");
		}
		catch(rrerror &e)
		{
			rrout.println("OpenGL error-- %s\nUsing X11 drawing instead",
				e.getMessage());
			if(_glf) {delete _glf;  _glf=NULL;}
			_drawmethod=RR_DRAWX11;
			rrout.println("Stereo requires OpenGL drawing.  Disabling stereo.");
			_stereo=false;
		}
	}
	if(_drawmethod==RR_DRAWOGL)
	{
		delete _b;  _b=NULL;
	}
}
#endif

int rrcwin::match(int dpynum, Window window)
{
	return (_dpynum==dpynum && _window==window);
}

rrjpeg *rrcwin::getFrame(void)
{
	rrjpeg *j=NULL;
	if(_t) _t->checkerror();
	_jpgmutex.lock();
	j=&_jpg[_jpgi];  _jpgi=(_jpgi+1)%NB;
	_jpgmutex.unlock();
	j->waituntilcomplete();
	if(_t) _t->checkerror();
	return j;
}

void rrcwin::drawFrame(rrjpeg *j)
{
	if(_t) _t->checkerror();
	#ifdef USEGL
	if((j->_flags==RR_RIGHT || j->_flags==RR_LEFT) && (!_stereo || _drawmethod!=RR_DRAWOGL))
	{
		_stereo=true;  _drawmethod=RR_DRAWOGL;
		initgl();
	}
	#endif
	_q.add(j);
}


void rrcwin::run(void)
{
	rrprofiler pt("Total     "), pb("Blit      "), pd("Decompress");
	rrjpeg *j=NULL;  long bytes=0;
	double xtime=0., gltime=0., t1=0.;
	bool dobenchmark=false;
	bool firstframe=(_drawmethod==RR_DRAWAUTO)? true:false;

	try {

	while(!_deadyet)
	{
		j=NULL;
		_q.get((void **)&j);  if(_deadyet) break;
		if(!j) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		if(j->_h.flags==RR_EOF)
		{
			bool stereo=false;
			pb.startframe();
			if(dobenchmark) t1=rrtime();
			if(_b)
			{
				_b->init(&j->_h);
				_b->redraw();
			}
			if(dobenchmark) xtime+=rrtime()-t1;
			#ifdef USEGL
			if(dobenchmark) t1=rrtime();
			if(_glf)
			{
				if(_glf->_h.flags==RR_LEFT || _glf->_h.flags==RR_RIGHT) stereo=true;
				_glf->init(&j->_h);
				_glf->redraw();
			}
			if(dobenchmark) gltime+=rrtime()-t1;
			if(dobenchmark && (xtime>0.25 || gltime>0.25))
			{
				rrout.print("X11: %.3f ms  OGL: %.3f ms  ", xtime*1000., gltime*1000.);
				if(gltime<0.95*xtime)
				{
					delete _b;  _b=NULL;  rrout.println("Using OpenGL");
				}
				else
				{
					delete _glf;  _glf=NULL;  rrout.println("Using X11");
				}
				dobenchmark=false;
			}
			if(firstframe) {dobenchmark=true;  firstframe=false;}
			#endif
			if(_b)
			{
				pb.endframe(_b->_h.framew*_b->_h.frameh* (stereo? 2:1), 0, 1);
				pt.endframe(_b->_h.framew*_b->_h.frameh* (stereo? 2:1), bytes, 1);
			}
			#ifdef USEGL
			if(_glf)
			{
				pb.endframe(_glf->_h.framew*_glf->_h.frameh* (stereo? 2:1), 0, 1);
				pt.endframe(_glf->_h.framew*_glf->_h.frameh* (stereo? 2:1), bytes, 1);
			}
			#endif
			bytes=0;
			pt.startframe();
		}
		else
		{
			pd.startframe();
			if(dobenchmark) t1=rrtime();
			if(_b) *_b=*j;
			if(dobenchmark) xtime+=rrtime()-t1;
			#ifdef USEGL
			if(dobenchmark) t1=rrtime();
			if(_glf) *_glf=*j;
			if(dobenchmark) gltime+=rrtime()-t1;
			#endif
			bool stereo=j->_h.flags==RR_LEFT || j->_h.flags==RR_RIGHT;
			pd.endframe(j->_h.width*j->_h.height, 0, (double)(j->_h.width*j->_h.height)/
				(double)(j->_h.framew*j->_h.frameh) * (stereo? 0.5:1.0));
			bytes+=j->_h.size;
		}
		j->complete();
	}

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);  if(j) j->complete();  throw;
	}
}
